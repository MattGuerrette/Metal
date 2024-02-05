
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <string>

#include <SDL2/SDL.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "GameTimer.hpp"
#include "Keyboard.hpp"
#include "Mouse.hpp"

class Example : public CA::MetalDisplayLinkDelegate
{
public:

	SDL_Window* Window;

	Example(const char* title, uint32_t width, uint32_t height);

	virtual ~Example();

	int Run([[maybe_unused]] int argc, [[maybe_unused]] char** argv);

	void Quit();

	[[nodiscard]] uint32_t GetFrameWidth() const;

	[[nodiscard]] uint32_t GetFrameHeight() const;

	virtual bool Load() = 0;

	virtual void Update(const GameTimer& timer) = 0;

	virtual void SetupUi(const GameTimer& timer);

	virtual void Render(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) = 0;

	static std::string PathForResource(const std::string& resourceName);

	void metalDisplayLinkNeedsUpdate(CA::MetalDisplayLink* displayLink, CA::MetalDisplayLinkUpdate* update) override;

protected:
	static constexpr int BufferCount = 3;

	SDL_MetalView View;

	// Keyboard and Mouse
	std::unique_ptr<class Keyboard> Keyboard;
	std::unique_ptr<class Mouse> Mouse;

	// Metal
	NS::SharedPtr<CA::MetalDisplayLink> DisplayLink_;
	NS::SharedPtr<MTL::Device> Device;
	NS::SharedPtr<MTL::CommandQueue> CommandQueue;
	NS::SharedPtr<MTL::Texture> DepthStencilTexture;
	NS::SharedPtr<MTL::DepthStencilState> DepthStencilState;
	NS::SharedPtr<MTL::Library> PipelineLibrary;
	MTL::PixelFormat FrameBufferPixelFormat;

	// Sync primitives
	uint32_t FrameIndex = 0;
	dispatch_semaphore_t FrameSemaphore;

private:

	uint32_t Width;
	uint32_t Height;
	GameTimer Timer;
	bool Running;

	void CreateDepthStencil();;

};
