
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

Example::Example(const char* title, uint32_t width, uint32_t height)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.IniSavingRate = 0.0f;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    ImGui::StyleColorsDark();

    // Initialize SDL
    if (const int result =
            SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_TIMER);
        result < 0)
    {
        fmt::println(fmt::format("Failed to initialize SDL: {}", SDL_GetError()));
        abort();
    }

    int  numDisplays = 0;
    auto displays = SDL_GetDisplays(&numDisplays);
    assert(numDisplays != 0);

    const auto display = displays[0];
    const auto mode = SDL_GetDesktopDisplayMode(display);
    int32_t    screenWidth = mode->w;
    int32_t    screenHeight = mode->h;
    SDL_free(displays);

    int flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_METAL;
#ifdef SDL_PLATFORM_MACOS
    flags ^= SDL_WINDOW_FULLSCREEN;
    flags |= SDL_WINDOW_RESIZABLE;
    screenWidth = width;
    screenHeight = height;
#endif

    Window = SDL_CreateWindow(title, screenWidth, screenHeight, flags);
    if (!Window)
    {
        throw std::runtime_error(fmt::format("Failed to create SDL window: {}", SDL_GetError()));
    }
    View = SDL_Metal_CreateView(Window);
    Running = true;
    SDL_ShowWindow(Window);

    Width = width;
    Height = height;

    Device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

    auto* layer = static_cast<CA::MetalLayer*>((SDL_Metal_GetLayer(View)));
    FrameBufferPixelFormat = MTL::PixelFormatBGRA8Unorm_sRGB;
    layer->setPixelFormat(FrameBufferPixelFormat);
    layer->setDevice(Device.get());

    ImGui_ImplMetal_Init(Device.get());
    ImGui_ImplSDL3_InitForMetal(Window);

    CommandQueue = NS::TransferPtr(Device->newCommandQueue());

    CreateDepthStencil();

    // Load Pipeline Library
    // TODO: Showcase how to use Metal archives to erase compilation
    PipelineLibrary = NS::TransferPtr(Device->newDefaultLibrary());

    FrameSemaphore = dispatch_semaphore_create(BufferCount);

    Keyboard = std::make_unique<class Keyboard>();
    Mouse = std::make_unique<class Mouse>(Window);

    Timer.SetFixedTimeStep(false);
    Timer.ResetElapsedTime();

    DisplayLink_ = NS::TransferPtr(CA::MetalDisplayLink::alloc()->init(layer));
    // Enable 120HZ refresh for devices that support Pro Motion
    DisplayLink_->setPreferredFrameRateRange({60, 120, 120});
    DisplayLink_->setDelegate(this);
}

Example::~Example()
{
    // Cleanup
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (Window != nullptr)
    {
        SDL_DestroyWindow(Window);
    }
    SDL_Quit();
}

void Example::CreateDepthStencil()
{
    DepthStencilState.reset();

    MTL::DepthStencilDescriptor* depthStencilDescriptor =
        MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);

    DepthStencilState = NS::TransferPtr(Device->newDepthStencilState(depthStencilDescriptor));

    depthStencilDescriptor->release();

    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(

        MTL::PixelFormatDepth32Float_Stencil8, GetFrameWidth(), GetFrameHeight(), false);
    textureDescriptor->setSampleCount(1);
    textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    textureDescriptor->setResourceOptions(MTL::ResourceOptionCPUCacheModeDefault |
                                          MTL::ResourceStorageModePrivate);
    textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);

    DepthStencilTexture = NS::TransferPtr(Device->newTexture(textureDescriptor));

    textureDescriptor->release();
}

uint32_t Example::GetFrameWidth() const
{
    int32_t w;
    SDL_GetWindowSizeInPixels(Window, &w, nullptr);
    return w;
}

uint32_t Example::GetFrameHeight() const
{
    int32_t h;
    SDL_GetWindowSizeInPixels(Window, nullptr, &h);
    return h;
}

NS::Menu* Example::createMenuBar()
{
    return nullptr;
}

