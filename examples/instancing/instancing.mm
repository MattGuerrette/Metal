
#include "example.h"

#import <Metal/Metal.h>
#include <DirectXColors.h>

#include "camera_x.h"

using namespace DirectX;

inline MTLClearColor XMColorToClearColor(DirectX::XMVECTORF32 color)
{
    return MTLClearColorMake(color.f[0], color.f[1], color.f[2], color.f[3]);
}

typedef XM_ALIGNED_STRUCT(16)
{
    XMFLOAT4 Position;
    XMFLOAT4 Color;
} Vertex;

typedef XM_ALIGNED_STRUCT(16)
{
    XMMATRIX Transform;
} InstanceData;

class Instancing : public Example
{
    static constexpr int BUFFER_COUNT   = 3;
    static constexpr int INSTANCE_COUNT = 3;
 public:
    Instancing();

    ~Instancing();

    void Init() override;

    void Update(double elapsed) override;

    void Render(double elapsed) override;

 private:
    std::shared_ptr<class Camera> Camera;

    id<MTLDevice>              Device{};
    id<MTLCommandQueue>        CommandQueue{};
    id<MTLDepthStencilState>   DepthStencilState{};
    id<MTLTexture>             DepthStencilTexture;
    id<MTLBuffer>              VertexBuffer{};
    id<MTLBuffer>              IndexBuffer{};
    id<MTLBuffer>              InstanceBuffer[BUFFER_COUNT];
    id<MTLRenderPipelineState> PipelineState{};
    id<MTLLibrary>             PipelineLibrary{};
    NSUInteger                 FrameIndex{};
    dispatch_semaphore_t       Semaphore{};

    void MakeBuffers();

    void UpdateUniform();

    MTLVertexDescriptor* CreateVertexDescriptor();

    float RotationY = 0.0f;
    float RotationX = 0.0f;
};

Instancing::Instancing() : Example("Instancing", 1280, 720)
{

}

Instancing::~Instancing() = default;

void Instancing::Init()
{
    Device = MTLCreateSystemDefaultDevice();

    // Metal initialization
    CAMetalLayer* layer = GetMetalLayer();
    layer.device = Device;

    CommandQueue = [Device newCommandQueue];

    MTLDepthStencilDescriptor* depthStencilDesc =
                                 [MTLDepthStencilDescriptor new];
    depthStencilDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStencilDesc.depthWriteEnabled    = true;
    DepthStencilState =
        [Device newDepthStencilStateWithDescriptor:depthStencilDesc];

    // Create depth and stencil textures
    uint32_t width  = static_cast<uint32_t>([layer drawableSize].width);
    uint32_t height = static_cast<uint32_t>([layer drawableSize].height);
    MTLTextureDescriptor* depthStencilTexDesc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                     width:width
                                    height:height
                                 mipmapped:NO];
    depthStencilTexDesc.sampleCount = 1;
    depthStencilTexDesc.usage       = MTLTextureUsageRenderTarget;
    depthStencilTexDesc.resourceOptions =
        MTLResourceOptionCPUCacheModeDefault | MTLResourceStorageModePrivate;
    depthStencilTexDesc.storageMode = MTLStorageModeMemoryless;

    DepthStencilTexture =
        [Device newTextureWithDescriptor:depthStencilTexDesc];

    MakeBuffers();

    NSString* libraryPath = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent:@"shader.metallib"];
    NSError * error       = nil;
    PipelineLibrary = [Device newLibraryWithFile:libraryPath error:&error];
    MTLRenderPipelineDescriptor* pipelineDescriptor =
                                   [MTLRenderPipelineDescriptor new];

    pipelineDescriptor.colorAttachments[0].pixelFormat     = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor        =
        MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor   =
        MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation           =
        MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor      =
        MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation         =
        MTLBlendOperationAdd;

    pipelineDescriptor.depthAttachmentPixelFormat   =
        MTLPixelFormatDepth32Float_Stencil8;
    pipelineDescriptor.stencilAttachmentPixelFormat =
        MTLPixelFormatDepth32Float_Stencil8;
    pipelineDescriptor.vertexFunction   =
        [PipelineLibrary newFunctionWithName:@"vertex_project"];
    pipelineDescriptor.fragmentFunction =
        [PipelineLibrary newFunctionWithName:@"fragment_flatcolor"];
    pipelineDescriptor.vertexDescriptor = CreateVertexDescriptor();

    PipelineState =
        [Device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                               error:&error];
    if (!PipelineState)
    {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }
    
    const CGSize drawableSize = layer.drawableSize;
    const float  aspect       = (float)drawableSize.width / (float)drawableSize.height;
    const float  fov          = XMConvertToRadians(75);
    const float  near         = 0.01f;
    const float  far          = 1000.0f;

    Camera = std::make_shared<class Camera>(
        XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f },
        XMFLOAT3{ 0.0f, 1.0f, 0.0f }, fov, aspect, near, far);

    Semaphore = dispatch_semaphore_create(BUFFER_COUNT);
}

