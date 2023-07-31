
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <SDL2/SDL.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

class Example {
public:
    Example(const char *title, uint32_t width, uint32_t height);

    virtual ~Example();

    int Run([[maybe_unused]] int argc, [[maybe_unused]] char **argv);

    [[nodiscard]] uint32_t GetFrameWidth() const;

    [[nodiscard]] uint32_t GetFrameHeight() const;

    virtual bool Load() = 0;

    virtual void Update(float elapsed) = 0;

    virtual void Render(CA::MetalDrawable *drawable, float elapsed) = 0;

protected:
    static constexpr int BUFFER_COUNT = 3;

    SDL_MetalView View;

    // Metal
    NS::SharedPtr<MTL::Device> Device;
    NS::SharedPtr<MTL::CommandQueue> CommandQueue;
    NS::SharedPtr<MTL::Texture> DepthStencilTexture;
    NS::SharedPtr<MTL::DepthStencilState> DepthStencilState;
    NS::SharedPtr<MTL::Library> PipelineLibrary;

    // Sync primitives
    uint32_t FrameIndex = 0;
    dispatch_semaphore_t FrameSemaphore;

private:
    SDL_Window *Window;
    uint32_t Width;
    uint32_t Height;
    uint64_t LastClockTimestamp;
    uint64_t CurrentClockTimestamp;
    bool Running;


    void CreateDepthStencil();;


};
