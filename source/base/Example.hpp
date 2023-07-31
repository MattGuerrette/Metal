
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <SDL2/SDL.h>

class Example {
public:
    Example(const char *title, uint32_t width, uint32_t height);

    virtual ~Example();

    int Run([[maybe_unused]] int argc, [[maybe_unused]] char **argv);

    [[nodiscard]] uint32_t GetFrameWidth() const;

    [[nodiscard]] uint32_t GetFrameHeight() const;

    virtual bool Load() = 0;

    virtual void Update(float elapsed) = 0;

    virtual void Render(float elapsed) = 0;

protected:
    SDL_MetalView View;
private:
    SDL_Window *Window;
    uint32_t Width;
    uint32_t Height;
    uint64_t LastClockTimestamp;
    uint64_t CurrentClockTimestamp;
    bool Running;
};
