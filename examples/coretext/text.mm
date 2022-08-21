
#import "Example.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <stdlib.h>
#define FONS_VERTEX_COUNT 2048
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

#define MTLFONTSTASH_IMPLEMENTATION
#include "mtlfontstash.h"

#import "Camera.h"

using namespace DirectX;

#define BUFFER_COUNT 3
#define INSTANCE_COUNT 1

XM_ALIGNED_STRUCT(16) Vertex
{
    XMFLOAT4 position;
    XMFLOAT4 color;
};


XM_ALIGNED_STRUCT(16) Uniforms
{
    XMMATRIX modelViewProj;
};

XM_ALIGNED_STRUCT(16) InstanceData
{
    XMMATRIX transform;
};

@interface GLTF : Example
{
    id<MTLDevice>              _device;
    id<MTLCommandQueue>        _commandQueue;
    id<MTLDepthStencilState>   _depthStencilState;
    id<MTLTexture>             _depthStencilTexture;
    id<MTLTexture>             _sampleTexture;
    id<MTLBuffer>              _vertexBuffer;
    id<MTLBuffer>              _indexBuffer;
    id<MTLBuffer>              _uniformBuffer[BUFFER_COUNT];
    id<MTLBuffer>              _instanceBuffer[BUFFER_COUNT];
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLLibrary>             _pipelineLibrary;
    NSUInteger                 _frameIndex;
    dispatch_semaphore_t       _semaphore;
    Camera* _camera;
    FONScontext *_fs;
    float _rotationX;
    float _rotationY;
    int _fontRegular;
}

- (instancetype)init;

- (BOOL)load;

- (void)update:(double)elapsed;

- (void)render:(double)elasped;

@end

@implementation GLTF

- (instancetype)init {
    self = [super initTitleWithDimensions:@"GLTF" :800 :600];
    
    return self;
}

- (void)updateUniform {
    id<MTLBuffer> instanceBuffer = _instanceBuffer[_frameIndex];
    
    InstanceData* instanceData = (InstanceData*)[instanceBuffer contents];
    for (auto index = 0; index < INSTANCE_COUNT; ++index)
    {
        auto scaleFactor = 1.5f;
        auto translation = XMFLOAT3(0.0f, -1.0f, -5.0f);
        auto rotationX   = _rotationX;
        auto rotationY   = _rotationY;
        
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

        CameraUniforms cameraUniforms = [_camera uniforms];

        instanceData[index].transform = modelMatrix * cameraUniforms.viewProjection;
    }
}

- (void)makeBuffers {
    
    NSString* filePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"CesiumMan.gltf"];
    
    static const Vertex vertices[] = {
        { .position = { 0, 1, 0, 1 }, .color = { 1, 0, 0, 1 }},
        { .position = { -1, -1, 0, 1 }, .color = { 0, 1, 0, 1 }},
        { .position = { 1, -1, 0, 1 }, .color = { 0, 0, 1, 1 }}};

    static const uint16_t indices[] = { 0, 1, 2 };

    _vertexBuffer =
        [_device newBufferWithBytes:vertices
                            length:sizeof(vertices)
                           options:MTLResourceOptionCPUCacheModeDefault];
    [_vertexBuffer setLabel:@"Vertices"];

    _indexBuffer =
        [_device newBufferWithBytes:indices
                            length:sizeof(indices)
                           options:MTLResourceOptionCPUCacheModeDefault];
    [_indexBuffer setLabel:@"Indices"];

    const size_t instanceDataSize = BUFFER_COUNT * INSTANCE_COUNT * sizeof(InstanceData);
    for (auto index = 0; index < BUFFER_COUNT; ++index)
    {
        _instanceBuffer[index] = [_device newBufferWithLength:instanceDataSize options:MTLResourceOptionCPUCacheModeDefault];
        NSString* label = [NSString stringWithFormat:@"InstanceBuffer: %d", index];
        [_instanceBuffer[index] setLabel:label];
    }
}

