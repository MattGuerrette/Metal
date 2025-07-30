
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

#include <SDL3/SDL.h>

#ifdef SDL_PLATFORM_MACOS
#include <AppKit/AppKit.hpp>
#endif

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "GameTimer.hpp"
#include "Gamepad.hpp"
#include "Keyboard.hpp"
#include "Mouse.hpp"

namespace SDL
{
    struct WindowDeleter
    {
        void operator()(SDL_Window* window) const
        {
            if (window != nullptr)
            {
                SDL_DestroyWindow(window);
            }
        }
    };
    using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;

    struct MetalViewDeleter
    {
        void operator()(void* view) const
        {
            if (view != nullptr)
            {
                SDL_Metal_DestroyView(view);
            }
        }
    };
    using MetalView = std::unique_ptr<void, MetalViewDeleter>;
}

class Example : public CA::MetalDisplayLinkDelegate
{
public:
    explicit Example(const char* title, int32_t width, int32_t height);

    ~Example() override;

    int run([[maybe_unused]] int argc, [[maybe_unused]] char** argv);

    void quit();

    void metalDisplayLinkNeedsUpdate(
        CA::MetalDisplayLink* displayLink, CA::MetalDisplayLinkUpdate* update) override;

protected:
    static constexpr int              s_bufferCount = 3;
    static constexpr int              s_multisampleCount = 4;
    static constexpr MTL::PixelFormat s_defaultPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;

    virtual bool onLoad() = 0;

    virtual void onUpdate(const GameTimer& timer) = 0;

    virtual void onSetupUi(const GameTimer& timer);

    virtual void onResize(uint32_t width, uint32_t height) = 0;

    virtual void onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) = 0;

    [[nodiscard]] SDL_Window* window() const;

    [[nodiscard]] int32_t windowWidth() const;

    [[nodiscard]] int32_t windowHeight() const;

    [[nodiscard]] const Keyboard& keyboard() const;

    [[nodiscard]] const Mouse& mouse() const;

    [[nodiscard]] uint32_t frameIndex() const;

    [[nodiscard]] MTL::Device* device() const;

    [[nodiscard]] MTL::CommandQueue* commandQueue() const;

    [[nodiscard]] MTL::DepthStencilState* depthStencilState() const;

    [[nodiscard]] MTL::Library* shaderLibrary() const;

#ifdef SDL_PLATFORM_MACOS
    [[nodiscard]] virtual NS::Menu* createMenuBar();
#endif

private:
    SDL::WindowPtr m_window;
    SDL::MetalView m_view;
    uint32_t       m_defaultWidth;
    uint32_t       m_defaultHeight;
    GameTimer      m_timer;
    bool           m_running;

#pragma region Input Handling
    std::unique_ptr<Keyboard> m_keyboard;
    std::unique_ptr<Mouse>    m_mouse;
    std::unique_ptr<Gamepad>  m_gamepad;
#pragma endregion

#pragma region Metal Resources
    NS::SharedPtr<CA::MetalDisplayLink>   m_displayLink;
    NS::SharedPtr<MTL::Device>            m_device;
    NS::SharedPtr<MTL::CommandQueue>      m_commandQueue;
    NS::SharedPtr<MTL::Texture>           m_msaaTexture;
    NS::SharedPtr<MTL::Texture>           m_depthStencilTexture;
    NS::SharedPtr<MTL::DepthStencilState> m_depthStencilState;
    NS::SharedPtr<MTL::Library>           m_shaderLibrary;
#pragma endregion

#pragma region Sync Primitives
    uint32_t             m_frameIndex = 0;
    dispatch_semaphore_t m_frameSemaphore;
#pragma endregion

    void createFrameResources(int32_t width, int32_t height);
};