MTLVertexDescriptor* Instancing::CreateVertexDescriptor()
{
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];

    // Position
    vertexDescriptor.attributes[0].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[0].offset      = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;

    // Texture coordinates
    vertexDescriptor.attributes[1].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[1].offset      = sizeof(XMFLOAT4);
    vertexDescriptor.attributes[1].bufferIndex = 0;

    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride       = sizeof(Vertex);

    return vertexDescriptor;
}

void Instancing::UpdateUniform()
{
    id<MTLBuffer> instanceBuffer = InstanceBuffer[FrameIndex];

    InstanceData* pInstanceData = reinterpret_cast< InstanceData*>( [instanceBuffer contents] );
    for (auto index = 0; index < INSTANCE_COUNT; ++index)
    {
        auto translation = XMFLOAT3(-5.0f + (index * 5.0f), 0.0f, -10.0f);
        auto rotationX   = RotationX;
        auto rotationY   = RotationY;
        auto scaleFactor = 1.0f;

        const XMFLOAT3 xAxis = { 1, 0, 0 };
        const XMFLOAT3 yAxis = { 0, 1, 0 };

        XMVECTOR xRotAxis = XMLoadFloat3(&xAxis);
        XMVECTOR yRotAxis = XMLoadFloat3(&yAxis);

        XMMATRIX xRot        = XMMatrixRotationAxis(xRotAxis, rotationX);
        XMMATRIX yRot        = XMMatrixRotationAxis(yRotAxis, rotationY);
        XMMATRIX rot         = XMMatrixMultiply(xRot, yRot);
        XMMATRIX trans       =
                     XMMatrixTranslation(translation.x, translation.y, translation.z);
        XMMATRIX scale       = XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);
        XMMATRIX modelMatrix = XMMatrixMultiply(scale, XMMatrixMultiply(rot, trans));

        CameraUniforms cameraUniforms = Camera->GetUniforms();

        pInstanceData[index].Transform = modelMatrix * cameraUniforms.ViewProjection;
    }
}

