
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
#include <ktx.h>

#include <memory>
#include <fmt/core.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <imgui.h>

#include "Example.hpp"
#include "Camera.hpp"


using namespace DirectX;

XM_ALIGNED_STRUCT(16) Vertex
{
	Vector4 Position;
	Vector2 TexCoord;
};

XM_ALIGNED_STRUCT(16) Uniforms
{
	[[maybe_unused]] Matrix ModelViewProjection;
};

static constexpr size_t TextureCount = 5;

XM_ALIGNED_STRUCT(16) FragmentArgumentBuffer
{
	MTL::ResourceID textures[TextureCount];
	uint32_t textureIndex;
	Matrix* transforms;
};

static const std::array<const char*, 5> ComboItems = { "Texture 0", "Texture 1", "Texture 2",
													   "Texture 3", "Texture 4" };

class Textures : public Example
{
	static constexpr int InstanceCount = 3;
public:
	Textures();

	~Textures() override;

	bool Load() override;

	void Update(const GameTimer& timer) override;

	void SetupUi(const GameTimer& timer) override;

	void Render(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) override;

private:
	void CreateBuffers();

	void CreatePipelineState();

	void CreateTextureHeap();

	void CreateArgumentBuffers();

	void UpdateUniforms();

	MTL::Texture* LoadTextureFromFile(const std::string& fileName);

	NS::SharedPtr<MTL::RenderPipelineState> PipelineState;
	NS::SharedPtr<MTL::Buffer> VertexBuffer;
	NS::SharedPtr<MTL::Buffer> IndexBuffer;
	NS::SharedPtr<MTL::Buffer> InstanceBuffer[BufferCount];
	std::unique_ptr<Camera> MainCamera;
	NS::SharedPtr<MTL::Heap> TextureHeap;
	NS::SharedPtr<MTL::Texture> Texture;
	NS::SharedPtr<MTL::Buffer> ArgumentBuffer[BufferCount];
	std::vector<NS::SharedPtr<MTL::Texture>> HeapTextures;

	float RotationX = 0.0f;
	float RotationY = 0.0f;

	int SelectedTexture = 0;
};

Textures::Textures()
	: Example("Textures", 800, 600)
{

}

Textures::~Textures() = default;

bool Textures::Load()
{
	const auto width = GetFrameWidth();
	const auto height = GetFrameHeight();
	const float aspect = (float)width / (float)height;
	const float fov = XMConvertToRadians(75.0f);
	const float near = 0.01f;
	const float far = 1000.0f;


	MainCamera = std::make_unique<Camera>(Vector3::Zero,
		Vector3::Forward,
		Vector3::Up,
		fov, aspect, near, far);

	CreateBuffers();

	CreatePipelineState();

	CreateTextureHeap();

	CreateArgumentBuffers();

	Texture = NS::TransferPtr(LoadTextureFromFile("001_basecolor.ktx"));

	return true;
}

