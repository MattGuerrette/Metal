////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Metal/Metal.hpp>

class SpriteBatch final
{
    static constexpr uint32_t MAX_SPRITE_COUNT = 65536;

public:
    SpriteBatch(MTL::Device* device,;

	~SpriteBatch() = default;


private:
	NS::SharedPtr<MTL::Buffer> ArgumentBuffer;
	NS::SharedPtr<MTL::Buffer> InstanceBuffer;
};
