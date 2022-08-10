
#include "example.h"

#import <Metal/Metal.h>

#include "camera_x.h"

using namespace DirectX;

typedef XM_ALIGNED_STRUCT(16)
{
    XMFLOAT4 Position;
    XMFLOAT4 Color;
} Vertex;

typedef XM_ALIGNED_STRUCT(16)
{
    XMMATRIX ModelViewProj;
} Uniforms;

constexpr NSUInteger kAlignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;

class Triangle : public Example
{
    static constexpr int BUFFER_COUNT = 3;
 public:
    Triangle();

    ~Triangle();

    void Init() override;

    void Update() override;

    void Render() override;

 private:
    std::shared_ptr<class Camera> Camera;

    id<MTLDevice>              Device{};
    id<MTLCommandQueue>        CommandQueue{};
    id<MTLDepthStencilState>   DepthStencilState{};
    id<MTLTexture>             DepthStencilTexture{};
    id<MTLBuffer>              VertexBuffer{};
    id<MTLBuffer>              IndexBuffer{};
    id<MTLBuffer>              UniformBuffer[BUFFER_COUNT];
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

Triangle::Triangle() : Example("Triangle", 1280, 720)
{

}

Triangle::~Triangle() = default;

void Triangle::Init()
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
    const float  fov          = (75.0f * (float)M_PI) / 180.0f;
    const float  near         = 0.01f;
    const float  far          = 1000.0f;

    Camera = std::make_shared<class Camera>(
        XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f },
        XMFLOAT3{ 0.0f, 1.0f, 0.0f }, fov, aspect, near, far);

    Semaphore = dispatch_semaphore_create(BUFFER_COUNT);
}

MTLVertexDescriptor* Triangle::CreateVertexDescriptor()
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

void Triangle::UpdateUniform()
{
    auto translation = XMFLOAT3(0.0f, 0.0f, -10.0f);
    auto rotationX   = RotationX;
    auto rotationY   = RotationY;
    auto scaleFactor = 3.0f;

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

    const NSUInteger uniformBufferOffset = kAlignedUniformSize * FrameIndex;

    char* buffer = reinterpret_cast<char*>([UniformBuffer[FrameIndex] contents]);
    memcpy(buffer + uniformBufferOffset, &uniforms, sizeof(uniforms));
}

void Triangle::MakeBuffers()
{
    static const Vertex vertices[] = {
        { .Position = { 0, 1, 0, 1 }, .Color = { 1, 0, 0, 1 }},
        { .Position = { -1, -1, 0, 1 }, .Color = { 0, 1, 0, 1 }},
        { .Position = { 1, -1, 0, 1 }, .Color = { 0, 0, 1, 1 }}};

    static const uint16_t indices[] = { 0, 1, 2 };

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

    for (auto index = 0; index < BUFFER_COUNT; index++)
    {
        UniformBuffer[index] =
            [Device newBufferWithLength:kAlignedUniformSize * BUFFER_COUNT
                                options:MTLResourceOptionCPUCacheModeDefault];
        NSString* label = [NSString stringWithFormat:@"Uniform: %d", index];
        [UniformBuffer[index] setLabel:label];
    }

}

void Triangle::Update()
{
    float delta = 1.0f / 60.0f;
    RotationX = 0.0f;//+= delta; // * left_stick_y_;
    RotationY += delta; //* left_stick_x_;
}

void Triangle::Render()
{
    UpdateUniform();

    @autoreleasepool
    {
        FrameIndex = (FrameIndex + 1) % BUFFER_COUNT;

        id<MTLCommandBuffer> commandBuffer = [CommandQueue commandBuffer];
        dispatch_semaphore_wait(Semaphore, DISPATCH_TIME_FOREVER);
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull)
        {
          dispatch_semaphore_signal(Semaphore);
        }];

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

            id<MTLRenderCommandEncoder> commandEncoder =
                                            [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            [commandEncoder setRenderPipelineState:PipelineState];
            [commandEncoder setDepthStencilState:DepthStencilState];
            [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
            [commandEncoder setCullMode:MTLCullModeNone];

            const NSUInteger uniformBufferOffset =
                                 kAlignedUniformSize * FrameIndex;

            [commandEncoder setVertexBuffer:VertexBuffer offset:0 atIndex:0];
            [commandEncoder setVertexBuffer:UniformBuffer[FrameIndex]
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
    Triangle* example = new Triangle;

    return example->Run(argc, argv);
}

#pragma clang diagnostic pop
