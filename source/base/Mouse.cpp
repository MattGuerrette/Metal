////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Mouse.hpp"

Mouse::Mouse(SDL_Window* window)
	: Window(window)
{
	// Enable relative mouse location tracking for deltas
	//SDL_SetRelativeMouseMode(SDL_TRUE);
}

bool Mouse::LeftClick() const
{
	return !CurrentState[SDL_BUTTON_LEFT].Pressed && PreviousState[SDL_BUTTON_LEFT].Pressed;
}

bool Mouse::LeftDoubleClick() const
{
	return (CurrentState[SDL_BUTTON_LEFT].Pressed && CurrentState[SDL_BUTTON_LEFT].IsDoubleClick);
}

bool Mouse::LeftPressed() const
{
	return CurrentState[SDL_BUTTON_LEFT].Pressed;
}

bool Mouse::RightClick() const
{
	return !CurrentState[SDL_BUTTON_RIGHT].Pressed && PreviousState[SDL_BUTTON_RIGHT].Pressed;
}

bool Mouse::RightDoubleClick() const
{
	return (CurrentState[SDL_BUTTON_RIGHT].Pressed && CurrentState[SDL_BUTTON_RIGHT].IsDoubleClick);
}

int32_t Mouse::X() const
{
	return LocationX;
}

int32_t Mouse::Y() const
{
	return LocationY;
}

int32_t Mouse::RelativeX() const
{
	return XRelative;
}

int32_t Mouse::RelativeY() const
{
	return YRelative;
}

float Mouse::WheelX() const
{
	return PreciseWheelX;
}

float Mouse::WheelY() const
{
	return PreciseWheelY;
}

void Mouse::Warp()
{
	int32_t w, h;
	SDL_GetWindowSize(Window, &w, &h);
	SDL_WarpMouseInWindow(Window, w / 2, h / 2);
}

void Mouse::RegisterMouseMotion(SDL_MouseMotionEvent* event)
{
	LocationX = event->x;
	LocationY = event->y;
	XRelative = event->xrel;
	YRelative = event->yrel;
}

void Mouse::RegisterMouseWheel(SDL_MouseWheelEvent* event)
{
	PreciseWheelX = event->x;
	PreciseWheelY = event->y;
}

void Mouse::RegisterMouseButton(SDL_MouseButtonEvent* event)
{
	CurrentState[event->button] = { .IsDoubleClick = event->clicks > 1,
		.Pressed = event->state == SDL_PRESSED,
		.X = event->x,
		.Y = event->y };
}

void Mouse::Update()
{
	PreviousState = CurrentState;
	//CurrentState = {};
	XRelative = 0;
	YRelative = 0;
}