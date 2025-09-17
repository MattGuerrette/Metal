
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <memory>

#include <fmt/core.h>

#include <Metal/Metal.hpp>

#include "Camera.hpp"
#include "Example.hpp"

#include <SDL3/SDL_main.h>

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector4 position;
    Vector4 color;
};

XM_ALIGNED_STRUCT(16) Uniforms
{
    [[maybe_unused]] Matrix modelViewProjection;
};

XM_ALIGNED_STRUCT(16) InstanceData
{
    Matrix transform;
};

class Instancing final : public Example
{
    static constexpr int s_instanceCount = 3;

public:
    Instancing();

    ~Instancing() override;

    bool onLoad() override;

    void onUpdate(const GameTimer& timer) override;

    void onRender(MTL4::RenderCommandEncoder* commandEncoder, const GameTimer& timer) override;

    void onResize(uint32_t width, uint32_t height) override;

private:
    void createArgumentTable();

    void createResidencySet();

    void createBuffers();

    void createPipelineState();

    void updateUniforms() const;

    NS::SharedPtr<MTL::RenderPipelineState>                 m_pipelineState;
    NS::SharedPtr<MTL::Buffer>                              m_vertexBuffer;
    NS::SharedPtr<MTL::Buffer>                              m_indexBuffer;
    NS::SharedPtr<MTL4::ArgumentTable>                      m_argumentTable;
    NS::SharedPtr<MTL::ResidencySet>                        m_residencySet;
    std::array<NS::SharedPtr<MTL::Buffer>, s_instanceCount> m_instanceBuffer;
    std::unique_ptr<Camera>                                 m_mainCamera;
    float                                                   m_rotationX = 0.0F;
    float                                                   m_rotationY = 0.0F;
};

Instancing::Instancing()
    : Example("Instancing", 800, 600)
{
}

Instancing::~Instancing() = default;

bool Instancing::onLoad()
{
    const auto width = windowWidth();
    const auto height = windowHeight();

    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0f);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;

    m_mainCamera = std::make_unique<Camera>(XMFLOAT3 { 0.0F, 0.0F, 0.0F },
        XMFLOAT3 { 0.0F, 0.0F, -1.0F }, XMFLOAT3 { 0.0F, 1.0F, 0.0F }, fov, aspect, near, far);

    createBuffers();

    createArgumentTable();

    createResidencySet();

    createPipelineState();

    return true;
}

void Instancing::onResize(uint32_t width, uint32_t height)
{
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0f);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;
    m_mainCamera->setProjection(fov, aspect, near, far);
}

void Instancing::onUpdate(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.elapsedSeconds());

    m_rotationX += elapsed;
    m_rotationY += elapsed;
}

void Instancing::onRender(MTL4::RenderCommandEncoder* commandEncoder, const GameTimer& /*timer*/)
{
    updateUniforms();

    const auto currentFrameIndex = frameIndex();

    commandEncoder->setRenderPipelineState(m_pipelineState.get());
    commandEncoder->setDepthStencilState(depthStencilState());
    commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setArgumentTable(m_argumentTable.get(), MTL::RenderStageVertex);
    
    m_argumentTable->setAddress(m_vertexBuffer->gpuAddress(), 0);
    
    commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, m_indexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16, m_indexBuffer->gpuAddress(), m_indexBuffer->length(), s_instanceCount);
}

void Instancing::createResidencySet()
{
    NS::Error* error = nullptr;

    MTL::ResidencySetDescriptor* residencySetDescriptor
        = MTL::ResidencySetDescriptor::alloc()->init();
    m_residencySet = NS::TransferPtr(device()->newResidencySet(residencySetDescriptor, &error));
    if (error != nullptr)
    {
        throw std::runtime_error(fmt::format(
            "Failed to create residence set: {}", error->localizedFailureReason()->utf8String()));
    }
    
    m_residencySet->addAllocation(m_vertexBuffer.get());
    m_residencySet->addAllocation(m_indexBuffer.get());
    for (uint32_t i = 0; i < s_bufferCount; i++)
    {
        m_residencySet->addAllocation(m_instanceBuffer[i].get());
    }
    
    commandQueue()->addResidencySet(m_residencySet.get());
    commandQueue()->addResidencySet(metalLayer()->residencySet());
    m_residencySet->commit();
}

