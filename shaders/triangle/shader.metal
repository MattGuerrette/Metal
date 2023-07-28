#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float4 position [[position]];
    float4 color;
};

struct Uniforms {
    float4x4 modelViewProjectionMatrix;
};

vertex Vertex triangle_vertex(
    device const Vertex* vertices [[buffer(0)]],
    constant Uniforms* uniforms [[buffer(1)]],
    uint vid [[vertex_id]])
{
    Vertex vertexOut;
    vertexOut.position = uniforms->modelViewProjectionMatrix * vertices[vid].position;
    vertexOut.color = vertices[vid].color;
    vertexOut.color *= 1;

    return vertexOut;
}

fragment half4 triangle_fragment(Vertex vertexIn [[stage_in]])
{
    return half4(vertexIn.color);
}