- (MTLVertexDescriptor*)createVertexDescriptor {
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];
    
    // Position
    vertexDescriptor.attributes[0].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[0].offset      = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;

    // Color
    vertexDescriptor.attributes[1].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[1].offset      = sizeof(XMFLOAT4);
    vertexDescriptor.attributes[1].bufferIndex = 0;

    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride       = sizeof(Vertex);
    
    return vertexDescriptor;
}

- (BOOL)load {
    _device = MTLCreateSystemDefaultDevice();
    

    // Metal initialization
    CAMetalLayer* layer = [self metalLayer];
    layer.device = _device;
    
    _fs = mtlfonsCreate(_device, 1024, 512, FONS_ZERO_TOPLEFT);
    NSAssert(_fs != nil, @"Failed to create fontstash context");
    
    NSString* filePath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"DroidSerif-Regular.ttf"];
    
    NSData* fontData = [[NSData alloc] initWithContentsOfFile:filePath];
    
    _fontRegular = fonsAddFontMem(_fs, "sans", (unsigned char*)[fontData bytes], (int)[fontData length], 0);
    //_fontRegular = fonsAddFont(_fs, "sans", "DroidSerif-Regular.ttf");
    if (_fontRegular == FONS_INVALID) {
        NSAssert(NO, @"Could not add font regular.\n");
    }
    mtlfonsSetRenderTargetPixelFormat(_fs, MTLPixelFormatBGRA8Unorm);

    _commandQueue = [_device newCommandQueue];

    MTLDepthStencilDescriptor* depthStencilDesc =
                                 [MTLDepthStencilDescriptor new];
    depthStencilDesc.depthCompareFunction = MTLCompareFunctionLess;
    depthStencilDesc.depthWriteEnabled    = true;
    _depthStencilState =
        [_device newDepthStencilStateWithDescriptor:depthStencilDesc];

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
    _depthStencilTexture =
        [_device newTextureWithDescriptor:depthStencilTexDesc];

    [self makeBuffers];

    NSString* libraryPath = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent:@"shader.metallib"];
    NSError * error       = nil;
    _pipelineLibrary = [_device newLibraryWithFile:libraryPath error:&error];
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
        [_pipelineLibrary newFunctionWithName:@"vertex_project"];
    pipelineDescriptor.fragmentFunction =
        [_pipelineLibrary newFunctionWithName:@"fragment_flatcolor"];
    pipelineDescriptor.vertexDescriptor = [self createVertexDescriptor];

    _pipelineState =
        [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                               error:&error];
    if (!_pipelineState)
    {
        NSLog(@"Error occurred when creating render pipeline state: %@", error);
    }
    
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:_device];

    NSString* texturePath = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent:@"dirt.png"];

    NSURL* url = [[NSURL alloc] initFileURLWithPath:texturePath];

    _sampleTexture = [textureLoader newTextureWithContentsOfURL:url options:@{ MTKTextureLoaderOptionSRGB: @NO } error:nil];

    const CGSize drawableSize = layer.drawableSize;
    const float  aspect       = (float)drawableSize.width / (float)drawableSize.height;
    const float  fov          = (75.0f * (float)M_PI) / 180.0f;
    const float  near         = 0.01f;
    const float  far          = 1000.0f;

    _camera = [[Camera alloc] initPerspectiveWithPosition:XMFLOAT3{0.0f, 0.0f, 0.0f} :XMFLOAT3{0.0f, 0.0f, -1.0f} :XMFLOAT3{0.0f, 1.0f, 0.0f} :fov :aspect :near :far];

    _semaphore = dispatch_semaphore_create(BUFFER_COUNT);
    fonsSetFont(_fs,_fontRegular);
    
    return YES;
}

unsigned int packRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

- (void)update:(double)elapsed {
    _rotationX = 0.0f;
    _rotationY += elapsed;
}