void Instancing::createArgumentTable()
{
    NS::Error* error = nullptr;

    MTL4::ArgumentTableDescriptor* argTableDescriptor
        = MTL4::ArgumentTableDescriptor::alloc()->init();
    argTableDescriptor->setMaxBufferBindCount(2);

    m_argumentTable = NS::TransferPtr(device()->newArgumentTable(argTableDescriptor, &error));
    if (error != nullptr)
    {
        throw std::runtime_error(fmt::format(
            "Failed to create argument table: {}", error->localizedFailureReason()->utf8String()));
    }
}

void Instancing::createPipelineState()
{
    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    // Position
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(0)->setOffset(0);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(0);

    // Color
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(1)->setOffset(offsetof(Vertex, color));
    vertexDescriptor->attributes()->object(1)->setBufferIndex(0);

    vertexDescriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(Vertex));

    MTL4::RenderPipelineDescriptor* pipelineDescriptor
        = MTL4::RenderPipelineDescriptor::alloc()->init();

    MTL4::LibraryFunctionDescriptor* vertexFunction
        = MTL4::LibraryFunctionDescriptor::alloc()->init();
    vertexFunction->setLibrary(shaderLibrary());
    vertexFunction->setName(MTLSTR("instancing_vertex"));
    pipelineDescriptor->setVertexFunctionDescriptor(vertexFunction);

    MTL4::LibraryFunctionDescriptor* fragmentFunction
        = MTL4::LibraryFunctionDescriptor::alloc()->init();
    fragmentFunction->setLibrary(shaderLibrary());
    fragmentFunction->setName(MTLSTR("instancing_fragment"));
    pipelineDescriptor->setFragmentFunctionDescriptor(fragmentFunction);

    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    pipelineDescriptor->setRasterSampleCount(4);
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(s_defaultPixelFormat);

    MTL4::CompilerTaskOptions* compilerTaskOptions = MTL4::CompilerTaskOptions::alloc()->init();
    // TODO: support binary archives
    // if (archive != null)
    // {
    //      compilerTaskOptions->setLookupArchives(archive);
    // }

    //    pipelineDescriptor->set
    //    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(s_defaultPixelFormat);
    //    pipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
    //    pipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(
    //        MTL::BlendFactorSourceAlpha);
    //    pipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
    //        MTL::BlendFactorOneMinusSourceAlpha);
    //    pipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
    //    pipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(
    //        MTL::BlendFactorSourceAlpha);
    //    pipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(
    //        MTL::BlendFactorOneMinusSourceAlpha);
    //    pipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(
    //        MTL::BlendOperationAdd);
    //    pipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    //    pipelineDescriptor->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    //    pipelineDescriptor->setVertexFunction(shaderLibrary()->newFunction(
    //        NS::String::string("triangle_vertex", NS::ASCIIStringEncoding)));
    //    pipelineDescriptor->setFragmentFunction(shaderLibrary()->newFunction(
    //        NS::String::string("triangle_fragment", NS::ASCIIStringEncoding)));
    //    pipelineDescriptor->setSampleCount(s_multisampleCount);

    NS::Error*                error = nullptr;
    MTL4::CompilerDescriptor* compilerDescriptor = MTL4::CompilerDescriptor::alloc()->init();
    MTL4::Compiler*           compiler = device()->newCompiler(compilerDescriptor, &error);
    if (error != nullptr)
    {
        throw std::runtime_error(fmt::format(
            "Failed to create shader compiler: {}", error->localizedFailureReason()->utf8String()));
    }

    m_pipelineState = NS::TransferPtr(
        compiler->newRenderPipelineState(pipelineDescriptor, compilerTaskOptions, &error));
    if (error != nullptr)
    {
        throw std::runtime_error(fmt::format(
            "Failed to create pipeline state: {}", error->localizedFailureReason()->utf8String()));
    }

    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void Instancing::createBuffers()
{
    constexpr std::array vertices
        = std::to_array<Vertex>({ { .position = { -1, 1, 1, 1 }, .color = { 0, 1, 1, 1 } },
            { .position = { -1, -1, 1, 1 }, .color = { 0, 0, 1, 1 } },
            { .position = { 1, -1, 1, 1 }, .color = { 1, 0, 1, 1 } },
            { .position = { 1, 1, 1, 1 }, .color = { 1, 1, 1, 1 } },
            { .position = { -1, 1, -1, 1 }, .color = { 0, 1, 0, 1 } },
            { .position = { -1, -1, -1, 1 }, .color = { 0, 0, 0, 1 } },
            { .position = { 1, -1, -1, 1 }, .color = { 1, 0, 0, 1 } },
            { .position = { 1, 1, -1, 1 }, .color = { 1, 1, 0, 1 } } });
    constexpr size_t vertexBufferLength = vertices.size() * sizeof(Vertex);

    constexpr std::array indices = std::to_array<uint16_t>({ 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4, 4,
        0, 3, 3, 7, 4, 1, 5, 6, 6, 2, 1, 0, 1, 2, 2, 3, 0, 7, 6, 5, 5, 4, 7 });
    constexpr size_t     indexBufferLength = indices.size() * sizeof(uint16_t);

    m_vertexBuffer = NS::TransferPtr(device()->newBuffer(
        vertices.data(), vertexBufferLength, MTL::ResourceCPUCacheModeDefaultCache));
    m_vertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    m_indexBuffer = NS::TransferPtr(device()->newBuffer(
        indices.data(), indexBufferLength, MTL::ResourceOptionCPUCacheModeDefault));
    m_indexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    constexpr size_t instanceDataSize
        = static_cast<unsigned long>(s_bufferCount * s_instanceCount) * sizeof(InstanceData);
    for (auto index = 0; std::cmp_less(index, s_bufferCount); ++index)
    {
        const auto                      label = fmt::format("Instance Buffer: {}", index);
        const NS::SharedPtr<NS::String> nsLabel
            = NS::TransferPtr(NS::String::string(label.c_str(), NS::ASCIIStringEncoding));
        m_instanceBuffer[index] = NS::TransferPtr(
            device()->newBuffer(instanceDataSize, MTL::ResourceOptionCPUCacheModeDefault));
        m_instanceBuffer[index]->setLabel(nsLabel.get());
    }
}

