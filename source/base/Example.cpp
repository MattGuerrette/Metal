
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Example.hpp"

#include <fmt/core.h>

#include <filesystem>
#include <memory>

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_impl_sdl3.h"

#include "File.hpp"
#include "GraphicsMath.hpp"

Example::Example(const char* title, int32_t width, int32_t height)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.IniSavingRate = 0.0F;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    ImGui::StyleColorsDark();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
    {
        throw std::runtime_error(fmt::format("Failed to initialize SDL: {}", SDL_GetError()));
    }

    int   numDisplays = 0;
    auto* displays = SDL_GetDisplays(&numDisplays);
    assert(numDisplays != 0);

    const auto        display = displays[0];
    const auto* const mode = SDL_GetDesktopDisplayMode(display);
    int32_t           screenWidth = mode->w;
    int32_t           screenHeight = mode->h;
    SDL_free(displays);

    int flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_METAL;
#ifdef SDL_PLATFORM_MACOS
    flags ^= SDL_WINDOW_FULLSCREEN;
    flags |= SDL_WINDOW_RESIZABLE;
    screenWidth = width;
    screenHeight = height;
#endif

    m_window.reset(SDL_CreateWindow(title, screenWidth, screenHeight, flags));
    if (m_window == nullptr)
    {
        throw std::runtime_error(fmt::format("Failed to create SDL window: {}", SDL_GetError()));
    }
    m_view.reset(SDL_Metal_CreateView(m_window.get()));
    m_running = true;

    m_defaultWidth = width;
    m_defaultHeight = height;

    m_device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

    auto* layer = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(m_view.get()));
    layer->setPixelFormat(s_defaultPixelFormat);
    layer->setDevice(m_device.get());

    ImGui_ImplMetal_Init(m_device.get());
    ImGui_ImplSDL3_InitForMetal(m_window.get());

    m_commandQueue = NS::TransferPtr(m_device->newMTL4CommandQueue());
    m_commandBuffer = NS::TransferPtr(m_device->newCommandBuffer());

    for (uint32_t i = 0; i < s_bufferCount; i++)
    {
        m_commandAllocator[i] = NS::TransferPtr(m_device->newCommandAllocator());
    }

    m_sharedEvent = NS::TransferPtr(m_device->newSharedEvent());
    m_sharedEvent->setSignaledValue(m_frameNumber);
    m_currentFrameIndex = 0;

    createFrameResources(windowWidth(), windowHeight());

    // Create a depth stencil state
    const NS::SharedPtr<MTL::DepthStencilDescriptor> depthStencilDescriptor
        = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);

    m_depthStencilState
        = NS::TransferPtr(m_device->newDepthStencilState(depthStencilDescriptor.get()));

    // Load shader Library
    // TODO: Showcase how to use Metal archives to erase compilation
    m_shaderLibrary = NS::TransferPtr(m_device->newDefaultLibrary());

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>(m_window.get());

    m_timer.setFixedTimeStep(false);
    m_timer.resetElapsedTime();

    m_displayLink = NS::TransferPtr(CA::MetalDisplayLink::alloc()->init(layer));
    // Enable 120HZ refresh for devices that support Pro Motion
    m_displayLink->setPreferredFrameRateRange({ 60, mode->refresh_rate, mode->refresh_rate });
    m_displayLink->setDelegate(this);
}

Example::~Example()
{
    // Cleanup
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    m_window.reset();

    SDL_Quit();
}

void Example::createFrameResources(const int32_t width, const int32_t height)
{
    m_msaaTexture.reset();
    m_depthStencilTexture.reset();

    // Create a multisample texture
    const NS::SharedPtr<MTL::TextureDescriptor> msaaTextureDescriptor
        = NS::RetainPtr(MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatBGRA8Unorm_sRGB, width, height, false));
    msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    msaaTextureDescriptor->setSampleCount(s_multisampleCount); // Set sample count for MSAA
    msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    msaaTextureDescriptor->setStorageMode(MTL::StorageModePrivate);

    m_msaaTexture = NS::TransferPtr(m_device->newTexture(msaaTextureDescriptor.get()));

    // Create a depth stencil texture
    const NS::SharedPtr<MTL::TextureDescriptor> textureDescriptor
        = NS::RetainPtr(MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatDepth32Float_Stencil8, width, height, false));
    textureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    textureDescriptor->setSampleCount(s_multisampleCount);
    textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    textureDescriptor->setResourceOptions(
        MTL::ResourceOptionCPUCacheModeDefault | MTL::ResourceStorageModePrivate);
    textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);

    m_depthStencilTexture = NS::TransferPtr(m_device->newTexture(textureDescriptor.get()));
}

