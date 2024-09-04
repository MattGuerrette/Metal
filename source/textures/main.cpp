////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <ktx.h>

#include <cstddef>
#include <fmt/core.h>
#include <memory>

#include <Metal/Metal.hpp>

#include <imgui.h>

#include "Camera.hpp"
#include "Example.hpp"
#include "File.hpp"

#include <SDL3/SDL_main.h>

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector4 position;
    Vector2 texCoord;
};

XM_ALIGNED_STRUCT(16) Uniforms { [[maybe_unused]] Matrix modelViewProjection; };

static constexpr size_t g_textureCount = 5;

XM_ALIGNED_STRUCT(16) FragmentArgumentBuffer
{
    [[maybe_unused]] std::array<MTL::ResourceID, g_textureCount> textures;
    [[maybe_unused]] uint32_t                                    textureIndex;
    [[maybe_unused]] Matrix*                                     transforms;
};

static const std::array<const char*, g_textureCount> g_comboItems
    = { "Texture 0", "Texture 1", "Texture 2", "Texture 3", "Texture 4" };

class Textures : public Example {
    static constexpr int s_instanceCount = 3;

public:
    Textures();

    ~Textures() override;

    bool onLoad() override;

    void onUpdate(const GameTimer& timer) override;

    void onSetupUi(const GameTimer& timer) override;

    void onResize(uint32_t width, uint32_t height) override;

    void onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) override;

private:
    void createBuffers();

    void createPipelineState();

    void createTextureHeap();

    void createArgumentBuffers();

    void updateUniforms();

    MTL::Texture* newTextureFromFile(const std::string& fileName);

    NS::SharedPtr<MTL::RenderPipelineState>               m_pipelineState;
    NS::SharedPtr<MTL::Buffer>                            m_vertexBuffer;
    NS::SharedPtr<MTL::Buffer>                            m_indexBuffer;
    std::array<NS::SharedPtr<MTL::Buffer>, s_bufferCount> m_instanceBuffer;
    std::unique_ptr<Camera>                               m_mainCamera;
    NS::SharedPtr<MTL::Heap>                              m_textureHeap;
    std::array<NS::SharedPtr<MTL::Buffer>, s_bufferCount> m_argumentBuffer;
    std::vector<NS::SharedPtr<MTL::Texture>>              m_heapTextures;
    float                                                 m_rotationX = 0.0F;
    float                                                 m_rotationY = 0.0F;
    int                                                   m_selectedTexture = 0;
};

Textures::Textures()
    : Example("Textures", 800, 600)
{
}

Textures::~Textures() = default;

bool Textures::onLoad()
{
    int32_t width;
    int32_t height;
    SDL_GetWindowSizeInPixels(m_window, &width, &height);
    const float aspect = (float)width / (float)height;
    const float fov = XMConvertToRadians(75.0F);
    const float near = 0.01F;
    const float far = 1000.0F;

    m_mainCamera = std::make_unique<Camera>(
        Vector3::Zero, Vector3::Forward, Vector3::Up, fov, aspect, near, far);

    createBuffers();

    createPipelineState();

    createTextureHeap();

    createArgumentBuffers();

    return true;
}

void Textures::onSetupUi(const GameTimer& timer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    ImGui::SetNextWindowPos(ImVec2(10, 20));
    ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Metal Example", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(m_window), timer.framesPerSecond());
    if (ImGui::Combo(" ", &m_selectedTexture, g_comboItems.data(), g_comboItems.size())) {
        /// Update argument buffer index
        for (const auto& buffer : m_argumentBuffer) {
            auto* contents = reinterpret_cast<FragmentArgumentBuffer*>(buffer->contents());
            contents->textureIndex = m_selectedTexture;
        }
    }
#if defined(SDL_PLATFORM_MACOS)
    ImGui::Text("Press Esc to quit");
#endif
    ImGui::End();
    ImGui::PopStyleVar();
}

void Textures::onUpdate(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.elapsedSeconds());
    // RotationX += elapsed;
    if (m_mouse->isLeftPressed()) {
        m_rotationY += static_cast<float>(m_mouse->relativeX()) * elapsed;
    }

    if (m_gamepad) {
        m_rotationY += m_gamepad->leftThumbstickHorizontal() * elapsed;
    }
}

