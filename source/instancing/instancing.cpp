
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "Example.hpp"
#include "Camera.hpp"

using namespace DirectX;

XM_ALIGNED_STRUCT(16) Vertex {
    XMFLOAT4 position;
    XMFLOAT4 color;
};

XM_ALIGNED_STRUCT(16) Uniforms {
    [[maybe_unused]] XMMATRIX modelViewProj;
};

XM_ALIGNED_STRUCT(16) InstanceData {
    XMMATRIX transform;
};

class Instancing : public Example {
    static constexpr int BUFFER_COUNT = 3;
    static constexpr int INSTANCE_COUNT = 3;
public:
    Instancing();

    ~Instancing() override;

    bool Load() override;

    void Update(float elapsed) override;

    void Render(float elapsed) override;

private:
    void CreateDepthStencil();

    void CreateBuffers();

    void CreatePipelineState();

    void UpdateUniforms();

    NS::SharedPtr<MTL::Device> Device;
    NS::SharedPtr<MTL::CommandQueue> CommandQueue;
    NS::SharedPtr<MTL::Texture> DepthStencilTexture;
    NS::SharedPtr<MTL::DepthStencilState> DepthStencilState;
    NS::SharedPtr<MTL::RenderPipelineState> PipelineState;
    NS::SharedPtr<MTL::Library> PipelineLibrary;
    NS::SharedPtr<MTL::Buffer> VertexBuffer;
    NS::SharedPtr<MTL::Buffer> IndexBuffer;
    NS::SharedPtr<MTL::Buffer> InstanceBuffer[BUFFER_COUNT];
    std::unique_ptr<Camera> MainCamera;

    uint32_t FrameIndex = 0;
    dispatch_semaphore_t FrameSemaphore;
    float RotationX = 0.0f;
    float RotationY = 0.0f;
};

Instancing::Instancing()
        : Example("Instancing", 800, 600) {

}

Instancing::~Instancing() {

}

bool Instancing::Load() {
    Device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

    auto *layer = static_cast<CA::MetalLayer *>((SDL_Metal_GetLayer(View)));
    layer->setDevice(Device.get());

    const auto width = GetFrameWidth();
    const auto height = GetFrameHeight();
    const float aspect = (float) width / (float) height;
    const float fov = (75.0f * (float) M_PI) / 180.0f;
    const float near = 0.01f;
    const float far = 1000.0f;

    MainCamera = std::make_unique<Camera>(XMFLOAT3{0.0f, 0.0f, 0.0f},
                                          XMFLOAT3{0.0f, 0.0f, -1.0f},
                                          XMFLOAT3{0.0f, 1.0f, 0.0f},
                                          fov, aspect, near, far);

    CommandQueue = NS::TransferPtr(Device->newCommandQueue());

    CreateDepthStencil();

    CreateBuffers();

    CreatePipelineState();

    FrameSemaphore = dispatch_semaphore_create(BUFFER_COUNT);

    return true;
}

void Instancing::Update(float elapsed) {
    RotationX += elapsed;
    RotationY += elapsed;
}

void Instancing::Render(float elapsed) {
    NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();

    FrameIndex = (FrameIndex + 1) % BUFFER_COUNT;

    MTL::CommandBuffer *commandBuffer = CommandQueue->commandBuffer();

    dispatch_semaphore_wait(FrameSemaphore, DISPATCH_TIME_FOREVER);
    commandBuffer->addCompletedHandler([this](MTL::CommandBuffer *buffer) {
        dispatch_semaphore_signal(FrameSemaphore);
    });

    UpdateUniforms();

    CA::MetalLayer *layer = (CA::MetalLayer *) (SDL_Metal_GetLayer(View));
    CA::MetalDrawable *drawable = layer->nextDrawable();
    if (drawable) {
        auto texture = drawable->texture();

        MTL::RenderPassDescriptor *passDescriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
        passDescriptor->colorAttachments()->object(0)->setTexture(texture);
        passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
        passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(.39, .58, .92, 1.0));
        passDescriptor->depthAttachment()->setTexture(DepthStencilTexture.get());
        passDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
        passDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
        passDescriptor->depthAttachment()->setClearDepth(1.0);
        passDescriptor->stencilAttachment()->setTexture(DepthStencilTexture.get());
        passDescriptor->stencilAttachment()->setLoadAction(MTL::LoadActionClear);
        passDescriptor->stencilAttachment()->setStoreAction(MTL::StoreActionDontCare);
        passDescriptor->stencilAttachment()->setClearStencil(0);

        MTL::RenderCommandEncoder *commandEncoder = commandBuffer->renderCommandEncoder(passDescriptor);
        commandEncoder->setRenderPipelineState(PipelineState.get());
        commandEncoder->setDepthStencilState(DepthStencilState.get());
        commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        commandEncoder->setCullMode(MTL::CullModeNone);

        const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
        const auto uniformBufferOffset =
                alignedUniformSize * FrameIndex;

        commandEncoder->setVertexBuffer(VertexBuffer.get(), 0, 0);
        commandEncoder->setVertexBuffer(InstanceBuffer[FrameIndex].get(), 0, 1);


        commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
                                              IndexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16,
                                              IndexBuffer.get(), 0, INSTANCE_COUNT);

        commandEncoder->endEncoding();

        commandBuffer->presentDrawable(drawable);
        commandBuffer->commit();
    }

    pool->release();
}

