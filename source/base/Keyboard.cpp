////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Keyboard.hpp"

bool Keyboard::isKeyClicked(SDL_Scancode key) const
{
    if (m_currentKeyState.contains(key) && m_previousKeyState.contains(key))
    {
        return m_currentKeyState.at(key) && !m_previousKeyState.at(key);
    }
    
    return false;
}

bool Keyboard::isKeyPressed(SDL_Scancode key) const
{
    if (m_currentKeyState.contains(key))
    {
        return m_currentKeyState.at(key);
    }
    
    return false;
}

void Keyboard::registerKeyEvent(SDL_KeyboardEvent* event)
{
    m_currentKeyState[event->scancode] = event->down;
}

void Keyboard::update()
{
    m_previousKeyState = m_currentKeyState;
}
