////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "FileUtil.hpp"

#include <Foundation/Foundation.hpp>

std::string FileUtil::PathForResource(const std::string& resourceName)
{
	auto path = NS::Bundle::mainBundle()->resourcePath();
	NS::String* resourcePath = path->stringByAppendingString(
		NS::String::string((std::string("/") + resourceName).c_str(), NS::ASCIIStringEncoding));

	std::string ret = resourcePath->utf8String();
	resourcePath->release();
	return ret;
}

NS::Data* FileUtil::DataForResource(const std::string& resourceName)
{
	auto filePath = FileUtil::PathForResource(resourceName);
	NS::String* nsFilePath = NS::String::string(filePath.c_str(), NS::UTF8StringEncoding);
	NS::Data* data = NS::Data::data(nsFilePath);
	nsFilePath->release();
	return data;
}