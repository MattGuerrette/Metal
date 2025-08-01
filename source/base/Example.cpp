
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

    m_commandQueue = NS::TransferPtr(m_device->newCommandQueue());

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

    m_frameSemaphore = dispatch_semaphore_create(s_bufferCount);

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

MTL::CommandQueue* Example::commandQueue() const
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
    return m_frameIndex;
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
    CA::MetalDisplayLink* /*displayLink*/, CA::MetalDisplayLinkUpdate* update)
{
    m_timer.tick([this] { onUpdate(m_timer); });

    m_keyboard->update();
    m_mouse->update();

    m_frameIndex = (m_frameIndex + 1) % s_bufferCount;

    MTL::CommandBuffer* commandBuffer = m_commandQueue->commandBuffer();

    dispatch_semaphore_wait(m_frameSemaphore, DISPATCH_TIME_FOREVER);
    commandBuffer->addCompletedHandler(
        [this](MTL::CommandBuffer* /*buffer*/) { dispatch_semaphore_signal(m_frameSemaphore); });

    if (const CA::MetalDrawable* drawable = update->drawable(); drawable != nullptr)
    {
        // Update depth stencil texture if necessary¬
        if (drawable->texture()->width() != m_depthStencilTexture->width()
            || drawable->texture()->height() != m_depthStencilTexture->height())
        {

            const auto width = windowWidth();
            const auto height = windowHeight();
            createFrameResources(width, height);

            // m_msaaTexture.reset();
            //
            // // Create a multisample texture
            // MTL::TextureDescriptor* msaaTextureDescriptor
            //     = MTL::TextureDescriptor::texture2DDescriptor(
            //         MTL::PixelFormatBGRA8Unorm_sRGB, width, height, false);
            // msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
            // msaaTextureDescriptor->setSampleCount(s_multisampleCount); // Set sample count for
            // MSAA msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
            // msaaTextureDescriptor->setStorageMode(MTL::StorageModePrivate);
            //
            // m_msaaTexture = NS::TransferPtr(m_device->newTexture(msaaTextureDescriptor));
            //
            // m_depthStencilTexture.reset();
            //
            // MTL::TextureDescriptor* textureDescriptor =
            // MTL::TextureDescriptor::texture2DDescriptor(
            //
            //     MTL::PixelFormatDepth32Float_Stencil8, width, height, false);
            // textureDescriptor->setSampleCount(s_multisampleCount);
            // textureDescriptor->setTextureType(MTL::TextureType2DMultisample);
            // textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
            // textureDescriptor->setResourceOptions(
            //     MTL::ResourceOptionCPUCacheModeDefault | MTL::ResourceStorageModePrivate);
            // textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);
            //
            // m_depthStencilTexture = NS::TransferPtr(m_device->newTexture(textureDescriptor));
        }

        MTL::RenderPassDescriptor* passDescriptor
            = MTL::RenderPassDescriptor::renderPassDescriptor();
        passDescriptor->colorAttachments()->object(0)->setResolveTexture(drawable->texture());
        passDescriptor->colorAttachments()->object(0)->setTexture(m_msaaTexture.get());
        passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        passDescriptor->colorAttachments()->object(0)->setStoreAction(
            MTL::StoreActionMultisampleResolve);
        passDescriptor->colorAttachments()->object(0)->setClearColor(
            MTL::ClearColor(.39, .58, .92, 1.0));
        passDescriptor->depthAttachment()->setTexture(m_depthStencilTexture.get());
        passDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
        passDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
        passDescriptor->depthAttachment()->setClearDepth(1.0);
        passDescriptor->stencilAttachment()->setTexture(m_depthStencilTexture.get());
        passDescriptor->stencilAttachment()->setLoadAction(MTL::LoadActionClear);
        passDescriptor->stencilAttachment()->setStoreAction(MTL::StoreActionDontCare);
        passDescriptor->stencilAttachment()->setClearStencil(0);

        MTL::RenderCommandEncoder* commandEncoder
            = commandBuffer->renderCommandEncoder(passDescriptor);

        commandEncoder->pushDebugGroup(MTLSTR("SAMPLE RENDERING"));

        onRender(commandEncoder, m_timer);

        commandEncoder->popDebugGroup();

        // ImGui rendering
        ImGui_ImplMetal_NewFrame(passDescriptor);
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        onSetupUi(m_timer);

        commandEncoder->pushDebugGroup(MTLSTR("IMGUI RENDERING"));

        // Rendering
        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, commandEncoder);

        commandEncoder->popDebugGroup();

        commandEncoder->endEncoding();

        commandBuffer->presentDrawable(drawable);
        commandBuffer->commit();
    }
}
