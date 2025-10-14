////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>

#include <SDL3/SDL.h>

class Keyboard final
{
    using KeyState = std::map<SDL_Scancode, bool>;

public:
    /// @brief Checks if specified key was clicked this frame.
    /// @param [in] key The clicked key.
    /// @return True if key was clicked, false otherwise.
    bool isKeyClicked(SDL_Scancode key) const;

    /// @brief Checks if specified key is pressed this frame.
    /// @param [in] key The pressed key.
    /// @return True if pressed, false otherwise.
    bool isKeyPressed(SDL_Scancode key) const;

    /// @brief Registers key event.
    /// @param [in] event The key event.
    void registerKeyEvent(SDL_KeyboardEvent* event);

    /// @brief Updates the state cache for next frame.
    void update();

private:
    KeyState m_previousKeyState; ///< Previous frame key-state.
    KeyState m_currentKeyState;  ///< Current frame key-state.
};
