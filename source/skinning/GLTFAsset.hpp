////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2024.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <cgltf.h>

class GLTFAsset final
{
public:
    explicit GLTFAsset(const std::string& name);
    explicit GLTFAsset(const std::filesystem::path filePath);

    ~GLTFAsset();

    std::vector<std::string> animations() const;

    const cgltf_animation* getAnimation(int32_t index) const;

private:
    void Init(const std::filesystem::path& filePath);

    cgltf_data* m_data;
};