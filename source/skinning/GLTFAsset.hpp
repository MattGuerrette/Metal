////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2024.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <Metal/Metal.hpp>

#include <cgltf.h>

struct MeshGeometry
{
    NS::SharedPtr<MTL::Buffer> vertexBuffer;
    NS::SharedPtr<MTL::Buffer> indexBuffer;
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
    explicit GLTFAsset(const std::string& name);
    explicit GLTFAsset(const std::filesystem::path& filePath);

    [[nodiscard]] std::vector<std::string> animations() const;

    [[nodiscard]] const cgltf_animation* getAnimation(int32_t index) const;

    void initDeviceResources();

    void render(MTL::RenderCommandEncoder* commandEncoder);

private:
    void init(const std::filesystem::path& filePath);

    UniqueData                m_data;
    std::vector<MeshGeometry> m_meshes;
};