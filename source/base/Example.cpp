
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Example.hpp"

Example::Example(const char *title, uint32_t width, uint32_t height) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Failed to initialize SDL.\n");
        abort();
    }

    int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_METAL;
#if defined(__IPHONEOS__) || defined(__TVOS__)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif
    Window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int) width, (int) height, flags);
    if (!Window) {
        fprintf(stderr, "Failed to create SDL window.\n");
        abort();
    }
    View = SDL_Metal_CreateView(Window);
    Running = true;

    LastClockTimestamp = 0;
    CurrentClockTimestamp = SDL_GetPerformanceCounter();
    Width = width;
    Height = height;


    Device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

    auto *layer = static_cast<CA::MetalLayer *>((SDL_Metal_GetLayer(View)));
    layer->setDevice(Device.get());

    CommandQueue = NS::TransferPtr(Device->newCommandQueue());

    CreateDepthStencil();

    // Load Pipeline Library
    CFBundleRef main_bundle = CFBundleGetMainBundle();
    CFURLRef url = CFBundleCopyResourcesDirectoryURL(main_bundle);
    char cwd[PATH_MAX];
    CFURLGetFileSystemRepresentation(url, TRUE, (UInt8 *) cwd, PATH_MAX);
    CFRelease(url);
    auto path = NS::Bundle::mainBundle()->resourcePath();
    NS::String *library = path->stringByAppendingString(
            NS::String::string("/default.metallib", NS::ASCIIStringEncoding));

    NS::Error *error = nullptr;
    PipelineLibrary = NS::TransferPtr(Device->newLibrary(library, &error));
    if (error != nullptr) {
        fprintf(stderr, "Failed to create pipeline library: %s\n",
                error->localizedFailureReason()->utf8String());
        abort();
    }


    FrameSemaphore = dispatch_semaphore_create(BUFFER_COUNT);
}

Example::~Example() {
    if (Window != nullptr) {
        SDL_DestroyWindow(Window);
    }
    SDL_Quit();
}

void Example::CreateDepthStencil() {
    MTL::DepthStencilDescriptor *depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);

    DepthStencilState = NS::TransferPtr(Device->newDepthStencilState(depthStencilDescriptor));

    depthStencilDescriptor->release();


    MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatDepth32Float_Stencil8, GetFrameWidth(), GetFrameHeight(), false);
    textureDescriptor->setSampleCount(1);
    textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    textureDescriptor->setResourceOptions(MTL::ResourceOptionCPUCacheModeDefault | MTL::ResourceStorageModePrivate);
    textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);

    DepthStencilTexture = NS::TransferPtr(Device->newTexture(textureDescriptor));

    textureDescriptor->release();
}

uint32_t Example::GetFrameWidth() const {
    int32_t w;
    SDL_Metal_GetDrawableSize(Window, &w, nullptr);
    return w;
}

uint32_t Example::GetFrameHeight() const {
    int32_t h;
    SDL_Metal_GetDrawableSize(Window, nullptr, &h);

    return h;
}

int Example::Run([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    if (!Load()) {
        return EXIT_FAILURE;
    }

    while (Running) {
        NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();

        SDL_Event e;
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            Running = false;
            break;
        }
        if (e.type == SDL_WINDOWEVENT) {
            switch (e.window.event) {
                case SDL_WINDOWEVENT_SHOWN: {
                    break;
                }
            }
        }

        LastClockTimestamp = CurrentClockTimestamp;
        CurrentClockTimestamp = SDL_GetPerformanceCounter();

        float elapsed = static_cast<float>((CurrentClockTimestamp - LastClockTimestamp))
                        / static_cast<float>(SDL_GetPerformanceFrequency());

        Update(elapsed);

        FrameIndex = (FrameIndex + 1) % BUFFER_COUNT;

        MTL::CommandBuffer *commandBuffer = CommandQueue->commandBuffer();

        dispatch_semaphore_wait(FrameSemaphore, DISPATCH_TIME_FOREVER);
        commandBuffer->addCompletedHandler([this](MTL::CommandBuffer *buffer) {
            dispatch_semaphore_signal(FrameSemaphore);
        });

        auto *layer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(View));
        CA::MetalDrawable *drawable = layer->nextDrawable();
        if (drawable) {
            Render(drawable, elapsed);
        }


        pool->release();
    }

    return EXIT_SUCCESS;
}