void Textures::SetupUi(const GameTimer& timer)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(125 * ImGui::GetIO().DisplayFramebufferScale.x, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("Metal Example",
		nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
	ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(Window), timer.GetFramesPerSecond());
	if (ImGui::Combo(" ", &SelectedTexture, ComboItems.data(), ComboItems.size()))
	{
		/// Update argument buffer index
		for (const auto& buffer: ArgumentBuffer)
		{
			auto* contents = reinterpret_cast<FragmentArgumentBuffer*>(buffer->contents());
			contents->textureIndex = SelectedTexture;
		}
		printf("Selected item: %d\n", SelectedTexture);
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void Textures::Update(const GameTimer& timer)
{
	//RotationX += elapsed;
	if (Mouse->LeftPressed())
	{
		//printf("Pressed\n");
		RotationY += (1.0f * Mouse->RelativeX()) * timer.GetElapsedSeconds();
	}
}

MTL::Texture* Textures::LoadTextureFromFile(const std::string& fileName)
{
	auto filePath = PathForResource(fileName);
	NS::String* nsFilePath = NS::String::string(filePath.c_str(), NS::UTF8StringEncoding);
	NS::Data* data = NS::Data::data(nsFilePath);
	assert(data != nullptr);
	nsFilePath->release();

	KTX_error_code result;
	uint32_t flags =
		KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT |
			KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT;
	ktxTexture2* ktx2Texture = nullptr;
	result = ktxTexture2_CreateFromMemory((ktx_uint8_t*)data->bytes(), data->length(), flags, &ktx2Texture);
	if (result != KTX_SUCCESS)
	{
		return nullptr;
	}

	MTL::TextureType type = MTL::TextureType2D;
	MTL::PixelFormat pixelFormat = MTL::PixelFormatASTC_8x8_sRGB;

	BOOL genMipmaps = ktx2Texture->generateMipmaps;
	NS::UInteger levelCount = ktx2Texture->numLevels;
	NS::UInteger baseWidth = ktx2Texture->baseWidth;
	NS::UInteger baseHeight = ktx2Texture->baseHeight;
	NS::UInteger baseDepth = ktx2Texture->baseDepth;
	NS::UInteger maxMipLevelCount = floor(log2(fmax(baseWidth, baseHeight))) + 1;
	NS::UInteger storedMipLevelCount = genMipmaps ? maxMipLevelCount : levelCount;

	MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
	textureDescriptor->setTextureType(type);
	textureDescriptor->setPixelFormat(pixelFormat);
	textureDescriptor->setWidth(baseWidth);
	textureDescriptor->setHeight((ktx2Texture->numDimensions > 1) ? baseHeight : 1);
	textureDescriptor->setDepth((ktx2Texture->numDimensions > 2) ? baseDepth : 1);
	textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
	textureDescriptor->setStorageMode(MTL::StorageModeShared);
	textureDescriptor->setArrayLength(1);
	textureDescriptor->setMipmapLevelCount(storedMipLevelCount);

	MTL::Texture* texture = Device->newTexture(textureDescriptor);

	auto* ktx1Texture = reinterpret_cast<ktxTexture*>(ktx2Texture);
	ktx_uint32_t layer = 0, faceSlice = 0;

	for (ktx_uint32_t level = 0; level < ktx2Texture->numLevels; ++level)
	{
		ktx_size_t offset = 0;
		result = ktxTexture_GetImageOffset(ktx1Texture, level, layer, faceSlice, &offset);
		ktx_uint8_t* imageBytes = ktxTexture_GetData(ktx1Texture) + offset;
		auto elementSize = ktxTexture_GetElementSize(ktx1Texture);
		ktx_uint32_t bytesPerRow = ktxTexture_GetRowPitch(ktx1Texture, level);
		ktx_size_t bytesPerImage = ktxTexture_GetImageSize(ktx1Texture, level);
		size_t levelWidth = fmax(1, (baseWidth >> level));
		size_t levelHeight = fmax(1, (baseHeight >> level));


		texture->replaceRegion(MTL::Region(0, 0, levelWidth, levelHeight),
			level, faceSlice, imageBytes, bytesPerRow, bytesPerImage);

	}

	ktxTexture_Destroy((ktxTexture*)ktx1Texture);
	data->release();
	return texture;
}

void Textures::Render(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer)
{

	UpdateUniforms();

	commandEncoder->useHeap(TextureHeap.get());
	commandEncoder->useResource(InstanceBuffer[FrameIndex].get(), MTL::ResourceUsageRead);
	commandEncoder->setRenderPipelineState(PipelineState.get());
	commandEncoder->setDepthStencilState(DepthStencilState.get());
	commandEncoder->setFrontFacingWinding(MTL::WindingClockwise);
	commandEncoder->setCullMode(MTL::CullModeNone);
	commandEncoder->setFragmentBuffer(ArgumentBuffer[FrameIndex].get(), 0, 0);
	commandEncoder->setVertexBuffer(VertexBuffer.get(), 0, 0);
	commandEncoder->setVertexBuffer(ArgumentBuffer[FrameIndex].get(), 0, 1);
	commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
		IndexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16,
		IndexBuffer.get(), 0, InstanceCount);
}

void Textures::CreatePipelineState()
{
	MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

	// Position
	vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
	vertexDescriptor->attributes()->object(0)->setOffset(0);
	vertexDescriptor->attributes()->object(0)->setBufferIndex(0);

	// Color
	vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
	vertexDescriptor->attributes()->object(1)->setOffset(offsetof(Vertex, TexCoord));
	vertexDescriptor->attributes()->object(1)->setBufferIndex(0);

	vertexDescriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
	vertexDescriptor->layouts()->object(0)->setStride(sizeof(Vertex));

	MTL::RenderPipelineDescriptor* pipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
	pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(FrameBufferPixelFormat);
	pipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
	pipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
	pipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
		MTL::BlendFactorOneMinusSourceAlpha);
	pipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
	pipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
	pipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(
		MTL::BlendFactorOneMinusSourceAlpha);
	pipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
	pipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
	pipelineDescriptor->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
	pipelineDescriptor->setVertexFunction(
		PipelineLibrary->newFunction(NS::String::string("texture_vertex", NS::ASCIIStringEncoding)));
	pipelineDescriptor->setFragmentFunction(
		PipelineLibrary->newFunction(NS::String::string("texture_fragment", NS::ASCIIStringEncoding)));
	pipelineDescriptor->setVertexDescriptor(vertexDescriptor);

	NS::Error* error = nullptr;
	PipelineState = NS::TransferPtr(Device->newRenderPipelineState(pipelineDescriptor, &error));
	if (error)
	{
		fprintf(stderr, "Failed to create pipeline state object: %s\n",
			error->description()->cString(NS::ASCIIStringEncoding));
	}

	vertexDescriptor->release();
	pipelineDescriptor->release();
}

