//
//  model.h
//  Metal
//
//  Created by Matthew Guerrette on 8/10/22.
//

#pragma once

#include <string>
#include <vector>

#import <Metal/Metal.h>

#include "graphics_math.h"

XM_ALIGNED_STRUCT(16) Vertex
{
    DirectX::XMFLOAT4 Position;
    DirectX::XMFLOAT4 Color;
    DirectX::XMFLOAT2 UV;
    DirectX::XMUINT4  Joints;
    DirectX::XMFLOAT4 Weights;
};

struct JointPose
{
    DirectX::XMFLOAT4 Rotation;
    DirectX::XMFLOAT4 Position;
    float32_t Scale;
};

struct Joint
{
    std::string Name;
    DirectX::XMMATRIX InverseBind;
    uint8_t ParentIndex;
};

struct Skeleton
{
    std::string Name;
    std::vector<Joint> Joints;
};

struct SkeletonPose
{
    std::shared_ptr<struct Skeleton> Skeleton;
    std::vector<JointPose> LocalPose;
};

struct AnimationSampler
{
    enum class Interpolation
    {
        Step,
        Linear,
        Cubic
    };
    enum Interpolation Interpolation;
    std::vector<float> Inputs;
    std::vector<DirectX::XMFLOAT4> Outputs;
};

struct AnimationChannel
{
    uint32_t SamplerIndex;
};

struct Animation
{
    std::vector<AnimationSampler> Samplers;
    std::vector<AnimationChannel> Channels;
};

class Model
{
    static constexpr uint32_t MAX_BONES_PER_VERTEX = 4;
public:
    Model(std::string file, id<MTLDevice> device);
    
    ~Model();
    
    const std::vector<Vertex>& GetVertices() const;
    
    const std::vector<uint16_t>& GetIndices() const;
    
    id<MTLTexture> GetTexture() const;
private:
    id<MTLBuffer> VertexBuffer;
    id<MTLBuffer> IndexBuffer;
    id<MTLTexture> Texture;
    std::shared_ptr<struct Skeleton> Skeleton;
    std::vector<Vertex> Vertices;
    std::vector<uint16_t> Indices;
    std::vector<Animation> Animations;
};
