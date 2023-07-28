
#include "Example.hpp"

Example::Example(const char* title, uint32_t width, uint32_t height)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "Failed to initialize SDL.\n");
        abort();
    }
    
    int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_METAL;
#if defined(__IPHONEOS__) || defined(__TVOS__)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif
    Window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)width, (int)height, flags);
    if (!Window)
    {
        fprintf(stderr, "Failed to create SDL window.\n");
        abort();
    }
    View = SDL_Metal_CreateView(Window);
    Running = true;
    
    LastClockTimestamp = 0;
    CurrentClockTimestamp = SDL_GetPerformanceCounter();
    Width = width;
    Height = height;
}

Example::~Example()
{
    if (Window != nullptr)
    {
        SDL_DestroyWindow(Window);
    }
    SDL_Quit();
}

uint32_t Example::GetFrameWidth() const
{
    int32_t w;
    SDL_Metal_GetDrawableSize(Window, &w, nullptr);
    return w;
}

uint32_t Example::GetFrameHeight() const
{
    int32_t h;
    SDL_Metal_GetDrawableSize(Window, nullptr, &h);
    
    return h;
}

int Example::Run(int argc, char **argv)
{
    if (!Load())
    {
        return EXIT_FAILURE;
    }
    
    while (Running)
    {
        SDL_Event e;
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT)
        {
            Running = false;
            break;
        }
        if(e.type == SDL_WINDOWEVENT)
        {
            switch(e.window.event)
            {
                case SDL_WINDOWEVENT_SHOWN:
                {
                    break;
                }
            }
        }

        LastClockTimestamp = CurrentClockTimestamp;
        CurrentClockTimestamp = SDL_GetPerformanceCounter();

        float elapsed = static_cast<float>((CurrentClockTimestamp - LastClockTimestamp))
                         / static_cast<float>(SDL_GetPerformanceFrequency());

        Update(elapsed);
        
        Render(elapsed);
    }
    
    return EXIT_SUCCESS;
}

