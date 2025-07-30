
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

    void onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) override;

    void onResize(uint32_t width, uint32_t height) override;

private:
    void createBuffers();

    void createPipelineState();

    void updateUniforms() const;

    NS::SharedPtr<MTL::RenderPipelineState>                 m_pipelineState;
    NS::SharedPtr<MTL::Buffer>                              m_vertexBuffer;
    NS::SharedPtr<MTL::Buffer>                              m_indexBuffer;
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
    int32_t width;
    int32_t height;
    SDL_GetWindowSizeInPixels(m_window, &width, &height);
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0f);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;

    m_mainCamera = std::make_unique<Camera>(XMFLOAT3 { 0.0F, 0.0F, 0.0F },
        XMFLOAT3 { 0.0F, 0.0F, -1.0F }, XMFLOAT3 { 0.0F, 1.0F, 0.0F }, fov, aspect, near, far);

    createBuffers();

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

void Instancing::onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& /*timer*/)
{
    updateUniforms();

    commandEncoder->setRenderPipelineState(m_pipelineState.get());
    commandEncoder->setDepthStencilState(m_depthStencilState.get());
    commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setVertexBuffer(m_vertexBuffer.get(), 0, 0);
    commandEncoder->setVertexBuffer(m_instanceBuffer[m_frameIndex].get(), 0, 1);
    commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
        m_indexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16, m_indexBuffer.get(), 0,
        s_instanceCount);
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

    MTL::RenderPipelineDescriptor* pipelineDescriptor
        = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(m_frameBufferPixelFormat);
    pipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(
        MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
        MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(
        MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(
        MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(
        MTL::BlendOperationAdd);
    pipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setVertexFunction(m_pipelineLibrary->newFunction(
        NS::String::string("instancing_vertex", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(m_pipelineLibrary->newFunction(
        NS::String::string("instancing_fragment", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    pipelineDescriptor->setSampleCount(s_multisampleCount);

    NS::Error* error = nullptr;
    m_pipelineState = NS::TransferPtr(m_device->newRenderPipelineState(pipelineDescriptor, &error));
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
    static const Vertex vertices[] = { { .position = { -1, 1, 1, 1 }, .color = { 0, 1, 1, 1 } },
        { .position = { -1, -1, 1, 1 }, .color = { 0, 0, 1, 1 } },
        { .position = { 1, -1, 1, 1 }, .color = { 1, 0, 1, 1 } },
        { .position = { 1, 1, 1, 1 }, .color = { 1, 1, 1, 1 } },
        { .position = { -1, 1, -1, 1 }, .color = { 0, 1, 0, 1 } },
        { .position = { -1, -1, -1, 1 }, .color = { 0, 0, 0, 1 } },
        { .position = { 1, -1, -1, 1 }, .color = { 1, 0, 0, 1 } },
        { .position = { 1, 1, -1, 1 }, .color = { 1, 1, 0, 1 } } };

    static const uint16_t indices[] = { 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4, 4, 0, 3, 3, 7, 4, 1, 5,
        6, 6, 2, 1, 0, 1, 2, 2, 3, 0, 7, 6, 5, 5, 4, 7 };

    m_vertexBuffer = NS::TransferPtr(
        m_device->newBuffer(vertices, sizeof(vertices), MTL::ResourceCPUCacheModeDefaultCache));
    m_vertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    m_indexBuffer = NS::TransferPtr(
        m_device->newBuffer(indices, sizeof(indices), MTL::ResourceOptionCPUCacheModeDefault));
    m_indexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    constexpr size_t instanceDataSize
        = static_cast<unsigned long>(s_bufferCount * s_instanceCount) * sizeof(InstanceData);
    for (auto index = 0; std::cmp_less(index, s_bufferCount); index++)
    {
        const auto                      label = fmt::format("Instance Buffer: {}", index);
        const NS::SharedPtr<NS::String> nsLabel
            = NS::TransferPtr(NS::String::string(label.c_str(), NS::ASCIIStringEncoding));
        m_instanceBuffer[index] = NS::TransferPtr(
            m_device->newBuffer(instanceDataSize, MTL::ResourceOptionCPUCacheModeDefault));
        m_instanceBuffer[index]->setLabel(nsLabel.get());
    }
}

void Instancing::updateUniforms() const
{
    MTL::Buffer* instanceBuffer = m_instanceBuffer[m_frameIndex].get();

    auto* instanceData = static_cast<InstanceData*>(instanceBuffer->contents());
    for (auto index = 0; index < s_instanceCount; ++index)
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