void Textures::CreateBuffers()
{
	static const Vertex vertices[] = {
		{ .Position = { -1, -1, 0, 1 }, .TexCoord = { 0, 0 }},
		{ .Position = { -1, 1, 0, 1 }, .TexCoord = { 0, 1 }},
		{ .Position = { 1, -1, 0, 1 }, .TexCoord = { 1, 0 }},
		{ .Position = { 1, 1, 0, 1 }, .TexCoord = { 1, 1 }}};

	static const uint16_t indices[] = { 0, 1, 2, 2, 1, 3 };

	VertexBuffer = NS::TransferPtr(
		Device->newBuffer(vertices, sizeof(vertices), MTL::ResourceCPUCacheModeDefaultCache));
	VertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

	IndexBuffer = NS::TransferPtr(Device->newBuffer(indices, sizeof(indices), MTL::ResourceOptionCPUCacheModeDefault));
	IndexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

	const size_t instanceDataSize = BufferCount * InstanceCount * sizeof(Matrix);
	NS::String* prefix = NS::String::string("Instance Buffer: ", NS::ASCIIStringEncoding);
	for (auto index = 0; index < BufferCount; index++)
	{
		char temp[12];
		snprintf(temp, sizeof(temp), "%d", index);

		InstanceBuffer[index] = NS::TransferPtr(
			Device->newBuffer(instanceDataSize, MTL::ResourceOptionCPUCacheModeDefault));
		InstanceBuffer[index]->setLabel(
			prefix->stringByAppendingString(NS::String::string(temp, NS::ASCIIStringEncoding)));
	}
	prefix->release();
}

void Textures::UpdateUniforms()
{
	MTL::Buffer* instanceBuffer = InstanceBuffer[FrameIndex].get();

	auto* instanceData = static_cast<Matrix*>( instanceBuffer->contents());
	for (auto index = 0; index < InstanceCount; ++index)
	{
		auto position = Vector3(-5.0f + (5.0f * (float)index), 0.0f, -8.0f);
		auto rotationX = RotationX;
		auto rotationY = RotationY;
		auto scaleFactor = 1.0f;

		const Vector3 xAxis = Vector3::Right;
		const Vector3 yAxis = Vector3::Up;

		Matrix xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
		Matrix yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
		Matrix rotation = xRot * yRot;

		Matrix translation = Matrix::CreateTranslation(position);
		Matrix scale = Matrix::CreateScale(scaleFactor);
		Matrix model = scale * rotation * translation;

		CameraUniforms cameraUniforms = MainCamera->GetUniforms();

		instanceData[index] = model * cameraUniforms.ViewProjection;
	}
}

