////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL_iostream.h>

class File
{
public:
	/// @brief Opens specified file for reading/
	/// @note This file should be located within the app resource folder.
	/// @param [in] fileName The file located in resource folder to open for reading.
	explicit File(const std::string& fileName);
	File(const File& file) = delete;
	File& operator=(const File& file) = delete;

	/// @brief Destructor which closes the IO stream.
	~File();

    /// @brief Accesses the SDL IO stream handle.
    /// @return The SDL_IOStream handle.
    [[nodiscard]] SDL_IOStream* Stream() const;

	/// @brief Reads all bytes from file.
	/// @return File contents as a vector of bytes.
	[[nodiscard]] std::vector<std::byte> ReadAll() const;

private:
	SDL_IOStream* Stream_; ///< SDL Read/Write stream handle.
};