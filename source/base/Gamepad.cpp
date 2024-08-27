////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2024.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Gamepad.hpp"

#include <fmt/format.h>

Gamepad::Gamepad(SDL_JoystickID id)
{
    SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
    if (gamepad == nullptr)
    {
        throw std::runtime_error(fmt::format("Failed to open gamepad: {}", SDL_GetError()));
    }

    Gamepad_.reset(SDL_OpenGamepad(id));
}

float Gamepad::LeftThumbstickHorizontal() const
{
    const auto axisValue = SDL_GetGamepadAxis(Gamepad_.get(), SDL_GAMEPAD_AXIS_LEFTX);
    return static_cast<float>(axisValue) / static_cast<float>(std::numeric_limits<int16_t>::max());
}

float Gamepad::LeftThumbstickVertical() const
{
    const auto axisValue = SDL_GetGamepadAxis(Gamepad_.get(), SDL_GAMEPAD_AXIS_LEFTY);
    return static_cast<float>(axisValue) / static_cast<float>(std::numeric_limits<int16_t>::max());
}