void Textures::CreateTextureHeap()
{
	auto** textures = new MTL::Texture* [TextureCount];
	for (size_t i = 0; i < TextureCount; i++)
	{
		std::string fileName = fmt::format("00{}_basecolor.ktx", i + 1);
		textures[i] = LoadTextureFromFile(fileName);
	}

	MTL::HeapDescriptor* heapDescriptor = MTL::HeapDescriptor::alloc()->init();
	heapDescriptor->setType(MTL::HeapTypeAutomatic);
	heapDescriptor->setStorageMode(MTL::StorageModePrivate);
	heapDescriptor->setSize(0);

	// Allocate space in heap for each texture
	NS::UInteger heapSize = 0;
	for (size_t i = 0; i < TextureCount; i++)
	{
		MTL::Texture* texture = textures[i];
		MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
		textureDescriptor->setTextureType(texture->textureType());
		textureDescriptor->setPixelFormat(texture->pixelFormat());
		textureDescriptor->setWidth(texture->width());
		textureDescriptor->setHeight(texture->height());
		textureDescriptor->setDepth(texture->depth());
		textureDescriptor->setMipmapLevelCount(texture->mipmapLevelCount());
		textureDescriptor->setSampleCount(texture->sampleCount());
		textureDescriptor->setStorageMode(MTL::StorageModePrivate);

		auto sizeAndAlignment = Device->heapTextureSizeAndAlign(textureDescriptor);
		heapSize += sizeAndAlignment.size;

		textureDescriptor->release();
	}
	heapDescriptor->setSize(heapSize);

	TextureHeap = NS::TransferPtr(Device->newHeap(heapDescriptor));
	heapDescriptor->release();

	// Move texture memory into heap
	MTL::CommandBuffer* commandBuffer = CommandQueue->commandBuffer();

	MTL::BlitCommandEncoder* blitCommandEncoder = commandBuffer->blitCommandEncoder();
	for (int i = 0; i < TextureCount; i++)
	{
		MTL::Texture* texture = textures[i];
		MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
		textureDescriptor->setTextureType(texture->textureType());
		textureDescriptor->setPixelFormat(texture->pixelFormat());
		textureDescriptor->setWidth(texture->width());
		textureDescriptor->setHeight(texture->height());
		textureDescriptor->setDepth(texture->depth());
		textureDescriptor->setMipmapLevelCount(texture->mipmapLevelCount());
		textureDescriptor->setSampleCount(texture->sampleCount());
		textureDescriptor->setStorageMode(TextureHeap->storageMode());

		NS::SharedPtr<MTL::Texture> heapTexture = NS::TransferPtr(TextureHeap->newTexture(textureDescriptor));
		textureDescriptor->release();

		MTL::Region blitRegion = MTL::Region(0, 0, texture->width(), texture->height());
		for (auto level = 0; level < texture->mipmapLevelCount(); level++)
		{
			for (auto slice = 0; slice < texture->arrayLength(); slice++)
			{
				blitCommandEncoder->copyFromTexture(texture,
					slice,
					level,
					blitRegion.origin,
					blitRegion.size,
					heapTexture.get(),
					slice,
					level,
					blitRegion.origin);
			}

			blitRegion.size.width /= 2;
			blitRegion.size.height /= 2;

			if (blitRegion.size.width == 0)
				blitRegion.size.width = 1;

			if (blitRegion.size.height == 0)
				blitRegion.size.height = 1;
		}

		HeapTextures.push_back(heapTexture);
	}

	blitCommandEncoder->endEncoding();
	commandBuffer->commit();
	commandBuffer->waitUntilCompleted();
	blitCommandEncoder->release();
	commandBuffer->release();


	for (int i = 0; i < TextureCount; i++)
	{
		textures[i]->release();
		textures[i] = nullptr;
	}
	delete[] textures;
}

void Textures::CreateArgumentBuffers()
{
	// Tier 2 argument buffers
	if (Device->argumentBuffersSupport() == MTL::ArgumentBuffersTier2)
	{
		for (auto i = 0; i < BufferCount; i++)
		{
			const auto size = sizeof(FragmentArgumentBuffer);
			ArgumentBuffer[i] = NS::TransferPtr(Device
				->newBuffer(size, MTL::ResourceOptionCPUCacheModeDefault));

			NS::String
				* label = NS::String::string(fmt::format("Argument Buffer {}", i).c_str(), NS::UTF8StringEncoding);
			ArgumentBuffer[i]->setLabel(label);
			label->release();

			auto* buffer = reinterpret_cast<FragmentArgumentBuffer*>(ArgumentBuffer[i]->contents());
			// Bind each texture's GPU id into argument buffer for access in fragment shader
			for (auto j = 0; j < HeapTextures.size(); j++)
			{
				auto texture = HeapTextures[j];
				buffer->textures[j] = texture->gpuResourceID();

				buffer->transforms = (Matrix*)InstanceBuffer[i]->gpuAddress();
			}
			buffer->textureIndex = 0;
		}
	}
}

#if defined(__IPHONEOS__) || defined(__TVOS__)
int SDL_main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
	std::unique_ptr<Textures> example = std::make_unique<Textures>();
	example->Run(argc, argv);

	return 0;
}