void Textures::onResize(uint32_t width, uint32_t height)
{
    const float aspect = (float)width / (float)height;
    const float fov = XMConvertToRadians(75.0F);
    const float near = 0.01F;
    const float far = 1000.0F;
    m_mainCamera->setProjection(fov, aspect, near, far);
}

MTL::Texture* Textures::newTextureFromFile(const std::string& fileName)
{
    MTL::Texture* texture = nullptr;

    try {
        const File file(fileName);

        const auto bytes = file.readAll();

        KTX_error_code result;
        const uint32_t flags
            = KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT | KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT;
        ktxTexture2* ktx2Texture = nullptr;
        result = ktxTexture2_CreateFromMemory(
            (ktx_uint8_t*)bytes.data(), bytes.size(), flags, &ktx2Texture);
        if (result != KTX_SUCCESS) {
            return nullptr;
        }

        const MTL::TextureType type = MTL::TextureType2D;
        const MTL::PixelFormat pixelFormat = MTL::PixelFormatASTC_8x8_sRGB;
        const bool             genMipmaps = ktx2Texture->generateMipmaps;
        const NS::UInteger     levelCount = ktx2Texture->numLevels;
        const NS::UInteger     baseWidth = ktx2Texture->baseWidth;
        const NS::UInteger     baseHeight = ktx2Texture->baseHeight;
        const NS::UInteger     baseDepth = ktx2Texture->baseDepth;
        auto                   maxMipLevelCount = static_cast<NS::UInteger>(
            std::floor(std::log2(std::fmax(baseWidth, baseHeight))) + 1);
        const NS::UInteger storedMipLevelCount = genMipmaps ? maxMipLevelCount : levelCount;

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

        texture = m_device->newTexture(textureDescriptor);

        auto*              ktx1Texture = reinterpret_cast<ktxTexture*>(ktx2Texture);
        const ktx_uint32_t layer = 0;
        const ktx_uint32_t faceSlice = 0;
        for (ktx_uint32_t level = 0; level < ktx2Texture->numLevels; ++level) {
            ktx_size_t offset = 0;
            result = ktxTexture_GetImageOffset(ktx1Texture, level, layer, faceSlice, &offset);
            ktx_uint8_t*       imageBytes = ktxTexture_GetData(ktx1Texture) + offset;
            const ktx_uint32_t bytesPerRow = ktxTexture_GetRowPitch(ktx1Texture, level);
            const ktx_size_t   bytesPerImage = ktxTexture_GetImageSize(ktx1Texture, level);
            auto levelWidth = static_cast<size_t>(std::fmax(1.0F, (float)(baseWidth >> level)));
            auto levelHeight = static_cast<size_t>(std::fmax(1.0F, (float)(baseHeight >> level)));

            texture->replaceRegion(MTL::Region(0, 0, levelWidth, levelHeight), level, faceSlice,
                imageBytes, bytesPerRow, bytesPerImage);
        }

        ktxTexture_Destroy((ktxTexture*)ktx1Texture);
    } catch (const std::runtime_error& error) {
        fmt::println(error.what());
    }

    return texture;
}

void Textures::onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& /*timer*/)
{
    updateUniforms();

    commandEncoder->useHeap(m_textureHeap.get());
    commandEncoder->useResource(m_instanceBuffer[m_frameIndex].get(), MTL::ResourceUsageRead);
    commandEncoder->setRenderPipelineState(m_pipelineState.get());
    commandEncoder->setDepthStencilState(m_depthStencilState.get());
    commandEncoder->setFrontFacingWinding(MTL::WindingClockwise);
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setFragmentBuffer(m_argumentBuffer[m_frameIndex].get(), 0, 0);
    commandEncoder->setVertexBuffer(m_vertexBuffer.get(), 0, 0);
    commandEncoder->setVertexBuffer(m_argumentBuffer[m_frameIndex].get(), 0, 1);
    commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
        m_indexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16, m_indexBuffer.get(), 0,
        s_instanceCount);
}

