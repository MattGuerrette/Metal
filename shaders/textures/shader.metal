#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float4 position [[position]];
    float2 uv;
};

typedef struct
{
    array<texture2d<half>, 5> textures;
    uint32_t textureIndex;
    device float4x4* transforms;
} ArgumentBuffer;

vertex Vertex texture_vertex(
    device const Vertex* vertices [[buffer(0)]],
    const device ArgumentBuffer& argBuffer[[buffer(1)]],
    uint vid [[vertex_id]],
    uint iid [[instance_id]])
{
    Vertex vertexOut;
    vertexOut.position = argBuffer.transforms[iid] * vertices[vid].position;
    vertexOut.uv = vertices[vid].uv;

    // Using KTX textures the orientation will be incompatible
    // Flip here. Maybe eventually 'ktx create' command will support specifying
    // the coordinate space.
    vertexOut.uv.y = 1.0 - vertexOut.uv.y;
    return vertexOut;
}

fragment half4 texture_fragment(Vertex vertexIn [[stage_in]], const device ArgumentBuffer& argBuffer[[buffer(2)]])
{
    constexpr sampler colorSampler(mip_filter::linear,
                                       mag_filter::linear,
                                       min_filter::linear);

    half4 colorSample   = argBuffer.textures[argBuffer.textureIndex].sample(colorSampler, vertexIn.uv.xy);

    return half4(colorSample);
}

