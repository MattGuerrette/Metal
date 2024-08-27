////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2024.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>

#include <SDL3/SDL_gamepad.h>

class Gamepad
{
    struct GamepadDeleter
    {
        void operator()(SDL_Gamepad* gamepad)
        {
            if (gamepad != nullptr)
            {
                SDL_CloseGamepad(gamepad);
            }
        }
    };
    using UniqueGamepad = std::unique_ptr<SDL_Gamepad, GamepadDeleter>;

public:
    explicit Gamepad(SDL_JoystickID id);

    float LeftThumbstickHorizontal() const;

    float LeftThumbstickVertical() const;

private:
    UniqueGamepad Gamepad_;
};