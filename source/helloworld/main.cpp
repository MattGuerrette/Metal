
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "Camera.hpp"
#include "Example.hpp"

using namespace DirectX;

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector4 Position;
    Vector4 Color;
};

XM_ALIGNED_STRUCT(16) Uniforms
{
    [[maybe_unused]] Matrix ModelViewProjection;
};

class HelloWorld : public Example
{
public:
    HelloWorld();

    ~HelloWorld() override;

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
    NS::SharedPtr<MTL::Buffer>              UniformBuffer[BufferCount];
    std::unique_ptr<Camera>                 MainCamera;

    float RotationY = 0.0f;
};

HelloWorld::HelloWorld() : Example("Hello, Metal", 800, 600)
{
}

HelloWorld::~HelloWorld() = default;

bool HelloWorld::Load()
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

void HelloWorld::Update(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.GetElapsedSeconds());

    RotationY += elapsed;
}

void HelloWorld::Render(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer)
{
    UpdateUniforms();

    const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
    const auto   uniformBufferOffset = alignedUniformSize * FrameIndex;

    commandEncoder->setRenderPipelineState(PipelineState.get());
    commandEncoder->setDepthStencilState(DepthStencilState.get());
    commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setVertexBuffer(VertexBuffer.get(), 0, 0);
    commandEncoder->setVertexBuffer(UniformBuffer[FrameIndex].get(), uniformBufferOffset, 1);

    commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, IndexBuffer->length() / sizeof(uint16_t),
                                          MTL::IndexTypeUInt16, IndexBuffer.get(), 0);
}

void HelloWorld::CreatePipelineState()
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

    MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    pipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
        MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(
        MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setVertexFunction(
        PipelineLibrary->newFunction(NS::String::string("triangle_vertex", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(
        PipelineLibrary->newFunction(NS::String::string("triangle_fragment", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);

    NS::Error* error = nullptr;
    PipelineState = NS::TransferPtr(Device->newRenderPipelineState(pipelineDescriptor, &error));
    if (error)
    {
        fprintf(stderr, "Failed to create pipeline state object: %s\n",
                error->description()->cString(NS::ASCIIStringEncoding));
        abort();
    }

    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void HelloWorld::CreateBuffers()
{
    static const Vertex vertices[] = {{.Position = {0, 1, 0, 1}, .Color = {1, 0, 0, 1}},
                                      {.Position = {-1, -1, 0, 1}, .Color = {0, 1, 0, 1}},
                                      {.Position = {1, -1, 0, 1}, .Color = {0, 0, 1, 1}}};

    static const uint16_t indices[] = {0, 1, 2};

    VertexBuffer =
        NS::TransferPtr(Device->newBuffer(vertices, sizeof(vertices), MTL::ResourceCPUCacheModeDefaultCache));
    VertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    IndexBuffer = NS::TransferPtr(Device->newBuffer(indices, sizeof(indices), MTL::ResourceOptionCPUCacheModeDefault));
    IndexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
    NS::String*  prefix = NS::String::string("Uniform: ", NS::ASCIIStringEncoding);
    for (auto index = 0; index < BufferCount; index++)
    {
        char temp[12];
        snprintf(temp, sizeof(temp), "%d", index);

        UniformBuffer[index] = NS::TransferPtr(
            Device->newBuffer(alignedUniformSize * BufferCount, MTL::ResourceOptionCPUCacheModeDefault));
        UniformBuffer[index]->setLabel(
            prefix->stringByAppendingString(NS::String::string(temp, NS::ASCIIStringEncoding)));
    }
    prefix->release();
}

void HelloWorld::UpdateUniforms()
{
    auto position = Vector3(0.0f, 0.0, -10.0f);
    auto rotationX = 0.0f;
    auto rotationY = RotationY;
    auto scaleFactor = 3.0f;

    const Vector3 xAxis = Vector3::Right;
    const Vector3 yAxis = Vector3::Up;

    Matrix xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
    Matrix yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
    Matrix rotation = xRot * yRot;

    Matrix translation = Matrix::CreateTranslation(position);
    Matrix scale = Matrix::CreateScale(scaleFactor);
    Matrix model = scale * rotation * translation;

    CameraUniforms cameraUniforms = MainCamera->GetUniforms();

    Uniforms uniforms{};
    uniforms.ModelViewProjection = model * cameraUniforms.ViewProjection;

    const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
    const size_t uniformBufferOffset = alignedUniformSize * FrameIndex;

    char* buffer = reinterpret_cast<char*>(this->UniformBuffer[FrameIndex]->contents());
    memcpy(buffer + uniformBufferOffset, &uniforms, sizeof(uniforms));
}

#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else

int main(int argc, char** argv)
#endif
{
    std::unique_ptr<HelloWorld> example = std::make_unique<HelloWorld>();
    return example->Run(argc, argv);
}