void Instancing::MakeBuffers()
{
    static const Vertex vertices[] = {
        { .Position = { -1, 1, 1, 1 }, .Color = { 0, 1, 1, 1 }},
        { .Position = { -1, -1, 1, 1 }, .Color = { 0, 0, 1, 1 }},
        { .Position = { 1, -1, 1, 1 }, .Color = { 1, 0, 1, 1 }},
        { .Position = { 1, 1, 1, 1 }, .Color = { 1, 1, 1, 1 }},
        { .Position = { -1, 1, -1, 1 }, .Color = { 0, 1, 0, 1 }},
        { .Position = { -1, -1, -1, 1 }, .Color = { 0, 0, 0, 1 }},
        { .Position = { 1, -1, -1, 1 }, .Color = { 1, 0, 0, 1 }},
        { .Position = { 1, 1, -1, 1 }, .Color = { 1, 1, 0, 1 }}};

    static const uint16_t indices[] = { 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4,
                                        4, 0, 3, 3, 7, 4, 1, 5, 6, 6, 2, 1,
                                        0, 1, 2, 2, 3, 0, 7, 6, 5, 5, 4, 7 };

    VertexBuffer =
        [Device newBufferWithBytes:vertices
                            length:sizeof(vertices)
                           options:MTLResourceOptionCPUCacheModeDefault];
    [VertexBuffer setLabel:@"Vertices"];

    IndexBuffer =
        [Device newBufferWithBytes:indices
                            length:sizeof(indices)
                           options:MTLResourceOptionCPUCacheModeDefault];
    [IndexBuffer setLabel:@"Indices"];

    const size_t instanceDataSize = BUFFER_COUNT * INSTANCE_COUNT * sizeof(InstanceData);
    for (auto    index            = 0; index < BUFFER_COUNT; ++index)
    {
        InstanceBuffer[index] = [Device newBufferWithLength:instanceDataSize options:MTLResourceOptionCPUCacheModeDefault];
        NSString* label = [NSString stringWithFormat:@"InstanceBuffer: %d", index];
        [InstanceBuffer[index] setLabel:label];
    }

}

void Instancing::Update(double elapsed)
{
    RotationX += elapsed;
    RotationY += elapsed;
}

void Instancing::Render(double elapsed)
{

    @autoreleasepool
    {
        FrameIndex = (FrameIndex + 1) % BUFFER_COUNT;

        id<MTLBuffer> instanceBuffer = InstanceBuffer[FrameIndex];

        id<MTLCommandBuffer> commandBuffer = [CommandQueue commandBuffer];
        dispatch_semaphore_wait(Semaphore, DISPATCH_TIME_FOREVER);
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull)
        {
          dispatch_semaphore_signal(Semaphore);
        }];

        UpdateUniform();

        CAMetalLayer* layer = GetMetalLayer();
        id<CAMetalDrawable> drawable = [layer nextDrawable];
        if (drawable != nil)
        {
            id<MTLTexture> texture = drawable.texture;

            MTLRenderPassDescriptor* passDesc =
                                       [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture     = texture;
            passDesc.colorAttachments[0].loadAction  = MTLLoadActionClear;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            passDesc.colorAttachments[0].clearColor = XMColorToClearColor(DirectX::Colors::CornflowerBlue);
               // MTLClearColorMake(.39, .58, .92, 1.0);
            passDesc.depthAttachment.texture        = DepthStencilTexture;
            passDesc.depthAttachment.loadAction     = MTLLoadActionClear;
            passDesc.depthAttachment.storeAction    = MTLStoreActionDontCare;
            passDesc.depthAttachment.clearDepth     = 1.0;
            passDesc.stencilAttachment.texture      = DepthStencilTexture;
            passDesc.stencilAttachment.loadAction   = MTLLoadActionClear;
            passDesc.stencilAttachment.storeAction  = MTLStoreActionDontCare;
            passDesc.stencilAttachment.clearStencil = 0;

            id<MTLRenderCommandEncoder> commandEncoder =
                                            [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            [commandEncoder setRenderPipelineState:PipelineState];
            [commandEncoder setDepthStencilState:DepthStencilState];
            [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
            [commandEncoder setCullMode:MTLCullModeBack];
            [commandEncoder setVertexBuffer:VertexBuffer offset:0 atIndex:0];
            [commandEncoder setVertexBuffer:instanceBuffer offset:0 atIndex:1];

            [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:
                    [IndexBuffer length]
                    / sizeof(uint16_t)  indexType:MTLIndexTypeUInt16
                                      indexBuffer:IndexBuffer indexBufferOffset:0
                                    instanceCount:INSTANCE_COUNT];
            [commandEncoder endEncoding];

            [commandBuffer presentDrawable:drawable];

            [commandBuffer commit];
        }
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCInconsistentNamingInspection"

#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
    auto* example = new Instancing;

    return example->Run(argc, argv);
}

#pragma clang diagnostic pop