void Instancing::CreateDepthStencil() {
    MTL::DepthStencilDescriptor *depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);

    DepthStencilState = NS::TransferPtr(Device->newDepthStencilState(depthStencilDescriptor));

    depthStencilDescriptor->release();


    MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatDepth32Float_Stencil8, GetFrameWidth(), GetFrameHeight(), false);
    textureDescriptor->setSampleCount(1);
    textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    textureDescriptor->setResourceOptions(MTL::ResourceOptionCPUCacheModeDefault | MTL::ResourceStorageModePrivate);
    textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);

    DepthStencilTexture = NS::TransferPtr(Device->newTexture(textureDescriptor));

    textureDescriptor->release();
}

void Instancing::CreatePipelineState() {
    CFBundleRef main_bundle = CFBundleGetMainBundle();
    CFURLRef url = CFBundleCopyResourcesDirectoryURL(main_bundle);
    char cwd[PATH_MAX];
    CFURLGetFileSystemRepresentation(url, TRUE, (UInt8 *) cwd, PATH_MAX);
    CFRelease(url);
    auto path = NS::Bundle::mainBundle()->resourcePath();
    NS::String *library = path->stringByAppendingString(
            NS::String::string("/default.metallib", NS::ASCIIStringEncoding));
    NS::Error *error = nullptr;

    auto std_str = library->utf8String();
    PipelineLibrary = NS::TransferPtr(Device->newLibrary(library, &error));

    MTL::VertexDescriptor *vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

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

    MTL::RenderPipelineDescriptor *pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
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
            PipelineLibrary->newFunction(NS::String::string("instancing_vertex", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(
            PipelineLibrary->newFunction(NS::String::string("instancing_fragment", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);

    PipelineState = NS::TransferPtr(Device->newRenderPipelineState(pipelineDescriptor, &error));
    if (error) {
        fprintf(stderr, "Failed to create pipeline state object: %s\n",
                error->description()->cString(NS::ASCIIStringEncoding));
    }

    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void Instancing::CreateBuffers() {
    static const Vertex vertices[] = {
            {.position = {-1, 1, 1, 1}, .color = {0, 1, 1, 1}},
            {.position = {-1, -1, 1, 1}, .color = {0, 0, 1, 1}},
            {.position = {1, -1, 1, 1}, .color = {1, 0, 1, 1}},
            {.position = {1, 1, 1, 1}, .color = {1, 1, 1, 1}},
            {.position = {-1, 1, -1, 1}, .color = {0, 1, 0, 1}},
            {.position = {-1, -1, -1, 1}, .color = {0, 0, 0, 1}},
            {.position = {1, -1, -1, 1}, .color = {1, 0, 0, 1}},
            {.position = {1, 1, -1, 1}, .color = {1, 1, 0, 1}}};

    static const uint16_t indices[] = {3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4,
                                       4, 0, 3, 3, 7, 4, 1, 5, 6, 6, 2, 1,
                                       0, 1, 2, 2, 3, 0, 7, 6, 5, 5, 4, 7};


    VertexBuffer = NS::TransferPtr(
            Device->newBuffer(vertices, sizeof(vertices), MTL::ResourceCPUCacheModeDefaultCache));
    VertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    IndexBuffer = NS::TransferPtr(Device->newBuffer(indices, sizeof(indices), MTL::ResourceOptionCPUCacheModeDefault));
    IndexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    const size_t instanceDataSize = BUFFER_COUNT * INSTANCE_COUNT * sizeof(InstanceData);
    NS::String *prefix = NS::String::string("Instance Buffer: ", NS::ASCIIStringEncoding);
    for (auto index = 0; index < BUFFER_COUNT; index++) {
        char temp[12];
        snprintf(temp, sizeof(temp), "%d", index);

        InstanceBuffer[index] = NS::TransferPtr(
                Device->newBuffer(instanceDataSize, MTL::ResourceOptionCPUCacheModeDefault));
        InstanceBuffer[index]->setLabel(
                prefix->stringByAppendingString(NS::String::string(temp, NS::ASCIIStringEncoding)));
    }
    prefix->release();
}

void Instancing::UpdateUniforms() {
    MTL::Buffer *instanceBuffer = InstanceBuffer[FrameIndex].get();

    InstanceData *instanceData = (InstanceData *) instanceBuffer->contents();
    for (auto index = 0; index < INSTANCE_COUNT; ++index) {
        auto translation = XMFLOAT3(-5.0f + (index * 5.0f), 0.0f, -10.0f);
        auto rotationX = RotationX;
        auto rotationY = RotationY;
        auto scaleFactor = 1.0f;

        const XMFLOAT3 xAxis = {1, 0, 0};
        const XMFLOAT3 yAxis = {0, 1, 0};

        XMVECTOR xRotAxis = XMLoadFloat3(&xAxis);
        XMVECTOR yRotAxis = XMLoadFloat3(&yAxis);

        XMMATRIX xRot = XMMatrixRotationAxis(xRotAxis, rotationX);
        XMMATRIX yRot = XMMatrixRotationAxis(yRotAxis, rotationY);
        XMMATRIX rot = XMMatrixMultiply(xRot, yRot);
        XMMATRIX trans =
                XMMatrixTranslation(translation.x, translation.y, translation.z);
        XMMATRIX scale = XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);
        XMMATRIX modelMatrix = XMMatrixMultiply(scale, XMMatrixMultiply(rot, trans));

        CameraUniforms cameraUniforms = MainCamera->GetUniforms();

        instanceData[index].transform = modelMatrix * cameraUniforms.viewProjection;
    }
}


#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else

int main(int argc, char **argv)
#endif
{
    std::unique_ptr<Instancing> example = std::make_unique<Instancing>();
    return example->Run(argc, argv);
}


