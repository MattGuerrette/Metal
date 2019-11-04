#include "example.h"

#import <Metal/Metal.h>

class Triangle : public Example
{
public:
    Triangle();
    
    ~Triangle();
    
    void init() override;
    void update() override;
    void render() override;
    
private:
    id<MTLDevice> device_;
    id<MTLCommandQueue> command_queue_;
    id<MTLDepthStencilState> depth_stencil_state_;
    
    // Resources
    id<MTLTexture> depth_stencil_texture_;
    id<MTLBuffer> vertex_buffer_;
    id<MTLBuffer> index_buffer_;
    
};

Triangle::Triangle()
    : Example("Triangle", 1280, 720)
{
    
}

Triangle::~Triangle()
{
    
}

void Triangle::init()
{
    // Metal initialization
    CAMetalLayer* layer = metalLayer();
    device_ = layer.device;
    
    command_queue_ = [device_ newCommandQueue];
    
    MTLDepthStencilDescriptor* depth_stencil_desc = [MTLDepthStencilDescriptor new];
    depth_stencil_desc.depthCompareFunction = MTLCompareFunctionLess;
    depth_stencil_desc.depthWriteEnabled = true;
    depth_stencil_state_ = [device_ newDepthStencilStateWithDescriptor:depth_stencil_desc];
    
    // Create depth and stencil textures
    uint32_t width = static_cast<uint32_t>([layer drawableSize].width);
    uint32_t height = static_cast<uint32_t>([layer drawableSize].height);
    MTLTextureDescriptor* depth_stencil_tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8 width:width height:height mipmapped:NO];
    depth_stencil_tex_desc.sampleCount = 1;
    depth_stencil_tex_desc.usage = MTLTextureUsageUnknown;
    depth_stencil_tex_desc.resourceOptions = MTLResourceOptionCPUCacheModeDefault | MTLResourceStorageModePrivate;
    depth_stencil_texture_ = [device_ newTextureWithDescriptor:depth_stencil_tex_desc];
}

void Triangle::update()
{
    
}

void Triangle::render()
{
    @autoreleasepool {
        CAMetalLayer* layer = metalLayer();
        id<CAMetalDrawable> drawable = [layer nextDrawable];
        if (drawable != nil) {
            
            id<MTLTexture> texture = drawable.texture;
            
            MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
            pass_desc.colorAttachments[0].texture = texture;
            pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
            pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
            pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(.39, .58, .92, 1.0);
            pass_desc.depthAttachment.texture = depth_stencil_texture_;
            pass_desc.depthAttachment.loadAction = MTLLoadActionClear;
            pass_desc.depthAttachment.storeAction = MTLStoreActionStore;
            pass_desc.depthAttachment.clearDepth = 1.0;
            pass_desc.stencilAttachment.texture = depth_stencil_texture_;
            pass_desc.stencilAttachment.loadAction = MTLLoadActionClear;
            pass_desc.stencilAttachment.storeAction = MTLStoreActionDontCare;
            pass_desc.stencilAttachment.clearStencil = 0.0;
            
            id<MTLCommandBuffer> command_buffer = [command_queue_ commandBuffer];
            id<MTLRenderCommandEncoder> command_encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_desc];
            [command_encoder setDepthStencilState:depth_stencil_state_];
            [command_encoder setFrontFacingWinding:MTLWindingClockwise];
            [command_encoder setCullMode:MTLCullModeBack];
            [command_encoder endEncoding];
            
            [command_buffer presentDrawable:drawable];
            
            [command_buffer commit];
        }
    }
}

int main(int argc, char** argv)
{
    Triangle example;
    
    return example.run();
}
