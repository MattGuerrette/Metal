////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

#include <Foundation/Foundation.hpp>

class FileUtil
{
public:
	static std::string PathForResource(const std::string& resourceName);

	static NS::Data* DataForResource(const std::string& resourceName);
};
