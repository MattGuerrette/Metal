////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Keyboard.hpp"

bool Keyboard::isKeyClicked(SDL_Scancode key)
{
    return m_currentKeyState[key] && !m_previousKeyState[key];
}

bool Keyboard::isKeyPressed(SDL_Scancode key)
{
    return m_currentKeyState[key];
}

void Keyboard::registerKeyEvent(SDL_KeyboardEvent* event)
{
    m_currentKeyState[event->scancode] = event->down;
}

void Keyboard::update()
{
    m_previousKeyState = m_currentKeyState;
}