const Keyboard& Example::keyboard() const
{
    return *m_keyboard;
}

const Mouse& Example::mouse() const
{
    return *m_mouse;
}

MTL::Device* Example::device() const
{
    return m_device.get();
}

MTL4::CommandQueue* Example::commandQueue() const
{
    return m_commandQueue.get();
}

MTL::DepthStencilState* Example::depthStencilState() const
{
    return m_depthStencilState.get();
}

MTL::Library* Example::shaderLibrary() const
{
    return m_shaderLibrary.get();
}

CA::MetalLayer* Example::metalLayer() const
{
    return (CA::MetalLayer*)SDL_Metal_GetLayer(m_view.get());
}

MTL4::RenderPassDescriptor* Example::defaultRenderPassDescriptor(CA::MetalDrawable* drawable) const
{
    MTL4::RenderPassDescriptor* passDescriptor = MTL4::RenderPassDescriptor::alloc()->init();
    passDescriptor->colorAttachments()->object(0)->setResolveTexture(drawable->texture());
    passDescriptor->colorAttachments()->object(0)->setTexture(msaaTexture());
    passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    passDescriptor->colorAttachments()->object(0)->setStoreAction(
        MTL::StoreActionMultisampleResolve);
    passDescriptor->colorAttachments()->object(0)->setClearColor(
        MTL::ClearColor(DirectX::Colors::CornflowerBlue.f[0], DirectX::Colors::CornflowerBlue.f[1],
            DirectX::Colors::CornflowerBlue.f[2], 1.0));
    passDescriptor->depthAttachment()->setTexture(depthStencilTexture());
    passDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    passDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    passDescriptor->depthAttachment()->setClearDepth(1.0);
    passDescriptor->stencilAttachment()->setTexture(depthStencilTexture());
    passDescriptor->stencilAttachment()->setLoadAction(MTL::LoadActionClear);
    passDescriptor->stencilAttachment()->setStoreAction(MTL::StoreActionDontCare);
    passDescriptor->stencilAttachment()->setClearStencil(0);

    return passDescriptor;
}

MTL4::CommandBuffer* Example::commandBuffer() const
{
    return m_commandBuffer.get();
}

MTL4::CommandAllocator* Example::commandAllocator() const
{
    return m_commandAllocator[m_currentFrameIndex].get();
}

MTL::Texture* Example::msaaTexture() const
{
    return m_msaaTexture.get();
}

MTL::Texture* Example::depthStencilTexture() const
{
    return m_depthStencilTexture.get();
}

SDL_Window* Example::window() const
{
    return m_window.get();
}

int32_t Example::windowWidth() const
{
    int32_t width = 0;
    SDL_GetWindowSizeInPixels(m_window.get(), &width, nullptr);
    return width;
}

int32_t Example::windowHeight() const
{
    int32_t height = 0;
    SDL_GetWindowSizeInPixels(m_window.get(), nullptr, &height);
    return height;
}

uint32_t Example::frameIndex() const
{
    return m_currentFrameIndex;
}

#ifdef SDL_PLATFORM_MACOS
NS::Menu* Example::createMenuBar()
{
    return nullptr;
}
#endif

