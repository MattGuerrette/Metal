////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2024.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "GLTFAsset.hpp"

#include <filesystem>

#include "File.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <fmt/format.h>

#include <SDL3/SDL_storage.h>

GLTFAsset::GLTFAsset(const std::string& name)
{
    cgltf_options         options = {};
    const auto            resourcePath = SDL_GetBasePath();
    std::filesystem::path path = resourcePath;
    path.append(name);

    init(path);
}

GLTFAsset::GLTFAsset(const std::filesystem::path& filePath)
{
    init(filePath);
}

void GLTFAsset::init(const std::filesystem::path& filePath)
{
    cgltf_data*   data = nullptr;
    cgltf_options options = {};
    cgltf_result  result = cgltf_parse_file(&options, filePath.c_str(), &data);
    if (result != cgltf_result_success)
    {
        throw std::runtime_error("Failed to parse GLTF asset");
    }
    m_data.reset(data);

    result = cgltf_load_buffers(&options, m_data.get(), filePath.c_str());
    if (result != cgltf_result_success)
    {
        throw std::runtime_error("Failed to load buffers");
    }
}

std::vector<std::string> GLTFAsset::animations() const
{
    assert(m_data != nullptr);

    std::vector<std::string> result;
    for (uint32_t i = 0; i < m_data->animations_count; ++i)
    {
        const cgltf_animation* animation = &m_data->animations[i];
        if (animation->name)
        {
            result.emplace_back(animation->name);
        }
        else
        {
            // Generate animation name
            result.emplace_back(fmt::format("Animation {}", i));
        }
    }

    return result;
}

const cgltf_animation* GLTFAsset::getAnimation(int32_t index) const
{
    assert(m_data != nullptr);

    return &m_data->animations[index];
}

void GLTFAsset::initDeviceResources()
{
    // TODO: Load device resources such as pipelines and GPU buffers

    for (uint32_t i = 0; i < m_data->meshes_count; ++i)
    {
        const cgltf_mesh* mesh = &m_data->meshes[i];
    }
}

void GLTFAsset::render(MTL::RenderCommandEncoder* commandEncoder)
{
    // TODO: Draw meshes using command encoder
}