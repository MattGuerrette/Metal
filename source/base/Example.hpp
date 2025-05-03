
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

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

class Example : public CA::MetalDisplayLinkDelegate
{
public:
    Example(const char* title, uint32_t width, uint32_t height);

    ~Example() override;

    int run([[maybe_unused]] int argc, [[maybe_unused]] char** argv);

    void quit();

    void metalDisplayLinkNeedsUpdate(
        CA::MetalDisplayLink* displayLink, CA::MetalDisplayLinkUpdate* update) override;

protected:
    static constexpr int s_bufferCount = 3;
    static constexpr int s_multisampleCount = 4;

    virtual bool onLoad() = 0;

    virtual void onUpdate(const GameTimer& timer) = 0;

    virtual void onSetupUi(const GameTimer& timer);

    virtual void onResize(uint32_t width, uint32_t height) = 0;

    virtual void onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) = 0;

#ifdef SDL_PLATFORM_MACOS
    virtual NS::Menu* createMenuBar();
#endif

    SDL_Window*   m_window;
    SDL_MetalView m_view;

    // Keyboard and Mouse
    std::unique_ptr<class Keyboard> m_keyboard;
    std::unique_ptr<class Mouse>    m_mouse;
    std::unique_ptr<Gamepad>        m_gamepad;

    // Metal
    NS::SharedPtr<CA::MetalDisplayLink>   m_displayLink;
    NS::SharedPtr<MTL::Device>            m_device;
    NS::SharedPtr<MTL::CommandQueue>      m_commandQueue;
    NS::SharedPtr<MTL::Texture>           m_msaaTexture;
    NS::SharedPtr<MTL::Texture>           m_depthStencilTexture;
    NS::SharedPtr<MTL::DepthStencilState> m_depthStencilState;
    NS::SharedPtr<MTL::Library>           m_pipelineLibrary;
    MTL::PixelFormat                      m_frameBufferPixelFormat;

    // Sync primitives
    uint32_t             m_frameIndex = 0;
    dispatch_semaphore_t m_frameSemaphore;

private:
    uint32_t  m_width;
    uint32_t  m_height;
    GameTimer m_timer;
    bool      m_running;

    void createDepthStencil();
};