int Example::run([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
#ifdef SDL_PLATFORM_MACOS
    if (const NS::Menu* menu = createMenuBar(); menu != nullptr)
    {
        NS::Application::sharedApplication()->setMainMenu(menu);
    }
#endif
    if (!onLoad())
    {
        return EXIT_FAILURE;
    }

    m_displayLink->addToRunLoop(NS::RunLoop::mainRunLoop(), NS::DefaultRunLoopMode);

    while (m_running)
    {
        SDL_Event e;
        if (SDL_WaitEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT)
            {
                m_running = false;
                break;
            }

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                const float density = SDL_GetWindowPixelDensity(m_window.get());
                auto*       layer = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(m_view.get()));
                layer->setDrawableSize(CGSize { static_cast<float>(e.window.data1) * density,
                    static_cast<float>(e.window.data2) * density });

                //                ImGuiIO& io = ImGui::GetIO();
                //                io.DisplaySize =
                //                    ImVec2((float)e.window.data1 * density, (float)e.window.data2
                //                    * density);

                onResize(e.window.data1, e.window.data2);
            }

            if (e.type == SDL_EVENT_JOYSTICK_ADDED)
            {
                if (SDL_IsGamepad(e.jdevice.which))
                {
                    m_gamepad = std::make_unique<Gamepad>(e.jdevice.which);
                }
            }

            if (e.type == SDL_EVENT_JOYSTICK_REMOVED)
            {
                if (SDL_IsGamepad(e.jdevice.which))
                {
                    m_gamepad.reset();
                }
            }

            if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP)
            {
                m_keyboard->registerKeyEvent(&e.key);
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                m_mouse->registerMouseButton(&e.button);
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                m_mouse->registerMouseMotion(&e.motion);
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL)
            {
                m_mouse->registerMouseWheel(&e.wheel);
            }

            if (m_keyboard->isKeyClicked(SDL_SCANCODE_ESCAPE))
            {
                quit();
            }
        }
    }

    return 0;
}

void Example::onSetupUi(const GameTimer& timer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(125 * 2.0f, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Metal Example", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(m_window.get()), timer.framesPerSecond());
    ImGui::Text("Press Esc to quit");
    ImGui::End();
    ImGui::PopStyleVar();
}

void Example::quit()
{
    m_running = false;
}

void Example::metalDisplayLinkNeedsUpdate(
    [[maybe_unused]] CA::MetalDisplayLink* displayLink, CA::MetalDisplayLinkUpdate* update)
{
    m_timer.tick([this] { onUpdate(m_timer); });

    m_keyboard->update();
    m_mouse->update();

    CA::MetalDrawable* drawable = update->drawable();
    if (!drawable)
        return;

    if (m_frameNumber >= s_bufferCount)
    {
        // Wait for the GPU to finish rendering the frame that's
        // `kMaxFramesInFlight` before this one, and then proceed to the next step.
        uint64_t previousValueToWaitFor = m_frameNumber - s_bufferCount;
        m_sharedEvent->waitUntilSignaledValue(previousValueToWaitFor, 10);
    }

    // Get the next allocator in the rotation.
    m_currentFrameIndex = m_frameNumber % s_bufferCount;
    MTL4::CommandAllocator* frameAllocator = m_commandAllocator[m_currentFrameIndex].get();

    // Prepare to use or reuse the allocator by resetting it.
    frameAllocator->reset();

    m_commandBuffer->beginCommandBuffer(frameAllocator);

    // TODO: This needs to be re-done with improved synchonization
    // Updating the frame buffer resources in the middle of rendering is not
    // good and should instead by done after all work as completed and before
    // any new work is scheduled.
    // Update depth stencil texture if necessary
    if (drawable->texture()->width() != m_depthStencilTexture->width()
        || drawable->texture()->height() != m_depthStencilTexture->height())
    {
        const auto width = windowWidth();
        const auto height = windowHeight();
        createFrameResources(width, height);
    }

    onRender(drawable, m_commandBuffer.get(), m_timer);

    m_commandBuffer->endCommandBuffer();

    m_commandQueue->wait(drawable);
    MTL4::CommandBuffer* buffers[] = { m_commandBuffer.get() };

    m_commandQueue->commit(buffers, 1);
    m_commandQueue->signalDrawable(drawable);
    static_cast<MTL::Drawable*>(drawable)->present();

    // Signal when the GPU finishes rendering this frame with a shared event.
    uint64_t futureValueToWaitFor = m_frameNumber;
    m_commandQueue->signalEvent(m_sharedEvent.get(), futureValueToWaitFor);
}
