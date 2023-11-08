////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

#include <Metal/Metal.hpp>

class Model
{
public:
	explicit Model(const std::string& fileName);

private:
	NS::SharedPtr<MTL::Buffer> VertexBuffer;
};
