#include "example.h"

#import <stdio.h>
#import <Metal/Metal.h>
#import <camera.h>
#import <simd/simd.h>
#import <simd_matrix.h>
#import <simd/simd.h>

typedef struct {
    simd_float4 position;
    simd_float4 color;
} Vertex;

typedef struct {
    simd_float4x4 mvp;
} Uniforms;
constexpr NSUInteger kAlignedUniformSize = (sizeof (Uniforms) + 0xFF) & -0x100;

class Triangle : public Example {
 public:
  Triangle ();

  ~Triangle ();

  void init () override;
  void update () override;
  void render () override;

#ifdef SYSTEM_MACOS
  void onKeyDown (Key key) override;
  void onKeyUp (Key key) override;
#endif

  void onSizeChange (float width, float height) override;
  void onLeftThumbstick (float x, float y) override;
  void onRightThumbstick (float x, float y) override;

 private:
  AAPLCamera *camera;

  id <MTLDevice> device_;
  id <MTLCommandQueue> command_queue_;
  id <MTLDepthStencilState> depth_stencil_state_;

  // Resources
  id <MTLTexture> depth_stencil_texture_;
  id <MTLBuffer> vertex_buffer_;
  id <MTLBuffer> index_buffer_;
  id <MTLBuffer> uniform_buffer_;
  id <MTLRenderPipelineState> pipeline_state_;
  id <MTLLibrary> library_;
  NSUInteger buffer_index_;

  void makeBuffers ();
  void updateUniform ();

  MTLVertexDescriptor *createVertexDescriptor ();

  static const int kBufferCount = 3;

  float rotation_y_ = 0.0f;
  float rotation_x_ = 0.0f;
  float stick_x_ = 0.0f;
  float stick_y_ = 0.0f;
  float left_stick_x_ = 0.0f;
  float left_stick_y_ = 0.0f;
  float camera_rotation_x_ = 0.0f;
  float camera_rotation_y_ = 0.0f;
  float camera_speed_ = 10.0f;
};

Triangle::Triangle ()
    : Example ("Triangle", 1280, 720)
{
}

Triangle::~Triangle ()
{
}

void
Triangle::init ()
{
  buffer_index_ = 0;

  // Metal initialization
  CAMetalLayer *layer = metalLayer ();
  device_ = layer.device;

  command_queue_ = [device_ newCommandQueue];

  MTLDepthStencilDescriptor *depth_stencil_desc = [MTLDepthStencilDescriptor new];
  depth_stencil_desc.depthCompareFunction = MTLCompareFunctionLess;
  depth_stencil_desc.depthWriteEnabled = true;
  depth_stencil_state_ = [device_ newDepthStencilStateWithDescriptor:depth_stencil_desc];

  // Create depth and stencil textures
  uint32_t width = static_cast<uint32_t>([layer drawableSize].width);
  uint32_t height = static_cast<uint32_t>([layer drawableSize].height);
  MTLTextureDescriptor *depth_stencil_tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8 width:width height:height mipmapped:NO];
  depth_stencil_tex_desc.sampleCount = 1;
  depth_stencil_tex_desc.usage = MTLTextureUsageUnknown;
  depth_stencil_tex_desc.resourceOptions =
      MTLResourceOptionCPUCacheModeDefault | MTLResourceStorageModePrivate;
  depth_stencil_texture_ = [device_ newTextureWithDescriptor:depth_stencil_tex_desc];

  makeBuffers ();

  library_ = [device_ newDefaultLibrary];

  MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];

  pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
  pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
  pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
  pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
  pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;

  pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
  pipelineDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
  pipelineDescriptor.vertexFunction = [library_ newFunctionWithName:@"vertex_project"];
  pipelineDescriptor.fragmentFunction = [library_ newFunctionWithName:@"fragment_flatcolor"];
  pipelineDescriptor.vertexDescriptor = createVertexDescriptor ();

  NSError *error = nullptr;
  pipeline_state_ = [device_ newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
  if (!pipeline_state_)
    {
      NSLog (@"Error occurred when creating render pipeline state: %@", error);
    }

  const CGSize drawableSize = layer.drawableSize;
  const float aspect = (float) drawableSize.width / (float) drawableSize.height;
  const float fov = (75.0 * M_PI) / 180.0f;
  const float near = 0.01f;
  const float far = 1000.0f;

  camera = [[AAPLCamera alloc] initPerspectiveWithPosition:{0.0f, 0.0f, 0.0f} direction:{0.0f, 0.0f, -1.0f} up:{0.0f, 1.0f, 0.0f} viewAngle:fov aspectRatio:aspect nearPlane:near farPlane:far];
  //    camera.setPerspective(aspect, fov, near, far);
  //    camera.setTranslation(0.0f, 0.0f, 0.0f);
}

