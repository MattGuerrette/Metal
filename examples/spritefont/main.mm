
#import "Example.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#import "Camera.h"
#import "SpriteFont.h"
#import "SpriteBatch.h"

using namespace DirectX;

#define BUFFER_COUNT 3

XM_ALIGNED_STRUCT(16) Vertex
{
    XMFLOAT4 position;
    XMFLOAT4 color;
    XMFLOAT2 uv;
};


XM_ALIGNED_STRUCT(16) Uniforms
{
    XMMATRIX modelViewProj;
};

@interface FontExample : Example
{
    id<MTLDevice>              _device;
    id<MTLCommandQueue>        _commandQueue;
    id<MTLDepthStencilState>   _depthStencilState;
    id<MTLTexture>             _depthStencilTexture;
    id<MTLBuffer>              _vertexBuffer;
    id<MTLBuffer>              _indexBuffer;
    id<MTLBuffer>              _uniformBuffer[BUFFER_COUNT];
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLLibrary>             _pipelineLibrary;
    id<MTLTexture> _basicSprite;
    NSUInteger                 _frameIndex;
    dispatch_semaphore_t       _semaphore;
    Camera* _camera;
    SpriteFont* _spriteFont;
    SpriteBatch* _spriteBatch;
    SpriteBatch* _spriteBatch2;
    float _rotationX;
    float _rotationY;
}

-(instancetype)init;

-(BOOL)load;

-(void)update:(double)elapsed;

-(void)render:(double)elasped;

@end

@implementation FontExample

- (instancetype)init {
    self = [super initTitleWithDimensions:@"Triangle" :800 :600];
    _rotationX = 0.0f;
    _rotationY = 0.0f;
    _frameIndex = 0;
    return self;
}

- (void)updateUniform {
    auto translation = XMFLOAT3(0.0f, 0.0f, -10.0f);
    auto rotationX   = _rotationX;
    auto rotationY   = _rotationY;
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

    CameraUniforms cameraUniforms = [_camera uniforms];

    Uniforms uniforms;
    auto     viewProj = cameraUniforms.projection;
    uniforms.modelViewProj = viewProj;

    const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
    const NSUInteger uniformBufferOffset = alignedUniformSize * _frameIndex;

    char* buffer = reinterpret_cast<char*>([_uniformBuffer[_frameIndex] contents]);
    memcpy(buffer + uniformBufferOffset, &uniforms, sizeof(uniforms));
}

- (void)makeBuffers {
    float scaleX = 227.0f;
    float scaleY = 335.0f;
    static const Vertex vertices[] = {
        { .position = { 0.0f * scaleX, 1 *scaleY, 0, 1 }, .color = { 0, 1, 1, 1 }, .uv = { 0, 0 }},
        { .position = { 0.0f * scaleX, 0.0f *scaleY, 0, 1 }, .color = { 0, 0, 1, 1 }, .uv = { 0, 1 }},
        { .position = { 1* scaleX, 0.0f*scaleY, 0, 1 }, .color = { 1, 0, 1, 1 }, .uv = { 1, 1 }},
        { .position = { 1*scaleX, 1*scaleY, 0, 1 }, .color = { 1, 1, 1, 1 }, .uv = { 1, 0 }}};

    static const uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };

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

    const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
    for (auto index = 0; index < BUFFER_COUNT; index++)
    {
        _uniformBuffer[index] =
            [_device newBufferWithLength:alignedUniformSize * BUFFER_COUNT
                                options:MTLResourceOptionCPUCacheModeDefault];
        NSString* label = [NSString stringWithFormat:@"Uniform: %d", index];
        [_uniformBuffer[index] setLabel:label];
    }
}

- (MTLVertexDescriptor*)createVertexDescriptor {
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];
    
    // Position
    vertexDescriptor.attributes[0].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[0].offset      = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;

//    // Color
//    vertexDescriptor.attributes[1].format      = MTLVertexFormatFloat4;
//    vertexDescriptor.attributes[1].offset      = sizeof(XMFLOAT4);
//    vertexDescriptor.attributes[1].bufferIndex = 0;
    
    // UV
    vertexDescriptor.attributes[2].format      = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[2].offset      = sizeof(XMFLOAT2);
    vertexDescriptor.attributes[2].bufferIndex = 0;

    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    vertexDescriptor.layouts[0].stride       = sizeof(Vertex);
    
    return vertexDescriptor;
}

