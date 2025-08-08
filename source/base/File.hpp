////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_iostream.h>

namespace SDL
{
    struct IOStreamDeleter
    {
        void operator()(SDL_IOStream* stream) const
        {
            if (stream != nullptr)
            {
                SDL_CloseIO(stream);
            }
        }
    };
    using IOStreamPtr = std::unique_ptr<SDL_IOStream, IOStreamDeleter>;
}

class File
{
public:
    static std::expected<std::vector<uint8_t>, std::string> readBytesFromPath(
        std::string_view path);

    static std::expected<std::vector<uint8_t>, std::string> readTextFromPath(std::string_view path);
};