int Example::Run([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
#ifdef SDL_PLATFORM_MACOS
    NS::Menu* menu = createMenuBar();
    if (menu)
    {
        NS::Application::sharedApplication()->setMainMenu(menu);
    }
#endif
    if (!Load())
    {
        return EXIT_FAILURE;
    }

    DisplayLink_->addToRunLoop(NS::RunLoop::mainRunLoop(), NS::DefaultRunLoopMode);

    while (Running)
    {
        SDL_Event e;
        if (SDL_WaitEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT)
            {
                Running = false;
                break;
            }

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                const float     density = SDL_GetWindowPixelDensity(Window);
                CA::MetalLayer* layer = (CA::MetalLayer*)SDL_Metal_GetLayer(View);
                layer->setDrawableSize(
                    CGSize{(float)e.window.data1 * density, (float)e.window.data2 * density});

                ImGuiIO& io = ImGui::GetIO();
                io.DisplaySize =
                    ImVec2((float)e.window.data1 * density, (float)e.window.data2 * density);
            }

            if (e.type == SDL_EVENT_JOYSTICK_ADDED)
            {
                if (SDL_IsGamepad(e.jdevice.which))
                {
                    Gamepad_ = std::make_unique<Gamepad>(e.jdevice.which);
                }
            }

            if (e.type == SDL_EVENT_JOYSTICK_REMOVED)
            {
                if (SDL_IsGamepad(e.jdevice.which))
                {
                    Gamepad_.reset();
                }
            }

            if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP)
            {
                Keyboard->RegisterKeyEvent(&e.key);
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                Mouse->RegisterMouseButton(&e.button);
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                Mouse->RegisterMouseMotion(&e.motion);
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL)
            {
                Mouse->RegisterMouseWheel(&e.wheel);
            }

            if (Keyboard->IsKeyClicked(SDL_SCANCODE_ESCAPE))
            {
                Quit();
            }
        }
    }

    return 0;
}

void Example::SetupUi(const GameTimer& timer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(125 * ImGui::GetIO().DisplayFramebufferScale.x, 0),
                             ImGuiCond_FirstUseEver);
    ImGui::Begin("Metal Example", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(Window), timer.GetFramesPerSecond());
    ImGui::Text("Press Esc to Quit");
    ImGui::End();
    ImGui::PopStyleVar();
}

void Example::Quit()
{
    Running = false;
}

void Example::metalDisplayLinkNeedsUpdate(CA::MetalDisplayLink*       displayLink,
                                          CA::MetalDisplayLinkUpdate* update)
{
    Timer.Tick([this]() { Update(Timer); });

    Keyboard->Update();
    Mouse->Update();

    FrameIndex = (FrameIndex + 1) % BufferCount;

    MTL::CommandBuffer* commandBuffer = CommandQueue->commandBuffer();

    dispatch_semaphore_wait(FrameSemaphore, DISPATCH_TIME_FOREVER);
    commandBuffer->addCompletedHandler(
        [this](MTL::CommandBuffer* buffer) { dispatch_semaphore_signal(FrameSemaphore); });

    CA::MetalDrawable* drawable = update->drawable();
    if (drawable)
    {
        // Update depth stencil texture if necessary¬
        if (drawable->texture()->width() != DepthStencilTexture->width() ||
            drawable->texture()->height() != DepthStencilTexture->height())
        {
            DepthStencilTexture.reset();

            MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(

                MTL::PixelFormatDepth32Float_Stencil8, GetFrameWidth(), GetFrameHeight(), false);
            textureDescriptor->setSampleCount(1);
            textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
            textureDescriptor->setResourceOptions(MTL::ResourceOptionCPUCacheModeDefault |
                                                  MTL::ResourceStorageModePrivate);
            textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);

            DepthStencilTexture = NS::TransferPtr(Device->newTexture(textureDescriptor));
        }

        MTL::RenderPassDescriptor* passDescriptor =
            MTL::RenderPassDescriptor::renderPassDescriptor();
        passDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
        passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
        passDescriptor->colorAttachments()->object(0)->setClearColor(
            MTL::ClearColor(.39, .58, .92, 1.0));
        passDescriptor->depthAttachment()->setTexture(DepthStencilTexture.get());
        passDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
        passDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
        passDescriptor->depthAttachment()->setClearDepth(1.0);
        passDescriptor->stencilAttachment()->setTexture(DepthStencilTexture.get());
        passDescriptor->stencilAttachment()->setLoadAction(MTL::LoadActionClear);
        passDescriptor->stencilAttachment()->setStoreAction(MTL::StoreActionDontCare);
        passDescriptor->stencilAttachment()->setClearStencil(0);
        MTL::RenderCommandEncoder* commandEncoder =
            commandBuffer->renderCommandEncoder(passDescriptor);

        commandEncoder->pushDebugGroup(MTLSTR("SAMPLE RENDERING"));

        Render(commandEncoder, Timer);

        commandEncoder->popDebugGroup();

        // IMGUI rendering
        ImGui_ImplMetal_NewFrame(passDescriptor);
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        SetupUi(Timer);

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
