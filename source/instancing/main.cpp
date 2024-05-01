
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include <fmt/core.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "Camera.hpp"
#include "Example.hpp"

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector4 Position;
    Vector4 Color;
};

XM_ALIGNED_STRUCT(16) Uniforms
{
    [[maybe_unused]] Matrix ModelViewProjection;
};

XM_ALIGNED_STRUCT(16) InstanceData
{
    Matrix Transform;
};

class Instancing : public Example
{
    static constexpr int InstanceCount = 3;

public:
    Instancing();

    ~Instancing() override;

    bool Load() override;

    void Update(const GameTimer& timer) override;

    void Render(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) override;

private:
    void CreateBuffers();

    void CreatePipelineState();

    void UpdateUniforms();

    NS::SharedPtr<MTL::RenderPipelineState> PipelineState;
    NS::SharedPtr<MTL::Buffer>              VertexBuffer;
    NS::SharedPtr<MTL::Buffer>              IndexBuffer;
    NS::SharedPtr<MTL::Buffer>              InstanceBuffer[BufferCount];
    std::unique_ptr<Camera>                 MainCamera;

    float RotationX = 0.0f;
    float RotationY = 0.0f;
};

Instancing::Instancing() : Example("Instancing", 800, 600)
{
}

Instancing::~Instancing() = default;

bool Instancing::Load()
{
    const auto  width = GetFrameWidth();
    const auto  height = GetFrameHeight();
    const float aspect = (float)width / (float)height;
    const float fov = (75.0f * (float)M_PI) / 180.0f;
    const float near = 0.01f;
    const float far = 1000.0f;

    MainCamera = std::make_unique<Camera>(XMFLOAT3{0.0f, 0.0f, 0.0f}, XMFLOAT3{0.0f, 0.0f, -1.0f},
                                          XMFLOAT3{0.0f, 1.0f, 0.0f}, fov, aspect, near, far);

    CreateBuffers();

    CreatePipelineState();

    return true;
}

void Instancing::Update(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.GetElapsedSeconds());

    RotationX += elapsed;
    RotationY += elapsed;
}

void Instancing::Render(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer)
{
    UpdateUniforms();

    commandEncoder->setRenderPipelineState(PipelineState.get());
    commandEncoder->setDepthStencilState(DepthStencilState.get());
    commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setVertexBuffer(VertexBuffer.get(), 0, 0);
    commandEncoder->setVertexBuffer(InstanceBuffer[FrameIndex].get(), 0, 1);
    commandEncoder->drawIndexedPrimitives(
        MTL::PrimitiveTypeTriangle, IndexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16,
        IndexBuffer.get(), 0, InstanceCount);
}

void Instancing::CreatePipelineState()
{
    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    // Position
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(0)->setOffset(0);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(0);

    // Color
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(1)->setOffset(offsetof(Vertex, Color));
    vertexDescriptor->attributes()->object(1)->setBufferIndex(0);

    vertexDescriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(Vertex));

    MTL::RenderPipelineDescriptor* pipelineDescriptor =
        MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(FrameBufferPixelFormat);
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
    pipelineDescriptor->setVertexFunction(PipelineLibrary->newFunction(
        NS::String::string("instancing_vertex", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(PipelineLibrary->newFunction(
        NS::String::string("instancing_fragment", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);

    NS::Error* error = nullptr;
    PipelineState = NS::TransferPtr(Device->newRenderPipelineState(pipelineDescriptor, &error));
    if (error)
    {
        throw std::runtime_error(fmt::format("Failed to create pipeline state: {}",
                                             error->localizedFailureReason()->utf8String()));
    }

    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void Instancing::CreateBuffers()
{
    static const Vertex vertices[] = {{.Position = {-1, 1, 1, 1}, .Color = {0, 1, 1, 1}},
                                      {.Position = {-1, -1, 1, 1}, .Color = {0, 0, 1, 1}},
                                      {.Position = {1, -1, 1, 1}, .Color = {1, 0, 1, 1}},
                                      {.Position = {1, 1, 1, 1}, .Color = {1, 1, 1, 1}},
                                      {.Position = {-1, 1, -1, 1}, .Color = {0, 1, 0, 1}},
                                      {.Position = {-1, -1, -1, 1}, .Color = {0, 0, 0, 1}},
                                      {.Position = {1, -1, -1, 1}, .Color = {1, 0, 0, 1}},
                                      {.Position = {1, 1, -1, 1}, .Color = {1, 1, 0, 1}}};

    static const uint16_t indices[] = {3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4, 4, 0, 3, 3, 7, 4,
                                       1, 5, 6, 6, 2, 1, 0, 1, 2, 2, 3, 0, 7, 6, 5, 5, 4, 7};

    VertexBuffer = NS::TransferPtr(
        Device->newBuffer(vertices, sizeof(vertices), MTL::ResourceCPUCacheModeDefaultCache));
    VertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    IndexBuffer = NS::TransferPtr(
        Device->newBuffer(indices, sizeof(indices), MTL::ResourceOptionCPUCacheModeDefault));
    IndexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    const size_t instanceDataSize = BufferCount * InstanceCount * sizeof(InstanceData);
    NS::String*  prefix = NS::String::string("Instance Buffer: ", NS::ASCIIStringEncoding);
    for (auto index = 0; index < BufferCount; index++)
    {
        char temp[12];
        snprintf(temp, sizeof(temp), "%d", index);

        InstanceBuffer[index] = NS::TransferPtr(
            Device->newBuffer(instanceDataSize, MTL::ResourceOptionCPUCacheModeDefault));
        InstanceBuffer[index]->setLabel(
            prefix->stringByAppendingString(NS::String::string(temp, NS::ASCIIStringEncoding)));
    }
    prefix->release();
}

void Instancing::UpdateUniforms()
{
    MTL::Buffer* instanceBuffer = InstanceBuffer[FrameIndex].get();

    auto* instanceData = static_cast<InstanceData*>(instanceBuffer->contents());
    for (auto index = 0; index < InstanceCount; ++index)
    {
        auto position = Vector3(-5.0f + (5.0f * (float)index), 0.0f, -10.0f);
        auto rotationX = RotationX;
        auto rotationY = RotationY;
        auto scaleFactor = 1.0f;

        const Vector3 xAxis = Vector3::Right;
        const Vector3 yAxis = Vector3::Up;

        Matrix xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
        Matrix yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
        Matrix rotation = xRot * yRot;

        Matrix         translation = Matrix::CreateTranslation(position);
        Matrix         scale = Matrix::CreateScale(scaleFactor);
        Matrix         model = scale * rotation * translation;
        CameraUniforms cameraUniforms = MainCamera->GetUniforms();

        instanceData[index].Transform = model * cameraUniforms.ViewProjection;
    }
}

#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else

int main(int argc, char** argv)
#endif
{
    int result = EXIT_FAILURE;
    try
    {
        auto example = std::make_unique<Instancing>();
        result = example->Run(argc, argv);
    }
    catch (const std::runtime_error& error)
    {
        fmt::println("Exiting...");
    }

    return result;
}
