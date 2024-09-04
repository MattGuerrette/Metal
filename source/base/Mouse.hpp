////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>

#include <SDL3/SDL.h>

class Mouse final
{
    struct ButtonState
    {
        bool  isDoubleClick;
        bool  isPressed;
        float x;
        float y;
    };
    static constexpr int32_t s_mouseButtons = 3;
    using MouseButtonState = std::array<ButtonState, s_mouseButtons>;

public:
    /// @brief Constructor
    /// @param [in] window The window used for warping mouse cursor.
    explicit Mouse(SDL_Window* window);

    /// @brief Checks if a left mouse button click has occurred.
    /// @return True if left button clicked, false otherwise.
    [[nodiscard]] bool didLeftClick() const;

    /// @brief Checks if a left mouse button double-click as occurred.
    /// @return True if left button double-clicked, false otherwise.
    [[nodiscard]] bool didLeftDoubleClick() const;

    /// @brief Checks if a right mouse button click has occurred.
    /// @return True if right button clicked, false otherwise.
    [[nodiscard]] bool didRightClick() const;

    /// @brief Checks if left mouse button is currently pressed
    /// @return True if pressed, false otherwise
    [[nodiscard]] bool isLeftPressed() const;

    /// @brief Checks if a right mouse button double-click has occurred.
    /// @return True if right button double-clicked, false otherwise.
    [[nodiscard]] bool didRightDoubleClick() const;

    /// @brief Gets the mouse X coordinate location.
    /// @return X coordinate.
    [[nodiscard]] int32_t x() const;

    /// @brief Gets the mouse Y coordinate location.
    /// @return Y coordinate.
    [[nodiscard]] int32_t y() const;

    /// @brief Gets the mouse relative movement in X coordinate.
    /// @return Relative mouse movement in X coordinate space.
    [[nodiscard]] int32_t relativeX() const;

    /// @brief Gets the mouse relative movement in Y coordinate.
    /// @return Relative mouse movement in Y coordinate space.
    [[nodiscard]] int32_t relativeY() const;

    /// @brief Gets the precise scroll-wheel movement in X axis.
    /// @return Precise scroll-wheel movement in X axis.
    [[nodiscard]] float wheelX() const;

    /// @brief Gets the precise scroll-wheel movement in Y axis.
    /// @return Precise scroll-wheel movement in Y axis.
    [[nodiscard]] float wheelY() const;

    /// @brief Constrains (warps) the mouse cursor to center of window.
    void warp();

    /// @brief Registers mouse motion event.
    /// @param [in] event The mouse motion event.
    void registerMouseMotion(SDL_MouseMotionEvent* event);

    /// @brief Register mouse wheel event.
    /// @param [in] event The mouse wheel event.
    void registerMouseWheel(SDL_MouseWheelEvent* event);

    /// @brief Registers mouse button event.
    /// @param [in] event The mouse button event.
    void registerMouseButton(SDL_MouseButtonEvent* event);

    /// @brief Updates the state cache for next frame.
    void update();

private:
    SDL_Window*      m_window;           ///< Window used to warp cursor to.
    float            m_locationX {};     ///< X mouse location.
    float            m_locationY {};     ///< Y mouse location.
    int32_t          m_relativeX {};     ///< X mouse location relative to last frame.
    int32_t          m_relativeY {};     ///< Y mouse location relative to last frame.
    float            m_preciseWheelX {}; ///< Scroll-wheel delta X (precise).
    float            m_preciseWheelY {}; ///< Scroll-wheel delta Y (precise).
    MouseButtonState m_currentState {};  ///< Current frame mouse button state.
    MouseButtonState m_previousState {}; ///< Previous frame mouse button state.
};