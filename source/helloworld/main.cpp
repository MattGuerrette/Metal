////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include <fmt/core.h>

#include <Metal/Metal.hpp>

#include "Camera.hpp"
#include "Example.hpp"

#include <SDL3/SDL_main.h>

using namespace DirectX;

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector4 position;
    Vector4 color;
};

XM_ALIGNED_STRUCT(16) Uniforms
{
    [[maybe_unused]] Matrix modelViewProjection;
};
constexpr size_t g_alignedUniformSize = sizeof(Uniforms) + 0xFF & -0x100;

class HelloWorld final : public Example
{
public:
    HelloWorld();

    ~HelloWorld() override;

    bool onLoad() override;

    void onUpdate(const GameTimer& timer) override;

    void onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) override;

    void onResize(uint32_t width, uint32_t height) override;

private:
    void createBuffers();

    void createPipelineState();

    void updateUniforms() const;

    NS::SharedPtr<MTL::RenderPipelineState>               m_pipelineState;
    NS::SharedPtr<MTL::Buffer>                            m_vertexBuffer;
    NS::SharedPtr<MTL::Buffer>                            m_indexBuffer;
    std::array<NS::SharedPtr<MTL::Buffer>, s_bufferCount> m_uniformBuffer;
    std::unique_ptr<Camera>                               m_mainCamera;
    float                                                 m_rotationY = 0.0F;
};

HelloWorld::HelloWorld()
    : Example("Hello, Metal", 800, 600)
{
}

HelloWorld::~HelloWorld() = default;

bool HelloWorld::onLoad()
{
    const auto      width = windowWidth();
    const auto      height = windowHeight();
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0F);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;

    m_mainCamera = std::make_unique<Camera>(XMFLOAT3 { 0.0F, 0.0F, 0.0F },
        XMFLOAT3 { 0.0F, 0.0F, -1.0F }, XMFLOAT3 { 0.0F, 1.0F, 0.0F }, fov, aspect, near, far);

    createBuffers();

    createPipelineState();

    return true;
}

void HelloWorld::onUpdate(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.elapsedSeconds());

    m_rotationY += elapsed;
}

void HelloWorld::onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& /*timer*/)
{
    updateUniforms();

    const auto currentFrameIndex = frameIndex();
    const auto uniformBufferOffset = g_alignedUniformSize * currentFrameIndex;

    commandEncoder->setRenderPipelineState(m_pipelineState.get());
    commandEncoder->setDepthStencilState(depthStencilState());
    commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setVertexBuffer(m_vertexBuffer.get(), 0, 0);
    commandEncoder->setVertexBuffer(
        m_uniformBuffer[currentFrameIndex].get(), uniformBufferOffset, 1);

    commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
        m_indexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16, m_indexBuffer.get(), 0);
}

void HelloWorld::onResize(const uint32_t width, const uint32_t height)
{
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0F);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;
    m_mainCamera->setProjection(fov, aspect, near, far);
}

void HelloWorld::createPipelineState()
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
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(s_defaultPixelFormat);
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
    pipelineDescriptor->setVertexFunction(shaderLibrary()->newFunction(
        NS::String::string("triangle_vertex", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(shaderLibrary()->newFunction(
        NS::String::string("triangle_fragment", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    pipelineDescriptor->setSampleCount(s_multisampleCount);

    NS::Error* error = nullptr;
    m_pipelineState = NS::TransferPtr(device()->newRenderPipelineState(pipelineDescriptor, &error));
    if (error != nullptr)
    {
        throw std::runtime_error(fmt::format(
            "Failed to create pipeline state: {}", error->localizedFailureReason()->utf8String()));
    }

    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void HelloWorld::createBuffers()
{
    constexpr std::array vertices
        = std::to_array<Vertex>({ { .position = { 0, 1, 0, 1 }, .color = { 1, 0, 0, 1 } },
            { .position = { -1, -1, 0, 1 }, .color = { 0, 1, 0, 1 } },
            { .position = { 1, -1, 0, 1 }, .color = { 0, 0, 1, 1 } } });
    constexpr size_t vertexBufferLength = vertices.size() * sizeof(Vertex);

    constexpr std::array indices = std::to_array<uint16_t>({ 0, 1, 2 });
    constexpr size_t     indexBufferLength = indices.size() * sizeof(uint16_t);

    m_vertexBuffer = NS::TransferPtr(device()->newBuffer(
        vertices.data(), vertexBufferLength, MTL::ResourceCPUCacheModeDefaultCache));
    m_vertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    m_indexBuffer = NS::TransferPtr(device()->newBuffer(
        indices.data(), indexBufferLength, MTL::ResourceOptionCPUCacheModeDefault));
    m_indexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    for (auto index = 0; std::cmp_less(index, s_bufferCount); index++)
    {
        const auto                      label = fmt::format("Uniform: {}", index);
        const NS::SharedPtr<NS::String> nsLabel
            = NS::TransferPtr(NS::String::string(label.c_str(), NS::ASCIIStringEncoding));
        m_uniformBuffer[index] = NS::TransferPtr(device()->newBuffer(
            g_alignedUniformSize * s_bufferCount, MTL::ResourceOptionCPUCacheModeDefault));
        m_uniformBuffer[index]->setLabel(nsLabel.get());
    }
}

void HelloWorld::updateUniforms() const
{
    const auto currentFrameIndex = frameIndex();

    auto position = Vector3(0.0F, 0.0, -10.0F);
    auto rotationX = 0.0F;
    auto rotationY = m_rotationY;
    auto scaleFactor = 3.0F;

    const Vector3 xAxis = Vector3::Right;
    const Vector3 yAxis = Vector3::Up;

    const Matrix         xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
    const Matrix         yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
    const Matrix         rotation = xRot * yRot;
    const Matrix         translation = Matrix::CreateTranslation(position);
    const Matrix         scale = Matrix::CreateScale(scaleFactor);
    const Matrix         model = scale * rotation * translation;
    const CameraUniforms cameraUniforms = m_mainCamera->uniforms();

    Uniforms uniforms {};
    uniforms.modelViewProjection = model * cameraUniforms.viewProjection;

    const size_t uniformBufferOffset = g_alignedUniformSize * currentFrameIndex;

    auto* buffer = static_cast<char*>(this->m_uniformBuffer[currentFrameIndex]->contents());
    memcpy(buffer + uniformBufferOffset, &uniforms, sizeof(uniforms));
}

int main(const int argc, char** argv)
{
    int result = EXIT_FAILURE;
    try
    {
        const auto example = std::make_unique<HelloWorld>();
        result = example->run(argc, argv);
    }
    catch (const std::runtime_error&)
    {
        fmt::println("Exiting...");
    }

    return result;
}
