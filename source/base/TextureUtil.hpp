////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <Metal/Metal.hpp>

class TextureUtil
{
public:
	static MTL::Texture* LoadTextureFromFile(MTL::Device* device, const std::string& fileName);

	static MTL::Texture* LoadTextureFromBytes(MTL::Device* device, const void* bytes, size_t numBytes);
	
};
