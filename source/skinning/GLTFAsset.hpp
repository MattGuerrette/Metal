////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2024.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include <cgltf.h>

#include "GraphicsMath.hpp"
#include "StaticGeometry.hpp"

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector4 position;
    Vector4 color;
    Vector2 texcoord;
    Vector4 joint;
    Vector4 weight;
};

struct Material
{
    explicit Material(MTK::TextureLoader* textureLoader, const cgltf_material* material);

    [[nodiscard]] MTL::Texture* baseColorTexture() const;

private:
    NS::SharedPtr<MTL::Texture> m_baseColorTexture;
    //    std::shared_ptr<const Asset::Texture> MetallicRoughnessTexture_;
    //    Vector4                               BaseColorFactor_;
    //    float                                 MetallicFactor_;
    //    float                                 RoughnessFactor_;
    //    float                                 SpecularFactor_;
    //    Vector4                               SpecularColorFactor_;
    //    std::shared_ptr<const Asset::Texture> SpecularTexture_;
    //    std::shared_ptr<Asset::Texture>       EmissiveTexture_;
    //    Vector3                               EmissiveFactor_;
    //    std::shared_ptr<Asset::Texture>       OcclusionTexture_;
    //    std::shared_ptr<Asset::Texture>       NormalTexture_;
    //    float                                 IndexOfRefraction_;
};

struct Primitive
{
    explicit Primitive(MTL::Device* device, const cgltf_primitive* prim);
    std::unique_ptr<StaticGeometry<Vertex>> geometry;
    std::unique_ptr<Material>               material;
};

struct Mesh
{
    explicit Mesh(MTL::Device* device, const cgltf_mesh* mesh);
    std::vector<Primitive> primitives;

    BoundingBox boundingBox() const;
};

class GLTFAsset final
{
    struct DataDeleter
    {
        void operator()(cgltf_data* data)
        {
            if (data != nullptr)
            {
                cgltf_free(data);
            }
        }
    };
    using UniqueData = std::unique_ptr<cgltf_data, DataDeleter>;

public:
    explicit GLTFAsset(MTL::Device* device, const std::string& name);
    explicit GLTFAsset(MTL::Device* device, const std::filesystem::path& filePath);

    [[nodiscard]] std::vector<Matrix>      boneMatricesForAnimation(int32_t animation) const;
    [[nodiscard]] std::vector<std::string> animations() const;

    [[nodiscard]] const cgltf_animation* getAnimation(int32_t index) const;

    void initDeviceResources();

    void drawUi();
    void render(MTL::RenderCommandEncoder* commandEncoder);

private:
    void init(const std::filesystem::path& filePath);

    UniqueData        m_data;
    std::vector<Mesh> m_meshes;
    MTL::Device*      m_device;
};
