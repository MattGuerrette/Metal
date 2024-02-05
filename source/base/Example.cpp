
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Example.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_metal.h"

Example::Example(const char* title, uint32_t width, uint32_t height)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.IniSavingRate = 0.0f;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
	ImGui::StyleColorsDark();


	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "Failed to initialize SDL.\n");
		abort();
	}


	int flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_METAL;
#if defined(__IPHONEOS__) || defined(__TVOS__)
	flags |= SDL_WINDOW_FULLSCREEN;
#else
	flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(0, &mode);
	width = mode.w;
	height = mode.h;
#endif
	Window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)width, (int)height, flags);
	if (!Window)
	{
		fprintf(stderr, "Failed to create SDL window.\n");
		abort();
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
	ImGui_ImplSDL2_InitForMetal(Window);

	CommandQueue = NS::TransferPtr(Device->newCommandQueue());

	CreateDepthStencil();

	// Load Pipeline Library
	// TODO: Showcase how to use Metal archives to erase compilation
	auto path = NS::Bundle::mainBundle()->resourcePath();
	NS::String* library = path->stringByAppendingString(
			NS::String::string("/default.metallib", NS::ASCIIStringEncoding));

	NS::Error* error = nullptr;
	PipelineLibrary = NS::TransferPtr(Device->newLibrary(library, &error));
	if (error != nullptr)
	{
		fprintf(stderr, "Failed to create pipeline library: %s\n",
				error->localizedFailureReason()->utf8String());
		abort();
	}

	FrameSemaphore = dispatch_semaphore_create(BufferCount);

	Keyboard = std::make_unique<class Keyboard>();
	Mouse = std::make_unique<class Mouse>(Window);


	Timer.SetTargetElapsedSeconds(1.0f / static_cast<float>(mode.refresh_rate));
	Timer.SetFixedTimeStep(true);

	DisplayLink_ = NS::TransferPtr(CA::MetalDisplayLink::alloc()->init(layer));
	DisplayLink_->setDelegate(this);

}

Example::~Example()
{
	// Cleanup
	ImGui_ImplMetal_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if (Window != nullptr)
	{
		SDL_DestroyWindow(Window);
	}
	SDL_Quit();
}

void Example::CreateDepthStencil()
{
	MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
	depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
	depthStencilDescriptor->setDepthWriteEnabled(true);

	DepthStencilState = NS::TransferPtr(Device->newDepthStencilState(depthStencilDescriptor));

	depthStencilDescriptor->release();

	MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(
			MTL::PixelFormatDepth32Float_Stencil8, GetFrameWidth(), GetFrameHeight(), false);
	textureDescriptor->setSampleCount(1);
	textureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
	textureDescriptor->setResourceOptions(MTL::ResourceOptionCPUCacheModeDefault | MTL::ResourceStorageModePrivate);
	textureDescriptor->setStorageMode(MTL::StorageModeMemoryless);

	DepthStencilTexture = NS::TransferPtr(Device->newTexture(textureDescriptor));

	textureDescriptor->release();

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

std::string Example::PathForResource(const std::string& resourceName)
{
	auto path = NS::Bundle::mainBundle()->resourcePath();
	NS::String* resourcePath = path->stringByAppendingString(
			NS::String::string((std::string("/") + resourceName).c_str(), NS::ASCIIStringEncoding));

	std::string ret = resourcePath->utf8String();
	resourcePath->release();
	return ret;
}

int Example::Run([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
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
			ImGui_ImplSDL2_ProcessEvent(&e);

			if (e.type == SDL_QUIT)
			{
				Running = false;
				break;
			}
			if (e.type == SDL_WINDOWEVENT)
			{
				switch (e.window.event)
				{
				case SDL_WINDOWEVENT_SHOWN:
				{
					break;
				}
				}
			}

			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
			{
				Keyboard->RegisterKeyEvent(&e.key);
			}
			if (e.type == SDL_MOUSEBUTTONUP)
			{
				Mouse->RegisterMouseButton(&e.button);
			}
			if (e.type == SDL_MOUSEBUTTONDOWN)
			{
				Mouse->RegisterMouseButton(&e.button);
			}
			if (e.type == SDL_MOUSEMOTION)
			{
				Mouse->RegisterMouseMotion(&e.motion);
			}
			if (e.type == SDL_MOUSEWHEEL)
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
	ImGui::SetNextWindowSize(ImVec2(125 * ImGui::GetIO().DisplayFramebufferScale.x, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("Metal Example",
			nullptr,
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

void Example::metalDisplayLinkNeedsUpdate(CA::MetalDisplayLink* displayLink, CA::MetalDisplayLinkUpdate* update)
{
	Timer.Tick([this]()
	{
		Update(Timer);
	});

	Keyboard->Update();
	Mouse->Update();

	FrameIndex = (FrameIndex + 1) % BufferCount;

	MTL::CommandBuffer* commandBuffer = CommandQueue->commandBuffer();

	dispatch_semaphore_wait(FrameSemaphore, DISPATCH_TIME_FOREVER);
	commandBuffer->addCompletedHandler([this](MTL::CommandBuffer* buffer)
	{
		dispatch_semaphore_signal(FrameSemaphore);
	});

	CA::MetalDrawable* drawable = update->drawable();
	if (drawable)
	{
		MTL::RenderPassDescriptor* passDescriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
		passDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
		passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
		passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
		passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(.39, .58, .92, 1.0));
		passDescriptor->depthAttachment()->setTexture(DepthStencilTexture.get());
		passDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionClear);
		passDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
		passDescriptor->depthAttachment()->setClearDepth(1.0);
		passDescriptor->stencilAttachment()->setTexture(DepthStencilTexture.get());
		passDescriptor->stencilAttachment()->setLoadAction(MTL::LoadActionClear);
		passDescriptor->stencilAttachment()->setStoreAction(MTL::StoreActionDontCare);
		passDescriptor->stencilAttachment()->setClearStencil(0);
		MTL::RenderCommandEncoder* commandEncoder = commandBuffer->renderCommandEncoder(passDescriptor);

		Render(commandEncoder, Timer);

		// IMGUI rendering
		ImGui_ImplMetal_NewFrame(passDescriptor);
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		SetupUi(Timer);

		// Rendering
		ImGui::Render();
		ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, commandEncoder);

		commandEncoder->endEncoding();

		commandBuffer->presentDrawable(drawable);
		commandBuffer->commit();
	}
}