MTLVertexDescriptor *
Triangle::createVertexDescriptor ()
{
  MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor new];

  // Position
  vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
  vertexDescriptor.attributes[0].offset = 0;
  vertexDescriptor.attributes[0].bufferIndex = 0;

  // Texture coordinates
  vertexDescriptor.attributes[1].format = MTLVertexFormatFloat4;
  vertexDescriptor.attributes[1].offset = sizeof (vector_float4);
  vertexDescriptor.attributes[1].bufferIndex = 0;

  vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
  vertexDescriptor.layouts[0].stride = sizeof (Vertex);

  return vertexDescriptor;
}

void
Triangle::updateUniform ()
{
  auto translation = simd_make_float3 (0.0f, 0.0f, -10.0f);
  auto rotationX = rotation_x_;
  auto rotationY = rotation_y_;
  //auto scaleFactor = 1.0f;

  const vector_float3 xAxis = {1, 0, 0};
  const vector_float3 yAxis = {0, 1, 0};
  const matrix_float4x4 xRot = simd_float4x4_rotation (xAxis, rotationX);
  const matrix_float4x4 yRot = simd_float4x4_rotation (yAxis, rotationY);
  const matrix_float4x4 rot = matrix_multiply (xRot, yRot);
  const matrix_float4x4 trans = simd_float4x4_translation (translation);
  const matrix_float4x4 modelMatrix = matrix_multiply (trans, rot); //matrix_multiply(matrix_multiply(xRot, yRot), scale);

  Uniforms uniforms;
  auto viewProj = camera.uniforms.viewProjectionMatrix;
  uniforms.mvp = matrix_multiply (viewProj, modelMatrix);

  const NSUInteger uniformBufferOffset = kAlignedUniformSize * buffer_index_;

  char *buffer = reinterpret_cast<char *>([uniform_buffer_ contents]);
  memcpy (buffer + uniformBufferOffset, &uniforms, sizeof (uniforms));
}

void
Triangle::makeBuffers ()
{
  static const Vertex vertices[] = {
      {.position = {-1, 1, 1, 1}, .color = {0, 1, 1, 1}},
      {.position = {-1, -1, 1, 1}, .color = {0, 0, 1, 1}},
      {.position = {1, -1, 1, 1}, .color = {1, 0, 1, 1}},
      {.position = {1, 1, 1, 1}, .color = {1, 1, 1, 1}},
      {.position = {-1, 1, -1, 1}, .color = {0, 1, 0, 1}},
      {.position = {-1, -1, -1, 1}, .color = {0, 0, 0, 1}},
      {.position = {1, -1, -1, 1}, .color = {1, 0, 0, 1}},
      {.position = {1, 1, -1, 1}, .color = {1, 1, 0, 1}}
  };

  static const uint16_t indices[] = {
      3, 2, 6, 6, 7, 3,
      4, 5, 1, 1, 0, 4,
      4, 0, 3, 3, 7, 4,
      1, 5, 6, 6, 2, 1,
      0, 1, 2, 2, 3, 0,
      7, 6, 5, 5, 4, 7
  };

  vertex_buffer_ = [device_ newBufferWithBytes:vertices length:sizeof (vertices) options:MTLResourceOptionCPUCacheModeDefault];
  [vertex_buffer_ setLabel:@"Vertices"];

  index_buffer_ = [device_ newBufferWithBytes:indices length:sizeof (indices) options:MTLResourceOptionCPUCacheModeDefault];
  [index_buffer_ setLabel:@"Indices"];

  uniform_buffer_ = [device_ newBufferWithLength:kAlignedUniformSize
                                                 * kBufferCount options:MTLResourceOptionCPUCacheModeDefault];
  [uniform_buffer_ setLabel:@"Uniforms"];
}

