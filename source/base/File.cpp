////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "File.hpp"

#include <filesystem>

#include <fmt/core.h>

#include <SDL3/SDL_storage.h>

namespace
{
    std::string pathForResource(const std::string_view resource)
    {
        std::filesystem::path resourcePath = SDL_GetBasePath();
        resourcePath.append(resource);

        return resourcePath.string();
    }
} // namespace
std::expected<std::vector<uint8_t>, std::string> File::readBytesFromPath(
    const std::string_view path)
{
    const std::filesystem::path basePath = SDL_GetBasePath();
    const std::filesystem::path fullPath = basePath / path;
    SDL_IOStream*               stream = SDL_IOFromFile(fullPath.string().c_str(), "rb");
    if (!stream)
    {
        return std::unexpected("Failed to open file stream");
    }

    size_t numBytesRead;
    void*  data = SDL_LoadFile_IO(stream, &numBytesRead, true);
    if (data == nullptr)
    {
        return std::unexpected("Failed to read from file stream");
    }

    std::vector<uint8_t> bytes(numBytesRead);
    memcpy(bytes.data(), data, numBytesRead);
    SDL_free(data);

    return std::move(bytes);
}

std::expected<std::vector<uint8_t>, std::string> File::readTextFromPath(const std::string_view path)
{
    const std::filesystem::path basePath = SDL_GetBasePath();
    const std::filesystem::path fullPath = basePath / path;
    SDL_IOStream*               stream = SDL_IOFromFile(fullPath.string().c_str(), "r");
    if (!stream)
    {
        return std::unexpected("Failed to open file stream");
    }

    size_t numBytesRead;
    void*  data = SDL_LoadFile_IO(stream, &numBytesRead, true);
    if (data == nullptr)
    {
        return std::unexpected("Failed to read from file stream");
    }

    std::vector<uint8_t> bytes(numBytesRead);
    memcpy(bytes.data(), data, numBytesRead);
    SDL_free(data);

    return std::move(bytes);
}
