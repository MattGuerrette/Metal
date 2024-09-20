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
    std::string pathForResource(const std::string& resourceName)
    {
        std::filesystem::path resourcePath = SDL_GetBasePath();
        resourcePath.append(resourceName);

        return resourcePath.string();
    }
} // namespace

File::File(const std::string& fileName)
{
    const auto filePath = pathForResource(fileName);

    m_stream.reset(SDL_IOFromFile(filePath.c_str(), "rb"));
    if (m_stream == nullptr)
    {
        throw std::runtime_error(
            fmt::format("Failed to open {} for read. SDL_Error: {}", fileName, SDL_GetError()));
    }
}

SDL_IOStream* File::stream() const
{
    return m_stream.get();
}

std::vector<std::byte> File::readAll() const
{
    SDL_SeekIO(m_stream.get(), 0, SDL_IO_SEEK_END);
    const auto numBytes = SDL_TellIO(m_stream.get());
    SDL_SeekIO(m_stream.get(), 0, SDL_IO_SEEK_SET);

    std::vector<std::byte> result = {};
    result.resize(numBytes);

    size_t numBytesRead;
    void*  data = SDL_LoadFile_IO(m_stream.get(), &numBytesRead, false);
    if (data == nullptr)
    {
        return {};
    }

    memcpy(result.data(), data, numBytesRead);
    SDL_free(data);

    return result;
}