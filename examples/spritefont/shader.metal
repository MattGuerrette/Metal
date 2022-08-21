#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float4 position [[position]];
    float4 color;
    float2 uv;
};

struct Uniforms {
    float4x4 modelViewProjectionMatrix;
};

vertex Vertex vertex_project(
    device const Vertex* vertices [[buffer(0)]],
    constant Uniforms* uniforms [[buffer(1)]],
    uint vid [[vertex_id]])
{
    Vertex vertexOut;
    vertexOut.position = uniforms->modelViewProjectionMatrix * vertices[vid].position;
    vertexOut.color = vertices[vid].color;
    vertexOut.color *= 1;
    vertexOut.uv = vertices[vid].uv;

    return vertexOut;
}

fragment half4 fragment_flatcolor(Vertex vertexIn [[stage_in]],
                                  texture2d< half, access::sample > tex [[texture(0)]])
{
    constexpr sampler s( address::repeat, filter::linear );
    half4 color = tex.sample(s, vertexIn.uv).rgba;
    
    return half4(color);
}
