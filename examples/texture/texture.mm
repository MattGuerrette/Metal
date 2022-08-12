
#include "example.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "camera_x.h"

using namespace DirectX;

typedef XM_ALIGNED_STRUCT(16)
{
    XMFLOAT4 Position;
    XMFLOAT4 Color;
    XMFLOAT2 UV;
} Vertex;

typedef XM_ALIGNED_STRUCT(16)
{
    XMMATRIX ModelViewProj;
} Uniforms;

constexpr NSUInteger kAlignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;

class Texture : public Example
{
 public:
    Texture();

    ~Texture() override;

    void Init() override;

    void Update(double elapsed) override;

    void Render(double elapsed) override;

 private:
    std::shared_ptr<class Camera> Camera;

    id<MTLDevice>              Device{};
    id<MTLCommandQueue>        CommandQueue{};
    id<MTLDepthStencilState>   DepthStencilState{};
    id<MTLTexture>             DepthStencilTexture{};
    id<MTLTexture>             SampleTexture{};
    id<MTLBuffer>              VertexBuffer{};
    id<MTLBuffer>              IndexBuffer{};
    id<MTLBuffer>              UniformBuffer{};
    id<MTLRenderPipelineState> PipelineState{};
    id<MTLLibrary>             PipelineLibrary{};
    NSUInteger                 BufferIndex{};

    void MakeBuffers();

    void UpdateUniform();

    static MTLVertexDescriptor* CreateVertexDescriptor();

    static constexpr int BUFFER_COUNT = 3;

    float RotationY = 0.0f;
    float RotationX = 0.0f;
};

Texture::Texture() : Example("Texture", 1280, 720)
{

}

Texture::~Texture() = default;

void Texture::Init()
{
    Device      = MTLCreateSystemDefaultDevice();
    BufferIndex = 0;

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

    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:Device];

    NSString* texturePath = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent:@"dirt.png"];

    NSURL* url = [[NSURL alloc] initFileURLWithPath:texturePath];

    SampleTexture = [textureLoader newTextureWithContentsOfURL:url options:@{ MTKTextureLoaderOptionSRGB: @NO } error:nil];

    const CGSize drawableSize = layer.drawableSize;
    const float  aspect       = (float)drawableSize.width / (float)drawableSize.height;
    const float  fov          = (75.0f * (float)M_PI) / 180.0f;
    const float  near         = 0.01f;
    const float  far          = 1000.0f;

    Camera = std::make_shared<class Camera>(
        XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f },
        XMFLOAT3{ 0.0f, 1.0f, 0.0f }, fov, aspect, near, far);
}

MTLVertexDescriptor* Texture::CreateVertexDescriptor()
{
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];

    // Position
    vertexDescriptor.attributes[0].format      = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[0].offset      = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;

    // Color
    vertexDescriptor.attributes[1].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[1].offset      = sizeof(XMFLOAT3);//offsetof(Vertex, Position);
    vertexDescriptor.attributes[1].bufferIndex = 0;

    // Texture coordinates
    vertexDescriptor.attributes[2].format      = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[2].offset      = sizeof(XMFLOAT4);//offsetof(Vertex, Color);
    vertexDescriptor.attributes[2].bufferIndex = 0;

    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride       = sizeof(Vertex);

    return vertexDescriptor;
}

void Texture::UpdateUniform()
{
    auto translation = XMFLOAT3(0.0f, 0.0f, -10.0f);
    auto rotationX   = RotationX;
    auto rotationY   = RotationY;
    auto scaleFactor = 5.0f;

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

    Uniforms uniforms;
    auto     viewProj = cameraUniforms.ViewProjection;
    uniforms.ModelViewProj = modelMatrix * viewProj;

    const NSUInteger uniformBufferOffset = kAlignedUniformSize * BufferIndex;

    char* buffer = reinterpret_cast<char*>([UniformBuffer contents]);
    memcpy(buffer + uniformBufferOffset, &uniforms, sizeof(uniforms));
}

void Texture::MakeBuffers()
{
    static const Vertex vertices[] = {
        { .Position = { -1, 1, 0, 1 }, .Color = { 0, 1, 1, 1 }, .UV = { 1, 0 }},
        { .Position = { -1, -1, 0, 1 }, .Color = { 0, 0, 1, 1 }, .UV = { 1, 1 }},
        { .Position = { 1, -1, 0, 1 }, .Color = { 1, 0, 1, 1 }, .UV = { 0, 1 }},
        { .Position = { 1, 1, 0, 1 }, .Color = { 1, 1, 1, 1 }, .UV = { 0, 0 }}};

    static const uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };

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

    UniformBuffer =
        [Device newBufferWithLength:kAlignedUniformSize * BUFFER_COUNT
                            options:MTLResourceOptionCPUCacheModeDefault];
    [UniformBuffer setLabel:@"Uniforms"];
}

void Texture::Update(double elapsed)
{
    RotationX = 0.0f;
    RotationY = 0.0f;
}

void Texture::Render(double elapsed)
{
    UpdateUniform();

    @autoreleasepool
    {
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
            passDesc.colorAttachments[0].clearColor =
                MTLClearColorMake(.39, .58, .92, 1.0);
            passDesc.depthAttachment.texture        = DepthStencilTexture;
            passDesc.depthAttachment.loadAction     = MTLLoadActionClear;
            passDesc.depthAttachment.storeAction    = MTLStoreActionDontCare;
            passDesc.depthAttachment.clearDepth     = 1.0;
            passDesc.stencilAttachment.texture      = DepthStencilTexture;
            passDesc.stencilAttachment.loadAction   = MTLLoadActionClear;
            passDesc.stencilAttachment.storeAction  = MTLStoreActionDontCare;
            passDesc.stencilAttachment.clearStencil = 0;

            id<MTLCommandBuffer>        commandBuffer  = [CommandQueue commandBuffer];
            id<MTLRenderCommandEncoder> commandEncoder =
                                            [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            [commandEncoder setRenderPipelineState:PipelineState];
            [commandEncoder setDepthStencilState:DepthStencilState];
            [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
            [commandEncoder setCullMode:MTLCullModeBack];

            const NSUInteger uniformBufferOffset =
                                 kAlignedUniformSize * BufferIndex;

            [commandEncoder setFragmentTexture:SampleTexture atIndex:0];
            [commandEncoder setVertexBuffer:VertexBuffer offset:0 atIndex:0];
            [commandEncoder setVertexBuffer:UniformBuffer
                                     offset:uniformBufferOffset
                                    atIndex:1];

            [commandEncoder
                drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                           indexCount:[IndexBuffer length] / sizeof(uint16_t)
                            indexType:MTLIndexTypeUInt16
                          indexBuffer:IndexBuffer
                    indexBufferOffset:0];
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
    auto* example = new Texture;
    return example->Run(argc, argv);
}

#pragma clang diagnostic pop
