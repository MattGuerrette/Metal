/*
 *    Metal Examples
 *    Copyright(c) 2019 Matt Guerrette
 *
 *    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "platform.h"

#include <chrono>
#include <string>

#include <SDL.h>
#include <QuartzCore/QuartzCore.h>

class Example
{
 public:
    Example(const char* title, uint32_t width, uint32_t height);

    virtual ~Example();

    virtual void Init() = 0;

    virtual void Update() = 0;

    virtual void Render() = 0;

    // Run loop
    int Run(int argc, char* argv[]);

    CAMetalLayer* GetMetalLayer() const;

 private:
    static int EventFilter(void* self, SDL_Event* e);

    static void FrameTick(void* self);

    std::string   Title;
    bool          Running;
    uint32_t      Width;
    uint32_t      Height;
    SDL_MetalView MetalView;
    SDL_Window* Window;
};
