/*
 *    Metal Examples
 *    Copyright(c) 2019 Matt Guerrette
 *
 *    This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */


#include "example.h"


#include <atomic>
#include <cassert>
#include <chrono>
#include <string>

using namespace std::chrono_literals;

constexpr std::chrono::nanoseconds timestep(16ms);

Example::Example(std::string title, uint32_t width, uint32_t height)
		: Title(title), Width(width), Height(height)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		NSLog(@"Failed to initialize SDL.");
	}

	Window =
			SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_METAL);
	MetalView = SDL_Metal_CreateView(Window);
	Running   = true;
}

Example::~Example()
{
}

CAMetalLayer* Example::GetMetalLayer() const
{
	return (__bridge CAMetalLayer*)SDL_Metal_GetLayer(MetalView);
}

int Example::EventFilter(void* self, SDL_Event* e)
{
	Example* example = static_cast<Example*>(self);

	switch (e->type)
	{
	case SDL_APP_WILLENTERBACKGROUND:
	{
		break;
	}


	case SDL_APP_WILLENTERFOREGROUND:
	{
		break;
	}


	default:
		break;
	}

	return 1;
}

void Example::FrameTick(void* self)
{
	Example* example = static_cast<Example*>(self);

	// Update
	example->Update();

	// Render
	example->Render();
}

int Example::Run(int argc, char* argv[])
{
	Init();
#if defined(__IPHONEOS__) || defined(__TVOS__)
	SDL_SetEventFilter(EventFilter, this);

	// Use application callback for iOS/iPadOS
	SDL_iPhoneSetAnimationCallback(Window, 1, FrameTick, this);
#else
	// Run normal application loop
	while (Running) {
	  SDL_Event e;
	  SDL_PollEvent(&e);
	  if (e.type == SDL_QUIT) {
		Running = false;
		break;
	  }

	  Update();

	  Render();
	}
#endif
	return EXIT_SUCCESS;
}
