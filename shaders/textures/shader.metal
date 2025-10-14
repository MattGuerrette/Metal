#include <metal_stdlib>

using namespace metal;

struct VertexIn {
    float4 position;
    float2 uv;
};

struct VertexOut {
    float4 position [[position]];
    float2 uv;
    uint32_t textureIndex;
};

typedef struct
{
    array<texture2d<half>, 5> textures;
    device float4x4* transforms;
} ArgumentBuffer;

vertex VertexOut texture_vertex(
    device const VertexIn* vertices [[buffer(0)]],
    const device ArgumentBuffer& argBuffer[[buffer(1)]],
    uint vid [[vertex_id]],
    uint iid [[instance_id]])
{
    VertexOut vertexOut;
    vertexOut.position = argBuffer.transforms[iid] * vertices[vid].position;
    vertexOut.uv = vertices[vid].uv;
    vertexOut.textureIndex = iid;

    // Using KTX textures the orientation will be incompatible
    // Flip here. Maybe eventually 'ktx create' command will support specifying
    // the coordinate space.
    vertexOut.uv.y = 1.0 - vertexOut.uv.y;
    return vertexOut;
}

fragment half4 texture_fragment(VertexOut vertexIn [[stage_in]], const device ArgumentBuffer& argBuffer[[buffer(2)]])
{
    constexpr sampler colorSampler(mip_filter::linear,
                                       mag_filter::linear,
                                       min_filter::linear);

    half4 colorSample   = argBuffer.textures[vertexIn.textureIndex].sample(colorSampler, vertexIn.uv.xy);

    return half4(colorSample);
}

