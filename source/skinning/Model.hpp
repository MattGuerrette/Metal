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
	explicit Model(const std::string& fileName, MTL::Device* device);


	void Render(MTL::RenderCommandEncoder* commandEncoder);

private:
	NS::SharedPtr<MTL::Buffer> VertexBuffer;
	NS::SharedPtr<MTL::Buffer> IndexBuffer;
};