void
Triangle::update ()
{
  float delta = 1.0f / 60.0f;
  rotation_x_ += delta; // * left_stick_y_;
  rotation_y_ += delta; //* left_stick_x_;
}

void
Triangle::onSizeChange (float width, float height)
{
  float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
  [camera setAspectRatio:aspect_ratio];
  [camera updateUniforms];
}

#ifdef SYSTEM_MACOS

void
Triangle::onKeyDown (Key key)
{
  if (key == Key::A)
    {
      stick_x_ = 1.0f;
    }
  else if (key == Key::D)
    {
      stick_x_ = -1.0f;
    }
}

void
Triangle::onKeyUp (Key key)
{
  if (key == Key::A)
    {
      stick_x_ = 0.0f;
    }
  else if (key == Key::D)
    {
      stick_x_ = 0.0f;
    }
}

#endif

void
Triangle::onRightThumbstick (float x, float y)
{
  stick_x_ = x;
  stick_y_ = y;
}

void
Triangle::onLeftThumbstick (float x, float y)
{
  left_stick_x_ = x;
  left_stick_y_ = y;
}

void
Triangle::render ()
{
  float delta = 1.0f / 60.0f;
  camera_rotation_x_ = delta * stick_y_;
  camera_rotation_y_ = delta * -stick_x_;

  [camera rotateOnAxis:[camera right] radians:camera_rotation_x_];
  [camera rotateOnAxis:[camera up] radians:camera_rotation_y_];
  auto pos = [camera position];
  auto newPos = pos + (camera.forward * (delta * left_stick_y_) * camera_speed_);
  newPos = newPos + (camera.right * (delta * left_stick_x_) * camera_speed_);
  [camera setPosition:newPos];

  updateUniform ();

  @autoreleasepool
    {
      CAMetalLayer *layer = metalLayer ();
      id <CAMetalDrawable> drawable = [layer nextDrawable];
      if (drawable != nil)
        {

          id <MTLTexture> texture = drawable.texture;

          MTLRenderPassDescriptor *pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
          pass_desc.colorAttachments[0].texture = texture;
          pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
          pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
          pass_desc.colorAttachments[0].clearColor = MTLClearColorMake (.39, .58, .92, 1.0);
          pass_desc.depthAttachment.texture = depth_stencil_texture_;
          pass_desc.depthAttachment.loadAction = MTLLoadActionClear;
          pass_desc.depthAttachment.storeAction = MTLStoreActionStore;
          pass_desc.depthAttachment.clearDepth = 1.0;
          pass_desc.stencilAttachment.texture = depth_stencil_texture_;
          pass_desc.stencilAttachment.loadAction = MTLLoadActionClear;
          pass_desc.stencilAttachment.storeAction = MTLStoreActionDontCare;
          pass_desc.stencilAttachment.clearStencil = 0.0;

          id <MTLCommandBuffer> command_buffer = [command_queue_ commandBuffer];
          id <MTLRenderCommandEncoder> command_encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_desc];
          [command_encoder setRenderPipelineState:pipeline_state_];
          [command_encoder setDepthStencilState:depth_stencil_state_];
          [command_encoder setFrontFacingWinding:MTLWindingClockwise];
          [command_encoder setCullMode:MTLCullModeBack];

          const NSUInteger uniformBufferOffset = kAlignedUniformSize * buffer_index_;

          [command_encoder setVertexBuffer:vertex_buffer_ offset:0 atIndex:0];
          [command_encoder setVertexBuffer:uniform_buffer_ offset:uniformBufferOffset atIndex:1];

          [command_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                      indexCount:[index_buffer_ length] / sizeof (uint16_t)
                                       indexType:MTLIndexTypeUInt16
                                     indexBuffer:index_buffer_
                               indexBufferOffset:0];
          [command_encoder endEncoding];

          [command_buffer presentDrawable:drawable];

          [command_buffer commit];
        }
    }
}

int
main (int argc, char **argv)
{
  Triangle example;

  return example.run (argc, argv);
}