- (BOOL)load {
    _device = MTLCreateSystemDefaultDevice();

    // Metal initialization
    CAMetalLayer* layer = [self metalLayer];
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.device = _device;

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
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;

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

    const CGSize drawableSize = layer.drawableSize;
    const float  aspect       = (float)drawableSize.width / (float)drawableSize.height;
    const float  fov          = (75.0f * (float)M_PI) / 180.0f;
    const float  near         = 0.01f;
    const float  far          = 1000.0f;

//    _camera = [[Camera alloc] initPerspectiveWithPosition:XMFLOAT3{0.0f, 0.0f, 0.0f} :XMFLOAT3{0.0f, 0.0f, -1.0f} :XMFLOAT3{0.0f, 1.0f, 0.0f} :fov :aspect :near :far];

    _camera = [[Camera alloc] initOrthographic:800.0f :600.0f :0.0f :1.0f];
    
    _semaphore = dispatch_semaphore_create(BUFFER_COUNT);
    
    NSString* fontFile = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Arial.fnt"];
    _spriteFont = [[SpriteFont alloc] initWithFile:fontFile device:_device];
    
    _spriteBatch = [[SpriteBatch alloc] initWithDevice:_device];
    
    _spriteBatch2 = [[SpriteBatch alloc] initWithDevice:_device];
    
    NSString* texturePath = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent:@"BasicSprite.png"];
    NSURL* url = [[NSURL alloc] initFileURLWithPath:texturePath];
    
    NSDictionary *textureLoaderOptions = @{
          MTKTextureLoaderOptionSRGB: @NO,
          MTKTextureLoaderOptionTextureUsage       : @(MTLTextureUsageShaderRead),
          MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
        };
    
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:_device];
    _basicSprite = [textureLoader newTextureWithContentsOfURL:url options:textureLoaderOptions error:nil];
    
    return YES;
}

- (void)update:(double)elapsed {
    _rotationX = 0.0f;
    _rotationY =0.0f; //+= elapsed;
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
            [commandEncoder setRenderPipelineState:_pipelineState];
            [commandEncoder setDepthStencilState:_depthStencilState];
            [commandEncoder setFrontFacingWinding:MTLWindingClockwise];
            [commandEncoder setCullMode:MTLCullModeNone];

            const size_t alignedUniformSize = (sizeof(Uniforms) + 0xFF) & -0x100;
            const NSUInteger uniformBufferOffset =
                                 alignedUniformSize * _frameIndex;

            //[commandEncoder setFragmentTexture:_basicSprite atIndex:0];
            //[commandEncoder setFragmentTexture:[_spriteFont texture] atIndex:0];
            //[commandEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
            [commandEncoder setVertexBuffer:_uniformBuffer[_frameIndex]
                                     offset:uniformBufferOffset
                                    atIndex:1];

//            [commandEncoder
//                drawIndexedPrimitives:MTLPrimitiveTypeTriangle
//                           indexCount:[_indexBuffer length] / sizeof(uint16_t)
//                            indexType:MTLIndexTypeUInt16
//                          indexBuffer:_indexBuffer
//                    indexBufferOffset:0];
            
            [_spriteBatch begin:commandEncoder];
            //[_spriteBatch draw:_basicSprite position:DirectX::XMFLOAT2(0.0f, 0.0f) sourceRectangle:{}];
            
            float xOffset = 0.0f;
            float yOffset = 0.0f;
            for(uint32_t i = 0; i < 3; i++)
            {
                [_spriteBatch draw:_basicSprite position:DirectX::XMFLOAT2(xOffset, yOffset) sourceRectangle:{}];
                xOffset += 75.0f;

                if (i == 5) {
                    xOffset = 0.0f;
                    yOffset += 150.0f;
                }
            }
            // Draw sprites
//            [_spriteBatch draw:_basicSprite position:DirectX::XMFLOAT2(0.0f, 0.0f) sourceRectangle:{}];
//
//            //[_spriteBatch begin:commandEncoder];
//            [_spriteBatch draw:_basicSprite position:DirectX::XMFLOAT2(100.0f, 100.0f) sourceRectangle:{}];
            [_spriteBatch end];
            
//            [_spriteBatch2 begin:commandEncoder];
//            [_spriteBatch2 draw:[_spriteFont texture] position:DirectX::XMFLOAT2(100.0f, 100.0f) sourceRectangle:{}];
//            [_spriteBatch2 end];
            
            
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
    NSInteger result = EXIT_FAILURE;
    @autoreleasepool {
        FontExample* example = [[FontExample alloc] init];
        result = [example run:argc :(const char**)argv];
    }
    
    return (int)result;
}
