////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Mouse.hpp"

Mouse::Mouse(SDL_Window* window)
    : m_window(window)
{
    // Enable relative mouse location tracking for deltas
    // SDL_SetRelativeMouseMode(SDL_TRUE);
}

bool Mouse::didLeftClick() const
{
    return !m_currentState[SDL_BUTTON_LEFT].isPressed && m_previousState[SDL_BUTTON_LEFT].isPressed;
}

bool Mouse::didLeftDoubleClick() const
{
    return (
        m_currentState[SDL_BUTTON_LEFT].isPressed && m_currentState[SDL_BUTTON_LEFT].isDoubleClick);
}

bool Mouse::isLeftPressed() const
{
    return m_currentState[SDL_BUTTON_LEFT].isPressed;
}

bool Mouse::didRightClick() const
{
    return !m_currentState[SDL_BUTTON_RIGHT].isPressed
        && m_previousState[SDL_BUTTON_RIGHT].isPressed;
}

bool Mouse::didRightDoubleClick() const
{
    return (m_currentState[SDL_BUTTON_RIGHT].isPressed
        && m_currentState[SDL_BUTTON_RIGHT].isDoubleClick);
}

int32_t Mouse::x() const
{
    return m_locationX;
}

int32_t Mouse::y() const
{
    return m_locationY;
}

int32_t Mouse::relativeX() const
{
    return m_relativeX;
}

int32_t Mouse::relativeY() const
{
    return m_relativeY;
}

float Mouse::wheelX() const
{
    return m_preciseWheelX;
}

float Mouse::wheelY() const
{
    return m_preciseWheelY;
}

void Mouse::warp()
{
    int32_t w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    SDL_WarpMouseInWindow(m_window, w / 2, h / 2);
}

void Mouse::registerMouseMotion(SDL_MouseMotionEvent* event)
{
    m_locationX = event->x;
    m_locationY = event->y;
    m_relativeX = event->xrel;
    m_relativeY = event->yrel;
}

void Mouse::registerMouseWheel(SDL_MouseWheelEvent* event)
{
    m_preciseWheelX = event->x;
    m_preciseWheelY = event->y;
}

void Mouse::registerMouseButton(SDL_MouseButtonEvent* event)
{
    m_currentState[event->button] = { .isDoubleClick = event->clicks > 1,
        .isPressed = event->state == SDL_PRESSED,
        .x = event->x,
        .y = event->y };
}

void Mouse::update()
{
    m_previousState = m_currentState;
    // CurrentState = {};
    m_relativeX = 0;
    m_relativeY = 0;
}