- (void)render:(double)elasped {

    @autoreleasepool
    {
        _frameIndex = (_frameIndex + 1) % BUFFER_COUNT;

        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull)
        {
          dispatch_semaphore_signal(_semaphore);
        }];
        
        [self updateUniform];

        CAMetalLayer* layer = [self metalLayer];
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
            passDesc.depthAttachment.texture        = _depthStencilTexture;
            passDesc.depthAttachment.loadAction     = MTLLoadActionClear;
            passDesc.depthAttachment.storeAction    = MTLStoreActionDontCare;
            passDesc.depthAttachment.clearDepth     = 1.0;
            passDesc.stencilAttachment.texture      = _depthStencilTexture;
            passDesc.stencilAttachment.loadAction   = MTLLoadActionClear;
            passDesc.stencilAttachment.storeAction  = MTLStoreActionDontCare;
            passDesc.stencilAttachment.clearStencil = 0;

            id<MTLRenderCommandEncoder> commandEncoder =
                                            [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
            MTLViewport viewport = { .originX = 0, .originY = 0, .width = 800, .height = 600 };
            mtlfonsSetRenderCommandEncoder(_fs, commandEncoder, viewport);
            
            [commandEncoder setRenderPipelineState:_pipelineState];
            [commandEncoder setDepthStencilState:_depthStencilState];
            [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
            [commandEncoder setCullMode:MTLCullModeNone];

            const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
            const NSUInteger uniformBufferOffset =
                                 alignedUniformSize * _frameIndex;


            [commandEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
            [commandEncoder setVertexBuffer:_instanceBuffer[_frameIndex] offset:0 atIndex:1];

            [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:
                               [_indexBuffer length]
                               / sizeof(uint16_t)  indexType:MTLIndexTypeUInt16
                                                 indexBuffer:_indexBuffer indexBufferOffset:0
                                               instanceCount:INSTANCE_COUNT];
            
            unsigned int white = packRGBA(255,255,255,255);
            unsigned int brown = packRGBA(192,128,0,128);
            unsigned int blue = packRGBA(0,192,255,255);
            unsigned int black = packRGBA(0,0,0,255);
            float sx, sy, dx, dy, lh = 0;
            sx = 50; sy = 50;
            dx = sx; dy = sy;

            float scale = [[NSScreen mainScreen] backingScaleFactor];
            
            fonsClearState(_fs);
            float ascend;
            float descend;
            float lineh;
            fonsVertMetrics(_fs, &ascend, &descend, &lineh);

            fonsSetSize(_fs, scale * 16.0f);
            //fonsDrawDebug(_fs, 50, 50.0f);
            fonsDrawText(_fs, 50, 50, "This is an example", nil);
//            fonsDrawText(_fs, 50, 50+(lineh * scale), "This is an example too", nil);
            
//            fonsSetColor(_fs, black);
//            fonsSetSpacing(_fs, 0.0f);
//            fonsSetBlur(_fs, scale * 3.0f);
//            fonsDrawText(_fs, dx,dy+(scale * 2),"DROP THAT SHADOW",NULL);
//
//            fonsSetColor(_fs, white);
//            fonsSetBlur(_fs, 0);
//            fonsDrawText(_fs, dx,dy,"DROP THAT SHADOW",nil);
            
            [commandEncoder endEncoding];

            [commandBuffer presentDrawable:drawable];

            [commandBuffer commit];
        }
    }
}


@end

#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
    NSInteger result;
    @autoreleasepool {
        GLTF* example = [[GLTF alloc] init];
        result = [example run:argc :(const char**)argv];
    }
    return (int)result;
}


