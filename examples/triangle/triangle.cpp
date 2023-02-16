
#include <memory>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "Example.hpp"
#include "graphics_math.h"

using namespace DirectX;

XM_ALIGNED_STRUCT(16) Vertex
{
    XMFLOAT4 position;
    XMFLOAT4 color;
};

XM_ALIGNED_STRUCT(16) Uniforms
{
    XMMATRIX modelViewProj;
};

class Triangle : public Example
{
    static constexpr int BUFFER_COUNT = 3;
public:
    Triangle();
    
    ~Triangle() override;
    
    bool Load() override;
    
    void Update(float elapsed) override;
    
    void Render(float elapsed) override;
    
private:
    void CreateDepthStencil();
    
    void CreateBuffers();
    
    void CreatePipelineState();
    
    NS::SharedPtr<MTL::Device> Device;
    NS::SharedPtr<MTL::CommandQueue> CommandQueue;
    NS::SharedPtr<MTL::Texture> DepthStencilTexture;
    NS::SharedPtr<MTL::DepthStencilState> DepthStencilState;
    NS::SharedPtr<MTL::RenderPipelineState> PipelineState;
    NS::SharedPtr<MTL::Library> PipelineLibrary;
    NS::SharedPtr<MTL::Buffer> VertexBuffer;
    NS::SharedPtr<MTL::Buffer> IndexBuffer;
    NS::SharedPtr<MTL::Buffer> UniformBuffer[BUFFER_COUNT];
    
    uint32_t FrameIndex = 0;
    dispatch_semaphore_t FrameSemaphore;
};

Triangle::Triangle()
 : Example("Triangle", 800, 600)
{
    
}

Triangle::~Triangle()
{
    
}

bool Triangle::Load()
{
    Device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

    CA::MetalLayer* layer = (CA::MetalLayer*)(SDL_Metal_GetLayer(View));
    layer->setDevice(Device.get());
    
    CommandQueue = NS::TransferPtr(Device->newCommandQueue());
    
    CreateDepthStencil();
    
    CreateBuffers();
    
    CreatePipelineState();
    
    FrameSemaphore = dispatch_semaphore_create(BUFFER_COUNT);
    
    return true;
}

void Triangle::Update(float elapsed)
{
    
}

void Triangle::Render(float elapsed)
{
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    
    FrameIndex = (FrameIndex + 1) % BUFFER_COUNT;
    
    MTL::CommandBuffer* commandBuffer = CommandQueue->commandBuffer();
    
    dispatch_semaphore_wait(FrameSemaphore, DISPATCH_TIME_FOREVER);
    commandBuffer->addCompletedHandler([this](MTL::CommandBuffer* buffer) {
        dispatch_semaphore_signal(FrameSemaphore);
    });
    
    CA::MetalLayer* layer = (CA::MetalLayer*)(SDL_Metal_GetLayer(View));
    CA::MetalDrawable* drawable = layer->nextDrawable();
    if (drawable)
    {
        auto texture = drawable->texture();
        
        MTL::RenderPassDescriptor* passDescriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
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
        
        MTL::RenderCommandEncoder* commandEncoder = commandBuffer->renderCommandEncoder(passDescriptor);
        commandEncoder->setRenderPipelineState(PipelineState.get());
        commandEncoder->setDepthStencilState(DepthStencilState.get());
        commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        commandEncoder->setCullMode(MTL::CullModeNone);
        
        // TODO: draw something
        
        commandEncoder->endEncoding();
        
        commandBuffer->presentDrawable(drawable);
        commandBuffer->commit();
    }
    
    pool->release();
}

void Triangle::CreateDepthStencil()
{
    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);
    
    DepthStencilState = NS::TransferPtr(Device->newDepthStencilState(depthStencilDescriptor));
    
    depthStencilDescriptor->release();
    
    
    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float_Stencil8, GetFrameWidth(), GetFrameHeight(), false);
    textureDescriptor->setSampleCount(1);
    textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    textureDescriptor->setResourceOptions(MTL::ResourceOptionCPUCacheModeDefault | MTL::ResourceStorageModePrivate);
    textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);
    
    DepthStencilTexture = NS::TransferPtr(Device->newTexture(textureDescriptor));
    
    textureDescriptor->release();
}

void Triangle::CreatePipelineState()
{
    PipelineLibrary = NS::TransferPtr(Device->newDefaultLibrary());
    
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
    
    MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    pipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setVertexFunction(PipelineLibrary->newFunction(NS::String::string("vertex_project", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(PipelineLibrary->newFunction(NS::String::string("fragment_flatcolor", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    
    NS::Error* error;
    PipelineState = NS::TransferPtr(Device->newRenderPipelineState(pipelineDescriptor, &error));
    if (error)
    {
        fprintf(stderr, "Failed to create pipeline state object: %s\n", error->description()->cString(NS::ASCIIStringEncoding));
    }
    
    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void Triangle::CreateBuffers()
{
    static const Vertex vertices[] = {
        { .position = { 0, 1, 0, 1 }, .color = { 1, 0, 0, 1 }},
        { .position = { -1, -1, 0, 1 }, .color = { 0, 1, 0, 1 }},
        { .position = { 1, -1, 0, 1 }, .color = { 0, 0, 1, 1 }}};

    static const uint16_t indices[] = { 0, 1, 2 };
    
    
    VertexBuffer = NS::TransferPtr(Device->newBuffer(vertices, sizeof(vertices), MTL::ResourceCPUCacheModeDefaultCache));
    VertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));
    
    IndexBuffer = NS::TransferPtr(Device->newBuffer(indices, sizeof(indices), MTL::ResourceOptionCPUCacheModeDefault));
    IndexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));
    
    const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
    NS::String* prefix = NS::String::string("Unform: ", NS::ASCIIStringEncoding);
    for (auto index = 0; index < BUFFER_COUNT; index++)
    {
        char temp[12];
        snprintf(temp, sizeof(temp), "%d", index);
        
        UniformBuffer[index] = NS::TransferPtr(Device->newBuffer(alignedUniformSize * BUFFER_COUNT, MTL::ResourceOptionCPUCacheModeDefault));
        UniformBuffer[index]->setLabel(prefix->stringByAppendingString(NS::String::string(temp, NS::ASCIIStringEncoding)));
    }
    prefix->release();
}


#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
    std::unique_ptr<Triangle> example = std::make_unique<Triangle>();
    return example->Run(argc, argv);
}