void Instancing::updateUniforms() const
{
    const auto currentFrameIndex = frameIndex();

    MTL::Buffer* instanceBuffer = m_instanceBuffer[currentFrameIndex].get();

    auto* instanceData = static_cast<InstanceData*>(instanceBuffer->contents());
    for (auto index = 0; std::cmp_less(index, s_instanceCount); ++index)
    {
        auto position = Vector3(-5.0F + 5.0F * static_cast<float>(index), 0.0F, -10.0F);
        auto rotationX = m_rotationX;
        auto rotationY = m_rotationY;
        auto scaleFactor = 1.0F;

        const Vector3 xAxis = Vector3::Right;
        const Vector3 yAxis = Vector3::Up;

        const Matrix         xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
        const Matrix         yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
        const Matrix         rotation = xRot * yRot;
        const Matrix         translation = Matrix::CreateTranslation(position);
        const Matrix         scale = Matrix::CreateScale(scaleFactor);
        const Matrix         model = scale * rotation * translation;
        const CameraUniforms cameraUniforms = m_mainCamera->uniforms();

        instanceData[index].transform = model * cameraUniforms.viewProjection;
    }
    
    m_argumentTable->setAddress(m_instanceBuffer[currentFrameIndex]->gpuAddress(), 1);
}

int main(int argc, char** argv)
{
    int result = EXIT_FAILURE;
    try
    {
        const auto example = std::make_unique<Instancing>();
        result = example->run(argc, argv);
    }
    catch (const std::runtime_error&)
    {
        fmt::println("Exiting...");
    }

    return result;
}
