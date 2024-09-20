
#include <metal_stdlib>

using namespace metal;

struct Vertex {
    float4 position [[position]];
    float4 color;
    float2 texcoord;
    float4 joint;
    float4 weight;
};

struct InstanceData {
    float4x4 transform;
};

struct ArgumentBuffer {
    device InstanceData* data;
    float4x4 bones[1];
};

vertex Vertex skinning_vertex(
    device const Vertex* vertices [[buffer(0)]],
    const device ArgumentBuffer& argBuffer[[buffer(1)]],
    uint vid [[vertex_id]],
    uint iid [[instance_id]])
{
//    float4x4 skinMatrix =
//    vertices[vid].weight.x * argBuffer.bones[int(vertices[vid].joint.x)] +
//    vertices[vid].weight.y * argBuffer.bones[int(vertices[vid].joint.y)] +
//    vertices[vid].weight.z * argBuffer.bones[int(vertices[vid].joint.z)] +
//    vertices[vid].weight.w * argBuffer.bones[int(vertices[vid].joint.w)];
    
    float4x4 skinMatrix = argBuffer.bones[0];
    
    float4x4 identity_matrix = float4x4(
            float4(1.0, 0.0, 0.0, 0.0),
            float4(0.0, 1.0, 0.0, 0.0),
            float4(0.0, 0.0, 1.0, 0.0),
            float4(0.0, 0.0, 0.0, 1.0)
        );
    
    
    Vertex vertexOut;
    float4 worldPosition = vertices[vid].position;
    vertexOut.position = argBuffer.data[iid].transform * worldPosition;
    vertexOut.color = vertices[vid].color;
    vertexOut.color *= 1;
    vertexOut.texcoord = vertices[vid].texcoord;
    
    //vertexOut.texcoord.y = 1.0 - vertexOut.texcoord.y;

    return vertexOut;
}

fragment half4 skinning_fragment(Vertex vertexIn [[stage_in]], texture2d< half, access::sample > tex [[texture(0)]],
    const device ArgumentBuffer& argBuffer[[buffer(0)]])
{
    constexpr sampler s( address::repeat, filter::linear );
    half4 color = tex.sample(s, vertexIn.texcoord).rgba;

    //return half4(color);
    return half4(1.0f, 0.0f, 0.0f, 1.0f);
}
