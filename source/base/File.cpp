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
    std::string PathForResource(const std::string resourceName)
    {
        std::filesystem::path resourcePath = SDL_GetBasePath();
        resourcePath.append(resourceName);

        return resourcePath.string();
    }
} // namespace

File::File(const std::string& fileName)
{
    const auto filePath = PathForResource(fileName);

    Stream_ = SDL_IOFromFile(filePath.c_str(), "rb");
    if (Stream_ == nullptr)
    {
        throw std::runtime_error(
            fmt::format("Failed to open {} for read. SDL_Error: {}", fileName, SDL_GetError()));
    }
}

File::~File()
{
    if (Stream_ != nullptr)
    {
        SDL_CloseIO(Stream_);
        Stream_ = nullptr;
    }
}

SDL_IOStream* File::Stream() const
{
    return Stream_;
}

std::vector<std::byte> File::ReadAll() const
{
    SDL_SeekIO(Stream_, 0, SDL_IO_SEEK_END);
    const auto numBytes = SDL_TellIO(Stream_);
    SDL_SeekIO(Stream_, 0, SDL_IO_SEEK_SET);

    std::vector<std::byte> result = {};
    result.resize(numBytes);

    size_t numBytesRead;
    void*  data = SDL_LoadFile_IO(Stream_, &numBytesRead, SDL_FALSE);
    if (data == nullptr)
    {
        return {};
    }

    memcpy(&result[0], data, numBytesRead);
    SDL_free(data);

    return result;
}