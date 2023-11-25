////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "TextureUtil.hpp"

#include <filesystem>
#include <ktx.h>

#include "FileUtil.hpp"

MTL::Texture* TextureUtil::LoadTextureFromFile(MTL::Device* device, const std::string& fileName)
{
	NS::Data* data = FileUtil::DataForResource(fileName);
	if (data == nullptr) {
		return nullptr;
	}

	MTL::Texture* result;
	std::filesystem::path filePath = fileName;
	if (filePath.extension() == ".ktx") {
		result = LoadTextureFromBytes(device, data->bytes(), data->length());
	} else {
		throw std::runtime_error("Invalid texture format. Please make sure format is KTX compressed.");
	}

	data->release();
	return result;
}


MTL::Texture* TextureUtil::LoadTextureFromBytes(MTL::Device* device, const void* bytes, size_t numBytes)
{
	KTX_error_code result;
	uint32_t flags = KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT |
					 KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT;
	ktxTexture2* ktx2Texture = nullptr;
	result = ktxTexture2_CreateFromMemory((ktx_uint8_t*) bytes, numBytes, flags, &ktx2Texture);
	if (result != KTX_SUCCESS) {
		return nullptr;
	}

	if (ktxTexture2_NeedsTranscoding(ktx2Texture)) {

		khr_df_model_e colorModel = ktxTexture2_GetColorModel_e(ktx2Texture);

		ktx_transcode_fmt_e tf = KTX_TTF_NOSELECTION;
		if (colorModel == KHR_DF_MODEL_UASTC) {
			tf = KTX_TTF_ASTC_4x4_RGBA;
		}

		result = ktxTexture2_TranscodeBasis(ktx2Texture, tf, 0);
	}


	MTL::TextureType type = MTL::TextureType2D;
	MTL::PixelFormat pixelFormat = MTL::PixelFormatASTC_4x4_sRGB;

	BOOL genMipmaps = ktx2Texture->generateMipmaps;
	NS::UInteger levelCount = ktx2Texture->numLevels;
	NS::UInteger baseWidth = ktx2Texture->baseWidth;
	NS::UInteger baseHeight = ktx2Texture->baseHeight;
	NS::UInteger baseDepth = ktx2Texture->baseDepth;
	auto maxMipLevelCount = static_cast<NS::UInteger>(std::floor(std::log2(std::fmax(baseWidth, baseHeight))) + 1);
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

	MTL::Texture* texture = device->newTexture(textureDescriptor);

	auto* ktx1Texture = reinterpret_cast<ktxTexture*>(ktx2Texture);
	ktx_uint32_t layer = 0, faceSlice = 0;

	for (ktx_uint32_t level = 0; level < ktx2Texture->numLevels; ++level) {
		ktx_size_t offset = 0;
		result = ktxTexture_GetImageOffset(ktx1Texture, level, layer, faceSlice, &offset);
		ktx_uint8_t* imageBytes = ktxTexture_GetData(ktx1Texture) + offset;
		ktx_uint32_t bytesPerRow = ktxTexture_GetRowPitch(ktx1Texture, level);
		ktx_size_t bytesPerImage = ktxTexture_GetImageSize(ktx1Texture, level);
		auto levelWidth = static_cast<size_t>(std::fmax(1.0f, (float) (baseWidth >> level)));
		auto levelHeight = static_cast<size_t>(std::fmax(1.0f, (float) (baseHeight >> level)));

		texture->replaceRegion(MTL::Region(0, 0, levelWidth, levelHeight),
			level, faceSlice, imageBytes, bytesPerRow, bytesPerImage);

	}

	ktxTexture_Destroy((ktxTexture*) ktx1Texture);
	return texture;
}