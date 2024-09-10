////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Keyboard.hpp"

bool Keyboard::IsKeyClicked(SDL_Scancode key)
{
    return CurrentKeyState[key] && !PreviousKeyState[key];
}

bool Keyboard::IsKeyPressed(SDL_Scancode key)
{
    return CurrentKeyState[key];
}

void Keyboard::RegisterKeyEvent(SDL_KeyboardEvent* event)
{
    CurrentKeyState[event->scancode] = event->down;
}

void Keyboard::Update()
{
    PreviousKeyState = CurrentKeyState;
}