void Textures::createPipelineState()
{
    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    // Position
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(0)->setOffset(0);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(0);

    // Color
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);
    vertexDescriptor->attributes()->object(1)->setOffset(offsetof(Vertex, texCoord));
    vertexDescriptor->attributes()->object(1)->setBufferIndex(0);

    vertexDescriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(Vertex));

    MTL::RenderPipelineDescriptor* pipelineDescriptor
        = MTL::RenderPipelineDescriptor::alloc()->init();
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(m_frameBufferPixelFormat);
    pipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(
        MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
        MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
    pipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(
        MTL::BlendFactorSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(
        MTL::BlendFactorOneMinusSourceAlpha);
    pipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(
        MTL::BlendOperationAdd);
    pipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setStencilAttachmentPixelFormat(MTL::PixelFormatDepth32Float_Stencil8);
    pipelineDescriptor->setVertexFunction(m_pipelineLibrary->newFunction(
        NS::String::string("texture_vertex", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(m_pipelineLibrary->newFunction(
        NS::String::string("texture_fragment", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);

    NS::Error* error = nullptr;
    m_pipelineState = NS::TransferPtr(m_device->newRenderPipelineState(pipelineDescriptor, &error));
    if (error != nullptr) {
        throw std::runtime_error(fmt::format(
            "Failed to create pipeline state: {}", error->localizedFailureReason()->utf8String()));
    }

    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void Textures::createBuffers()
{
    static const Vertex vertices[] = { { .position = { -1, -1, 0, 1 }, .texCoord = { 0, 0 } },
        { .position = { -1, 1, 0, 1 }, .texCoord = { 0, 1 } },
        { .position = { 1, -1, 0, 1 }, .texCoord = { 1, 0 } },
        { .position = { 1, 1, 0, 1 }, .texCoord = { 1, 1 } } };

    static const uint16_t indices[] = { 0, 1, 2, 2, 1, 3 };

    m_vertexBuffer = NS::TransferPtr(
        m_device->newBuffer(vertices, sizeof(vertices), MTL::ResourceCPUCacheModeDefaultCache));
    m_vertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    m_indexBuffer = NS::TransferPtr(
        m_device->newBuffer(indices, sizeof(indices), MTL::ResourceOptionCPUCacheModeDefault));
    m_indexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    const size_t instanceDataSize
        = static_cast<unsigned long>(s_bufferCount * s_instanceCount) * sizeof(Matrix);
    for (auto index = 0; index < s_bufferCount; index++) {
        const auto                      label = fmt::format("Instance Buffer: {}", index);
        const NS::SharedPtr<NS::String> nsLabel
            = NS::TransferPtr(NS::String::string(label.c_str(), NS::ASCIIStringEncoding));

        m_instanceBuffer[index] = NS::TransferPtr(
            m_device->newBuffer(instanceDataSize, MTL::ResourceOptionCPUCacheModeDefault));
        m_instanceBuffer[index]->setLabel(nsLabel.get());
    }
}

void Textures::updateUniforms()
{
    MTL::Buffer* instanceBuffer = m_instanceBuffer[m_frameIndex].get();

    auto* instanceData = static_cast<Matrix*>(instanceBuffer->contents());
    for (auto index = 0; index < s_instanceCount; ++index) {
        auto position = Vector3(-5.0F + (5.0F * (float)index), 0.0F, -8.0F);
        auto rotationX = m_rotationX;
        auto rotationY = m_rotationY;
        auto scaleFactor = 1.0F;

        const Vector3 xAxis = Vector3::Right;
        const Vector3 yAxis = Vector3::Up;

        const Matrix         xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
        const Matrix         yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
        const Matrix         rotation = xRot * yRot;
        const Matrix         translation = Matrix::CreateTranslation(position);
        const Matrix         scale = Matrix::CreateScale(scaleFactor);
        const Matrix         model = scale * rotation * translation;
        const CameraUniforms cameraUniforms = m_mainCamera->uniforms();

        instanceData[index] = model * cameraUniforms.viewProjection;
    }
}

void Textures::createTextureHeap()
{
    auto** textures = new MTL::Texture*[g_textureCount];
    for (size_t i = 0; i < g_textureCount; i++) {
        const auto fileName = fmt::format("00{}_basecolor.ktx", i + 1);
        textures[i] = newTextureFromFile(fileName);
    }

    MTL::HeapDescriptor* heapDescriptor = MTL::HeapDescriptor::alloc()->init();
    heapDescriptor->setType(MTL::HeapTypeAutomatic);
    heapDescriptor->setStorageMode(MTL::StorageModePrivate);
    heapDescriptor->setSize(0);

    // Allocate space in heap for each texture
    NS::UInteger heapSize = 0;
    for (size_t i = 0; i < g_textureCount; i++) {
        MTL::Texture* texture = textures[i];
        if (texture == nullptr) {
            continue;
        }

        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setTextureType(texture->textureType());
        textureDescriptor->setPixelFormat(texture->pixelFormat());
        textureDescriptor->setWidth(texture->width());
        textureDescriptor->setHeight(texture->height());
        textureDescriptor->setDepth(texture->depth());
        textureDescriptor->setMipmapLevelCount(texture->mipmapLevelCount());
        textureDescriptor->setSampleCount(texture->sampleCount());
        textureDescriptor->setStorageMode(MTL::StorageModePrivate);

        auto sizeAndAlignment = m_device->heapTextureSizeAndAlign(textureDescriptor);
        heapSize += sizeAndAlignment.size;

        textureDescriptor->release();
    }
    heapDescriptor->setSize(heapSize);

    m_textureHeap = NS::TransferPtr(m_device->newHeap(heapDescriptor));
    heapDescriptor->release();

    // Move texture memory into heap
    MTL::CommandBuffer* commandBuffer = m_commandQueue->commandBuffer();

    MTL::BlitCommandEncoder* blitCommandEncoder = commandBuffer->blitCommandEncoder();
    for (int i = 0; i < g_textureCount; i++) {
        MTL::Texture*           texture = textures[i];
        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setTextureType(texture->textureType());
        textureDescriptor->setPixelFormat(texture->pixelFormat());
        textureDescriptor->setWidth(texture->width());
        textureDescriptor->setHeight(texture->height());
        textureDescriptor->setDepth(texture->depth());
        textureDescriptor->setMipmapLevelCount(texture->mipmapLevelCount());
        textureDescriptor->setSampleCount(texture->sampleCount());
        textureDescriptor->setStorageMode(m_textureHeap->storageMode());

        const NS::SharedPtr<MTL::Texture> heapTexture
            = NS::TransferPtr(m_textureHeap->newTexture(textureDescriptor));
        textureDescriptor->release();

        MTL::Region blitRegion = MTL::Region(0, 0, texture->width(), texture->height());
        for (auto level = 0; level < texture->mipmapLevelCount(); level++) {
            for (auto slice = 0; slice < texture->arrayLength(); slice++) {
                blitCommandEncoder->copyFromTexture(texture, slice, level, blitRegion.origin,
                    blitRegion.size, heapTexture.get(), slice, level, blitRegion.origin);
            }

            blitRegion.size.width /= 2;
            blitRegion.size.height /= 2;

            if (blitRegion.size.width == 0) {
                blitRegion.size.width = 1;
            }

            if (blitRegion.size.height == 0) {
                blitRegion.size.height = 1;
            }
        }

        m_heapTextures.push_back(heapTexture);
    }

    blitCommandEncoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
    blitCommandEncoder->release();
    commandBuffer->release();

    for (int i = 0; i < g_textureCount; i++) {
        textures[i]->release();
        textures[i] = nullptr;
    }
    delete[] textures;
}

void Textures::createArgumentBuffers()
{
    // Tier 2 argument buffers
    if (m_device->argumentBuffersSupport() == MTL::ArgumentBuffersTier2) {
        for (auto i = 0; i < s_bufferCount; i++) {
            const auto size = sizeof(FragmentArgumentBuffer);
            m_argumentBuffer[i] = NS::TransferPtr(
                m_device->newBuffer(size, MTL::ResourceOptionCPUCacheModeDefault));

            NS::String* label = NS::String::string(
                fmt::format("Argument Buffer {}", i).c_str(), NS::UTF8StringEncoding);
            m_argumentBuffer[i]->setLabel(label);
            label->release();

            auto* buffer
                = reinterpret_cast<FragmentArgumentBuffer*>(m_argumentBuffer[i]->contents());
            // Bind each texture's GPU id into argument buffer for access in fragment shader
            for (auto j = 0; j < m_heapTextures.size(); j++) {
                auto texture = m_heapTextures[j];
                buffer->textures[j] = texture->gpuResourceID();

                buffer->transforms = (Matrix*)m_instanceBuffer[i]->gpuAddress();
            }
            buffer->textureIndex = 0;
        }
    } else {
        // TODO: Add support for Tier1 argument buffers?
        // Or maybe just wait for Apple to phase out
        // support for Tier1 hardware.
    }
}

int main(int argc, char** argv)
{
    int result = EXIT_FAILURE;
    try {
        auto example = std::make_unique<Textures>();
        result = example->run(argc, argv);
    } catch (const std::runtime_error& error) {
        fmt::println("Exiting...");
    }

    return result;
}
