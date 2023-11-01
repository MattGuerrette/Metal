////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
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
	CurrentKeyState[event->keysym.scancode] = event->state == SDL_PRESSED;
}

void Keyboard::Update()
{
	PreviousKeyState = CurrentKeyState;
}