//#include "Example.h"
//
//#import <Metal/Metal.h>
//
//#include "camera_x.h"
//#include "model.h"
//
//using namespace DirectX;
//
//typedef XM_ALIGNED_STRUCT(16)
//{
//    XMMATRIX Transform;
//} InstanceData;
//
//class GLTF : public Example
//{
//    static constexpr int BUFFER_COUNT = 3;
//    static constexpr int INSTANCE_COUNT = 1;
// public:
//    GLTF();
//
//    ~GLTF();
//
//    void Init() override;
//
//    void Update(double elapsed) override;
//
//    void Render(double elapsed) override;
//
// private:
//    std::shared_ptr<class Camera> Camera;
//    std::unique_ptr<class Model> Model;
//
//    id<MTLDevice>              Device{};
//    id<MTLCommandQueue>        CommandQueue{};
//    id<MTLDepthStencilState>   DepthStencilState{};
//    id<MTLTexture>             DepthStencilTexture;
//    id<MTLBuffer>              VertexBuffer{};
//    id<MTLBuffer>              IndexBuffer{};
//    id<MTLBuffer>              InstanceBuffer[BUFFER_COUNT];
//    id<MTLRenderPipelineState> PipelineState{};
//    id<MTLLibrary>             PipelineLibrary{};
//    NSUInteger                 FrameIndex{};
//    dispatch_semaphore_t Semaphore{};
//
//    void MakeBuffers();
//
//    void UpdateUniform();
//
//    MTLVertexDescriptor* CreateVertexDescriptor();
//
//    float RotationY = 0.0f;
//    float RotationX = 0.0f;
//};
//
//GLTF::GLTF() : Example("GLTF", 1280, 720)
//{
//
//}
//
//GLTF::~GLTF() = default;
//
//void GLTF::Init()
//{
//    Device      = MTLCreateSystemDefaultDevice();
//
//    // Metal initialization
//    CAMetalLayer* layer = GetMetalLayer();
//    layer.device = Device;
//
//    CommandQueue = [Device newCommandQueue];
//
//    MTLDepthStencilDescriptor* depthStencilDesc =
//                                 [MTLDepthStencilDescriptor new];
//    depthStencilDesc.depthCompareFunction = MTLCompareFunctionLess;
//    depthStencilDesc.depthWriteEnabled    = true;
//    DepthStencilState =
//        [Device newDepthStencilStateWithDescriptor:depthStencilDesc];
//
//    // Create depth and stencil textures
//    uint32_t width  = static_cast<uint32_t>([layer drawableSize].width);
//    uint32_t height = static_cast<uint32_t>([layer drawableSize].height);
//    MTLTextureDescriptor* depthStencilTexDesc = [MTLTextureDescriptor
//        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
//                                     width:width
//                                    height:height
//                                 mipmapped:NO];
//    depthStencilTexDesc.sampleCount = 1;
//    depthStencilTexDesc.usage       = MTLTextureUsageRenderTarget;
//    depthStencilTexDesc.resourceOptions =
//        MTLResourceOptionCPUCacheModeDefault | MTLResourceStorageModePrivate;
//    depthStencilTexDesc.storageMode = MTLStorageModeMemoryless;
//
//    DepthStencilTexture =
//        [Device newTextureWithDescriptor:depthStencilTexDesc];
//
//
//    MakeBuffers();
//
//    NSString* libraryPath = [[[NSBundle mainBundle] resourcePath]
//        stringByAppendingPathComponent:@"shader.metallib"];
//    NSError * error       = nil;
//    PipelineLibrary = [Device newLibraryWithFile:libraryPath error:&error];
//    MTLRenderPipelineDescriptor* pipelineDescriptor =
//                                   [MTLRenderPipelineDescriptor new];
//
//    pipelineDescriptor.colorAttachments[0].pixelFormat     = MTLPixelFormatBGRA8Unorm;
//    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
//    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor        =
//        MTLBlendFactorSourceAlpha;
//    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor   =
//        MTLBlendFactorOneMinusSourceAlpha;
//    pipelineDescriptor.colorAttachments[0].rgbBlendOperation           =
//        MTLBlendOperationAdd;
//    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor      =
//        MTLBlendFactorSourceAlpha;
//    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor =
//        MTLBlendFactorOneMinusSourceAlpha;
//    pipelineDescriptor.colorAttachments[0].alphaBlendOperation         =
//        MTLBlendOperationAdd;
//
//    pipelineDescriptor.depthAttachmentPixelFormat   =
//        MTLPixelFormatDepth32Float_Stencil8;
//    pipelineDescriptor.stencilAttachmentPixelFormat =
//        MTLPixelFormatDepth32Float_Stencil8;
//    pipelineDescriptor.vertexFunction   =
//        [PipelineLibrary newFunctionWithName:@"vertex_project"];
//    pipelineDescriptor.fragmentFunction =
//        [PipelineLibrary newFunctionWithName:@"fragment_flatcolor"];
//    pipelineDescriptor.vertexDescriptor = CreateVertexDescriptor();
//
//    PipelineState =
//        [Device newRenderPipelineStateWithDescriptor:pipelineDescriptor
//                                               error:&error];
//    if (!PipelineState)
//    {
//        NSLog(@"Error occurred when creating render pipeline state: %@", error);
//    }
//
//    const CGSize drawableSize = layer.drawableSize;
//    const float  aspect       = (float)drawableSize.width / (float)drawableSize.height;
//    const float  fov          = (75.0f * (float)M_PI) / 180.0f;
//    const float  near         = 0.01f;
//    const float  far          = 1000.0f;
//
//    Camera = std::make_shared<class Camera>(
//        XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, -1.0f },
//        XMFLOAT3{ 0.0f, 1.0f, 0.0f }, fov, aspect, near, far);
//
//    Semaphore = dispatch_semaphore_create(BUFFER_COUNT);
//
//}
//
//MTLVertexDescriptor* GLTF::CreateVertexDescriptor()
//{
//    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];
//
//    // Position
//    vertexDescriptor.attributes[0].format      = MTLVertexFormatFloat4;
//    vertexDescriptor.attributes[0].offset      = 0;
//    vertexDescriptor.attributes[0].bufferIndex = 0;
//
//    // Color
//    vertexDescriptor.attributes[1].format      = MTLVertexFormatFloat4;
//    vertexDescriptor.attributes[1].offset      = offsetof(Vertex, Position);
//    vertexDescriptor.attributes[1].bufferIndex = 0;
//
//    // UV coordinates
//    vertexDescriptor.attributes[2].format      = MTLVertexFormatFloat2;
//    vertexDescriptor.attributes[2].offset      = offsetof(Vertex, Color);
//    vertexDescriptor.attributes[2].bufferIndex = 0;
//
//    // Joints
//    vertexDescriptor.attributes[3].format      = MTLVertexFormatUInt4;
//    vertexDescriptor.attributes[3].offset      = offsetof(Vertex, UV);
//    vertexDescriptor.attributes[3].bufferIndex = 0;
//
//    // Weights
//    vertexDescriptor.attributes[4].format      = MTLVertexFormatFloat2;
//    vertexDescriptor.attributes[4].offset      = offsetof(Vertex, Joints);
//    vertexDescriptor.attributes[4].bufferIndex = 0;
//
//    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
//    vertexDescriptor.layouts[0].stride       = sizeof(Vertex);
//
//    return vertexDescriptor;
//}
//
//void GLTF::UpdateUniform()
//{
//    id<MTLBuffer> instanceBuffer = InstanceBuffer[FrameIndex];
//
//    InstanceData* pInstanceData = reinterpret_cast< InstanceData *>( [instanceBuffer contents] );
//    for(auto index = 0; index < INSTANCE_COUNT; ++index)
//    {
//
//        auto scaleFactor = 1.5f;
//        auto translation = XMFLOAT3(0.0f, -1.0f, -5.0f);
//        auto rotationX   = RotationX;
//        auto rotationY   = RotationY;
//
//        const XMFLOAT3 xAxis = { 1, 0, 0 };
//        const XMFLOAT3 yAxis = { 0, 1, 0 };
//
//        XMVECTOR xRotAxis = XMLoadFloat3(&xAxis);
//        XMVECTOR yRotAxis = XMLoadFloat3(&yAxis);
//
//        XMMATRIX xRot        = XMMatrixRotationAxis(xRotAxis, rotationX);
//        XMMATRIX yRot        = XMMatrixRotationAxis(yRotAxis, rotationY);
//        XMMATRIX rot         = XMMatrixMultiply(xRot, yRot);
//        XMMATRIX trans       =
//                     XMMatrixTranslation(translation.x, translation.y, translation.z);
//        XMMATRIX scale       = XMMatrixScaling(scaleFactor, scaleFactor, scaleFactor);
//        XMMATRIX modelMatrix = XMMatrixMultiply(scale, XMMatrixMultiply(rot, trans));
//
//        CameraUniforms cameraUniforms = Camera->GetUniforms();
//
//        pInstanceData[index].Transform = modelMatrix * cameraUniforms.ViewProjection;
//    }
//}
//
//void GLTF::MakeBuffers()
//{
//    // Load GLTF model file
//    Model = std::make_unique<class Model>("CesiumMan.gltf", Device);
//
//    const auto& vertexList = Model->GetVertices();
//    const auto& indexList = Model->GetIndices();
//    VertexBuffer =
//        [Device newBufferWithBytes:vertexList.data()
//                            length:vertexList.size() * sizeof(Vertex)
//                           options:MTLResourceOptionCPUCacheModeDefault];
//    [VertexBuffer setLabel:@"Vertices"];
//
//    IndexBuffer =
//        [Device newBufferWithBytes:indexList.data()
//                            length:sizeof(uint16_t) * indexList.size()
//                           options:MTLResourceOptionCPUCacheModeDefault];
//    [IndexBuffer setLabel:@"Indices"];
//
//    const size_t instanceDataSize = BUFFER_COUNT * INSTANCE_COUNT * sizeof(InstanceData);
//    for(auto index = 0; index < BUFFER_COUNT; ++index)
//    {
//        InstanceBuffer[index] = [Device newBufferWithLength:instanceDataSize options:MTLResourceOptionCPUCacheModeDefault];
//        NSString* label = [NSString stringWithFormat:@"InstanceBuffer: %d", index];
//        [InstanceBuffer[index] setLabel:label];
//    }
//
//}
//
//void GLTF::Update(double elapsed)
//{
//    RotationX = 0.0f;
//    RotationY = 0.0f;
//}
//
//void GLTF::Render(double elapsed)
//{
//
//    @autoreleasepool
//    {
//
//        FrameIndex = (FrameIndex + 1) % BUFFER_COUNT;
//
//        id<MTLBuffer> instanceBuffer = InstanceBuffer[FrameIndex];
//
//        id<MTLCommandBuffer>        commandBuffer  = [CommandQueue commandBuffer];
//        dispatch_semaphore_wait(Semaphore, DISPATCH_TIME_FOREVER);
//        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
//            dispatch_semaphore_signal(Semaphore);
//        }];
//
//        UpdateUniform();
//
//        CAMetalLayer* layer = GetMetalLayer();
//        id<CAMetalDrawable> drawable = [layer nextDrawable];
//        if (drawable != nil)
//        {
//
//            id<MTLTexture> texture = drawable.texture;
//
//            MTLRenderPassDescriptor* passDesc =
//                                       [MTLRenderPassDescriptor renderPassDescriptor];
//            passDesc.colorAttachments[0].texture     = texture;
//            passDesc.colorAttachments[0].loadAction  = MTLLoadActionClear;
//            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
//            passDesc.colorAttachments[0].clearColor =
//                MTLClearColorMake(.39, .58, .92, 1.0);
//            passDesc.depthAttachment.texture        = DepthStencilTexture;
//            passDesc.depthAttachment.loadAction     = MTLLoadActionClear;
//            passDesc.depthAttachment.storeAction    = MTLStoreActionDontCare;
//            passDesc.depthAttachment.clearDepth     = 1.0;
//            passDesc.stencilAttachment.texture      = DepthStencilTexture;
//            passDesc.stencilAttachment.loadAction   = MTLLoadActionClear;
//            passDesc.stencilAttachment.storeAction  = MTLStoreActionDontCare;
//            passDesc.stencilAttachment.clearStencil = 0;
//
//            id<MTLRenderCommandEncoder> commandEncoder =
//                                            [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
//            [commandEncoder setRenderPipelineState:PipelineState];
//            [commandEncoder setDepthStencilState:DepthStencilState];
//            [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
//            [commandEncoder setCullMode:MTLCullModeBack];
//            [commandEncoder setFragmentTexture:Model->GetTexture() atIndex:0];
//            [commandEncoder setVertexBuffer:VertexBuffer offset:0 atIndex:0];
//            [commandEncoder setVertexBuffer:instanceBuffer offset:0 atIndex:1];
//
//            [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:[IndexBuffer length] / sizeof(uint16_t) indexType:MTLIndexTypeUInt16 indexBuffer:IndexBuffer indexBufferOffset:0 instanceCount:INSTANCE_COUNT];
//            [commandEncoder endEncoding];
//
//            [commandBuffer presentDrawable:drawable];
//
//            [commandBuffer commit];
//        }
//    }
//}
