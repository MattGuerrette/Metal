
#include "example.h"

#import <Metal/Metal.h>

#include "camera_x.h"
#include "model.h"

using namespace DirectX;

typedef XM_ALIGNED_STRUCT(16)
{
    XMMATRIX Transform;
} InstanceData;

class GLTF : public Example
{
    static constexpr int BUFFER_COUNT = 3;
    static constexpr int INSTANCE_COUNT = 1;
 public:
    GLTF();

    ~GLTF();

    void Init() override;

    void Update() override;

    void Render() override;

 private:
    std::shared_ptr<class Camera> Camera;
    std::unique_ptr<class Model> Model;

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
    dispatch_semaphore_t Semaphore{};

    void MakeBuffers();

    void UpdateUniform();

    MTLVertexDescriptor* CreateVertexDescriptor();

    float RotationY = 0.0f;
    float RotationX = 0.0f;
};

GLTF::GLTF() : Example("GLTF", 1280, 720)
{

}

GLTF::~GLTF() = default;

void GLTF::Init()
{
    Device      = MTLCreateSystemDefaultDevice();

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

MTLVertexDescriptor* GLTF::CreateVertexDescriptor()
{
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];

    // Position
    vertexDescriptor.attributes[0].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[0].offset      = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;

    // Color
    vertexDescriptor.attributes[1].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[1].offset      = offsetof(Vertex, Position);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    
    // UV coordinates
    vertexDescriptor.attributes[2].format      = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[2].offset      = offsetof(Vertex, Color);
    vertexDescriptor.attributes[2].bufferIndex = 0;
    
    // Joints
    vertexDescriptor.attributes[3].format      = MTLVertexFormatUInt4;
    vertexDescriptor.attributes[3].offset      = offsetof(Vertex, UV);
    vertexDescriptor.attributes[3].bufferIndex = 0;
    
    // Weights
    vertexDescriptor.attributes[4].format      = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[4].offset      = offsetof(Vertex, Joints);
    vertexDescriptor.attributes[4].bufferIndex = 0;

    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride       = sizeof(Vertex);

    return vertexDescriptor;
}

void GLTF::UpdateUniform()
{
    id<MTLBuffer> instanceBuffer = InstanceBuffer[FrameIndex];
    
    InstanceData* pInstanceData = reinterpret_cast< InstanceData *>( [instanceBuffer contents] );
    for(auto index = 0; index < INSTANCE_COUNT; ++index)
    {
        
        auto scaleFactor = 1.5f;
        auto translation = XMFLOAT3(0.0f, -1.0f, -5.0f);
        auto rotationX   = RotationX;
        auto rotationY   = RotationY;

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

void GLTF::MakeBuffers()
{
    // Load GLTF model file
    Model = std::make_unique<class Model>("CesiumMan.gltf", Device);
    
    const auto& vertexList = Model->GetVertices();
    const auto& indexList = Model->GetIndices();
    VertexBuffer =
        [Device newBufferWithBytes:vertexList.data()
                            length:vertexList.size() * sizeof(Vertex)
                           options:MTLResourceOptionCPUCacheModeDefault];
    [VertexBuffer setLabel:@"Vertices"];

    IndexBuffer =
        [Device newBufferWithBytes:indexList.data()
                            length:sizeof(uint16_t) * indexList.size()
                           options:MTLResourceOptionCPUCacheModeDefault];
    [IndexBuffer setLabel:@"Indices"];

    const size_t instanceDataSize = BUFFER_COUNT * INSTANCE_COUNT * sizeof(InstanceData);
    for(auto index = 0; index < BUFFER_COUNT; ++index)
    {
        InstanceBuffer[index] = [Device newBufferWithLength:instanceDataSize options:MTLResourceOptionCPUCacheModeDefault];
        NSString* label = [NSString stringWithFormat:@"InstanceBuffer: %d", index];
        [InstanceBuffer[index] setLabel:label];
    }
    
}

void GLTF::Update()
{
    float delta = 1.0f / 60.0f;
    RotationX = 0.0f;//+= delta; // * left_stick_y_;
    RotationY =0.0;//+= delta; //* left_stick_x_;
}

void GLTF::Render()
{
    
    @autoreleasepool
    {

        FrameIndex = (FrameIndex + 1) % BUFFER_COUNT;
        
        id<MTLBuffer> instanceBuffer = InstanceBuffer[FrameIndex];
        
        id<MTLCommandBuffer>        commandBuffer  = [CommandQueue commandBuffer];
        dispatch_semaphore_wait(Semaphore, DISPATCH_TIME_FOREVER);
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
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
            [commandEncoder setCullMode:MTLCullModeBack];

//            const NSUInteger uniformBufferOffset =
//                                 kAlignedInstanceDataSize * FrameIndex;

            [commandEncoder setFragmentTexture:Model->GetTexture() atIndex:0];
            [commandEncoder setVertexBuffer:VertexBuffer offset:0 atIndex:0];
            [commandEncoder setVertexBuffer:instanceBuffer offset:0 atIndex:1];

            [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:[IndexBuffer length] / sizeof(uint16_t) indexType:MTLIndexTypeUInt16 indexBuffer:IndexBuffer indexBufferOffset:0 instanceCount:INSTANCE_COUNT];
//            [commandEncoder
//                drawIndexedPrimitives:MTLPrimitiveTypeTriangle
//                           indexCount:[IndexBuffer length] / sizeof(uint16_t)
//                            indexType:MTLIndexTypeUInt16
//                          indexBuffer:IndexBuffer
//                    indexBufferOffset:0];
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
    auto* example = new GLTF;

    return example->Run(argc, argv);
}

#pragma clang diagnostic pop
