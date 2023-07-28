#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float4 position [[position]];
    float4 color;
};

struct InstanceData {
    float4x4 transform;
};

vertex Vertex instancing_vertex(
    device const Vertex* vertices [[buffer(0)]],
    device const InstanceData* instanceData [[buffer(1)]],
    uint vid [[vertex_id]],
    uint iid [[instance_id]])
{
    Vertex vertexOut;
    vertexOut.position = instanceData[iid].transform * vertices[vid].position;
    vertexOut.color = vertices[vid].color;
    vertexOut.color *= 1;

    return vertexOut;
}

fragment half4 instancing_fragment(Vertex vertexIn [[stage_in]])
{
    return half4(vertexIn.color);
}
