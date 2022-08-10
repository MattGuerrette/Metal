#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float4 position [[position]];
    float4 color;
    float2 uv;
};

struct InstanceData {
    float4x4 transform;
};

vertex Vertex vertex_project(
    device const Vertex* vertices [[buffer(0)]],
    device const InstanceData* instanceData [[buffer(1)]],
    uint vid [[vertex_id]],
    uint iid [[instance_id]])
{
    //float4 position = vertices[vid.position];
    
    Vertex vertexOut;
    vertexOut.position = instanceData[iid].transform * vertices[vid].position;
    vertexOut.color = vertices[vid].color;
    vertexOut.color *= 1;
    vertexOut.uv = vertices[vid].uv;

    return vertexOut;
}

fragment half4 fragment_flatcolor(Vertex vertexIn [[stage_in]],
                                  texture2d< half, access::sample > tex [[texture(0)]])
{
    constexpr sampler s( address::repeat, filter::linear );
    half3 color = tex.sample(s, vertexIn.uv).rgb;
    
    return half4(color, 1.0f);
    //return half4(1.0, 0.0, 0.0, 1.0);
    //return half4(vertexIn.color);
}
