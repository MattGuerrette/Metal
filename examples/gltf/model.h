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
};

class Model
{
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
    std::vector<Vertex> Vertices;
    std::vector<uint16_t> Indices;
};
