
#import "SpriteBatch.h"

#import <vector>
#import <array>

#import "graphics_math.h"

using namespace DirectX;

#define VERTICES_PER_SPRITE 4
#define INDICES_PER_SPRITE 6
#define MAX_BATCH_SIZE 2048
#define MIN_BATCH_SIZE 128
#define MAX_VERTEX_COUNT VERTICES_PER_SPRITE * MAX_BATCH_SIZE;
#define MAX_INDEX_COUNT INDICES_PER_SPRITE * MAX_BATCH_SIZE;

XM_ALIGNED_STRUCT(16) SourceRect
{
    XMFLOAT2 origin;
    XMFLOAT2 bounds;
};

XM_ALIGNED_STRUCT(16) BatchInfo
{
    XMFLOAT2 position;
    XMFLOAT4 color;
    SourceRect source;
    XMFLOAT2 origin;
    XMFLOAT2 scale;
    float rotation;
    float alpha;
};

XM_ALIGNED_STRUCT(16) SpriteVertex
{
    XMFLOAT4 position;
    XMFLOAT2 uv;
};


@interface SpriteBatch ()
{
    BOOL _inBeginEnd;
    id<MTLTexture> _activeTexture;
    id<MTLBuffer> _vertexBuffer;
    id<MTLBuffer> _indexBuffer;
    id<MTLRenderCommandEncoder> _commandEncoder;
    std::vector<BatchInfo> _batches;
}

- (void)flush;

- (void)buildVertexData:(BatchInfo)info
                       :(size_t)offset;

@end

@implementation SpriteBatch

- (instancetype)initWithDevice:(id<MTLDevice>)device {
    self = [super init];
    _inBeginEnd = NO;
    _activeTexture = nil;
    
    const NSUInteger vertexBufferSize = sizeof(SpriteVertex) * VERTICES_PER_SPRITE;
    _vertexBuffer = [device newBufferWithLength:vertexBufferSize options:MTLResourceOptionCPUCacheModeDefault];
    
    const NSUInteger indexBufferSize = sizeof(uint16_t) * MAX_INDEX_COUNT;
    _indexBuffer = [device newBufferWithLength:indexBufferSize options:MTLResourceOptionCPUCacheModeDefault];
    [_indexBuffer setLabel:@"SpriteBatch_IndexBuffer"];
    
    //populate index buffer
    size_t offset = 0;
    for (uint16_t i = 0, j = 0; i < MAX_BATCH_SIZE; i++, j += VERTICES_PER_SPRITE)
    {
        //each sprite is made up of 6 indices, 3 per triangle, 2 tri's per sprite
        const std::array<uint16_t, INDICES_PER_SPRITE> temp =
        {
            static_cast<uint16_t>(0 + j),
            static_cast<uint16_t>(1 + j),
            static_cast<uint16_t>(2 + j),
            static_cast<uint16_t>(2 + j),
            static_cast<uint16_t>(1 + j),
            static_cast<uint16_t>(3 + j)
        };
        
        char* buffer = reinterpret_cast<char*>([_indexBuffer contents]);
        memcpy(buffer + offset, temp.data(), temp.size() * sizeof(uint16_t));
        
        offset += sizeof(uint16_t) * INDICES_PER_SPRITE;
    }
    
    return self;
}


