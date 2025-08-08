#include <metal_stdlib>

using namespace metal;

struct VertexIn {
    float3 position;
};

struct VertexOut {
    float4 position [[position]];
};

struct Uniforms {
    float4x4 modelViewProjectionMatrix;
};

vertex VertexOut triangle_vertex(
    device const VertexIn* vertices [[buffer(0)]],
    constant Uniforms* uniforms [[buffer(1)]],
    uint vid [[vertex_id]])
{
    VertexOut vertexOut;
    vertexOut.position = uniforms->modelViewProjectionMatrix * float4(vertices[vid].position, 1.0f);

    return vertexOut;
}

fragment half4 triangle_fragment(VertexIn vertexIn [[stage_in]])
{
    return half4(1.0f, 0.0f, 0.0f, 1.0f);
}
