#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float4 position [[position]];
    float2 uv;
};

struct InstanceData {
    float4x4 transform;
};

typedef struct
{
    array<texture2d<half>, 5> textures;
    uint32_t textureIndex;
} FragmentArgumentBuffer;

vertex Vertex texture_vertex(
    device const Vertex* vertices [[buffer(0)]],
    device const InstanceData* instanceData [[buffer(1)]],
    uint vid [[vertex_id]],
    uint iid [[instance_id]])
{
    Vertex vertexOut;
    vertexOut.position = instanceData[iid].transform * vertices[vid].position;
    vertexOut.uv = vertices[vid].uv;

    // Using KTX textures the orientation will be incompatible
    // Flip here. Maybe eventually 'ktx create' command will support specifying
    // the coordinate space.
    vertexOut.uv.y = 1.0 - vertexOut.uv.y;
    return vertexOut;
}

fragment half4 texture_fragment(Vertex vertexIn [[stage_in]], const device FragmentArgumentBuffer& argBuffer[[buffer(0)]])
{
    constexpr sampler colorSampler(mip_filter::linear,
                                       mag_filter::linear,
                                       min_filter::linear);

    half4 colorSample   = argBuffer.textures[argBuffer.textureIndex].sample(colorSampler, vertexIn.uv.xy);

    return half4(colorSample);
}

/*fragment half4 texture_fragment(Vertex vertexIn [[stage_in]], texture2d<half> colorMap [[texture(0)]])
{
    constexpr sampler colorSampler(mip_filter::linear,
                                       mag_filter::linear,
                                       min_filter::linear);

    half4 colorSample   = colorMap.sample(colorSampler, vertexIn.uv.xy);

    return half4(colorSample);
}*/