- (void)begin:(id<MTLRenderCommandEncoder>)encoder {
    NSAssert(!_inBeginEnd, @"begin must align with corresponding call to end");
    
    _activeTexture = nil;
    _inBeginEnd = YES;
    _commandEncoder = encoder;
    
    [_commandEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
}

- (void)draw:(id<MTLTexture>)texture position:(DirectX::XMFLOAT2)position sourceRectangle:(Rect)rect {
    

    if (_batches.size() >= MAX_BATCH_SIZE || (_activeTexture != nil && _activeTexture != texture))
    {
        // Flush
        [self flush];
    }
    
    _activeTexture = texture;
    
    // Add batch info to queue
    BatchInfo info;
    info.position = position;
    info.scale.x = 1.0f;
    info.scale.y = 1.0f;
    info.origin.x = 0.0f;
    info.origin.y = 0.0f;
    info.rotation = 0.0f;
//    info.source.origin.x = rect.left;
//    info.source.origin.y = rect.top;
//    info.source.
    _batches.push_back(info);
}

- (void)end {
    NSAssert(_inBeginEnd, @"end must align with corresponding call to begin");
    
    [self flush];
    
    _activeTexture = nil;
    _inBeginEnd = NO;
    _commandEncoder = nil;
}

- (void)flush {
    // Accumulate queued texture geometry into vertex buffer
    size_t offset = 0;
    for(auto& batch : _batches)
    {
        [self buildVertexData:batch :offset];
        
        offset += sizeof(SpriteVertex) * VERTICES_PER_SPRITE;
    }
    

    [_commandEncoder setFragmentTexture:_activeTexture atIndex:0];
    [_commandEncoder
        drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                   indexCount:_batches.size() * INDICES_PER_SPRITE
                    indexType:MTLIndexTypeUInt16
                  indexBuffer:_indexBuffer
            indexBufferOffset:0];
    
    _batches.clear();
}

- (void)buildVertexData:(BatchInfo)info :(size_t)offset{
    
    /*cache texture width and height*/
    const float tex_width = (float)[_activeTexture width];
    const float tex_height = (float)[_activeTexture height];
    const float inv_tex_width = 1.0f / tex_width;
    const float inv_tex_height = 1.0f / tex_height;

    //----------------------
    // x1 = x
    // y1 = y
    // x2 = x + width
    // y2 = y + height
    //
    //(x1,y1)-----------(x2,y1)
    // |                      |
    // |                      |
    // |                      |
    // |                      |
    // |                      |
    // |                      |
    //(x1,y2)-----------(x2,y2)


    /*store origin offset based on position*/
    const float worldOriginX = info.position.x + info.origin.x;
    const float worldOriginY = info.position.y + info.origin.y;

    /*create corner points (pre-transform)*/
    float fx = -info.origin.x;
    float fy = -info.origin.y;
    float fx2;
    float fy2;
//    if (info.source.bounds.x != 0 || info.source.bounds.y != 0) {
//        fx2 = info.source.bounds.x - info.origin.x;
//        fy2 = info.source.bounds.y - info.origin.y;
//    }
//    else {
        fx2 = tex_width - info.origin.x;
        fy2 = tex_height - info.origin.y;
   // }

    /*apply scale*/
    fx *= info.scale.x;
    fy *= info.scale.y;
    fx2 *= info.scale.x;
    fy2 *= info.scale.y;

    /*construct corners*/
    const float c1x = fx;
    const float c1y = fy;
    const float c2x = fx2;
    const float c2y = fy;
    const float c3x = fx;
    const float c3y = fy2;
    const float c4x = fx2;
    const float c4y = fy2;


    /*create temp vars for corner points to have rotation applied*/
    float x1;
    float y1;
    float x2;
    float y2;
    float x3;
    float y3;
    float x4;
    float y4;

    /*apply rotation*/
    if (info.rotation) {

        const float cos = DirectX::XMScalarCos(info.rotation);
        const float sin = DirectX::XMScalarSin(info.rotation);

        //To rotate a point around origin point
        //the solution follows the trigonometric function:
        //[x'] = [cos0 - sin0][x]
        //[y'] = [sin0 + cos0][y]
        //
        //So when expanded we are left with:
        //
        // x' = x * cos0 - y * sin0
        // y' = x * sin0 + y * cos0
        //

        x1 = cos * c1x - sin * c1y;
        y1 = sin * c1x + cos * c1y;
        x2 = cos * c2x - sin * c2y;
        y2 = sin * c2x + cos * c2y;
        x3 = cos * c3x - sin * c3y;
        y3 = sin * c3x + cos * c3y;
        x4 = cos * c4x - sin * c4y;
        y4 = sin * c4x + cos * c4y;
    }
    else { /*no rotation*/
        x1 = c1x;
        y1 = c1y;
        x2 = c2x;
        y2 = c2y;
        x3 = c3x;
        y3 = c3y;
        x4 = c4x;
        y4 = c4y;
    }

    /*add origin offset*/
    x1 += worldOriginX;
    y1 += worldOriginY;
    x2 += worldOriginX;
    y2 += worldOriginY;
    x3 += worldOriginX;
    y3 += worldOriginY;
    x4 += worldOriginX;
    y4 += worldOriginY;

    /*setup uv's appropriate to source rect*/
    float u;
    float v;
    float u2;
    float v2;

//    SourceRect sourceRect = info.source;
//    if(sourceRect.origin.x != 0 || sourceRect.origin.y != 0 || sourceRect.bounds.x != 0 || sourceRect.bounds.y != 0)
//    {
//        u = info.source.origin.x * inv_tex_width;
//        v = info.source.origin.y * inv_tex_height;
//        u2 = (info.source.origin.x + info.source.bounds.x) * inv_tex_width;
//        v2 = (info.source.origin.y + info.source.bounds.y) * inv_tex_height;
//    }
//    else
//    {
        u = 0.0f;
        v = 0.0f;
        u2 = 1.0f;
        v2 = 1.0f;
    //}
    
    
    const std::array<SpriteVertex, VERTICES_PER_SPRITE> temp = {
        SpriteVertex{ .position = {x1, y1, 0.0f, 1.0f}, .uv = {u, v}},
        SpriteVertex{ .position = {x2, y2, 0.0f, 1.0f}, .uv = {u2, v}},
        SpriteVertex{ .position = {x3, y3, 0.0f, 1.0f}, .uv = {u, v2}},
        SpriteVertex{ .position = {x4, y4, 0.0f, 1.0f}, .uv = {u2, v2}}
    };
    
    char* buffer = reinterpret_cast<char*>([_vertexBuffer contents]);
    memcpy(buffer + offset, temp.data(), sizeof(SpriteVertex) * temp.size());
}

@end
