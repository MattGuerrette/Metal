////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#ifdef USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <cstddef>
#include <format>
#include <memory>
#include <print>
#include <ranges>
#include <span>
#include <utility>

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include <imgui.h>

#include "Camera.hpp"
#include "Example.hpp"
#include "File.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

XM_ALIGNED_STRUCT(16) Vertex
{
    Vector4 position;
    Vector2 texCoord;
};

XM_ALIGNED_STRUCT(16) Uniforms
{
    [[maybe_unused]] Matrix modelViewProjection;
};

static constexpr size_t g_textureCount = 5;

XM_ALIGNED_STRUCT(16) FragmentArgumentBuffer
{
    [[maybe_unused]] std::array<MTL::ResourceID, g_textureCount> textures;
    [[maybe_unused]] Matrix*                                     transforms;
};

static constexpr std::array g_comboItems
    = { "Texture 0", "Texture 1", "Texture 2", "Texture 3", "Texture 4" };

class Textures final : public Example
{
    static constexpr int s_instanceCount = 3;

public:
    Textures();

    ~Textures() override;

    bool onLoad() override;

    void onUpdate(const GameTimer& timer) override;

    void onSetupUi(const GameTimer& timer) override;

    void onResize(uint32_t width, uint32_t height) override;

    void onRender(CA::MetalDrawable* drawable,
        MTL4::CommandBuffer*         commandBuffer,
        const GameTimer&             timer) override;

private:
    void createArgumentTable();

    void createResidencySet();

    void createBuffers();

    void createPipelineState();

    void createTextureHeap();

    void createArgumentBuffers();

    void updateUniforms() const;

    [[nodiscard]] MTL::Texture* newTextureFromFile(const std::string& fileName) const;

    NS::SharedPtr<MTL::RenderPipelineState>               m_pipelineState;
    NS::SharedPtr<MTL::Buffer>                            m_vertexBuffer;
    NS::SharedPtr<MTL::Buffer>                            m_indexBuffer;
    std::array<NS::SharedPtr<MTL::Buffer>, s_bufferCount> m_instanceBuffer;
    NS::SharedPtr<MTL4::ArgumentTable>                    m_argumentTable;
    NS::SharedPtr<MTL::ResidencySet>                      m_residencySet;
    NS::SharedPtr<MTL::SharedEvent>                       m_computeEvent;
    std::unique_ptr<Camera>                               m_mainCamera;
    NS::SharedPtr<MTL::Heap>                              m_textureHeap;
    std::array<NS::SharedPtr<MTL::Buffer>, s_bufferCount> m_argumentBuffer;
    std::vector<NS::SharedPtr<MTL::Texture>>              m_heapTextures;
    float                                                 m_rotationX = 0.0F;
    float                                                 m_rotationY = 0.0F;
};

Textures::Textures()
    : Example("Textures", 800, 600)
{
}

Textures::~Textures() = default;

bool Textures::onLoad()
{
    const auto      width = windowWidth();
    const auto      height = windowHeight();
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0F);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;

    m_mainCamera = std::make_unique<Camera>(
        Vector3::Zero, Vector3::Forward, Vector3::Up, fov, aspect, near, far);

    createArgumentTable();

    createResidencySet();

    createBuffers();

    createPipelineState();

    m_computeEvent = NS::TransferPtr(device()->newSharedEvent());
    m_computeEvent->setSignaledValue(0);

    createTextureHeap();

    createArgumentBuffers();

    // populate residency set and bind to command queue
    m_residencySet->addAllocation(m_vertexBuffer.get());
    m_residencySet->addAllocation(m_indexBuffer.get());
    for (const auto& buffer : m_instanceBuffer)
    {
        m_residencySet->addAllocation(buffer.get());
    }

    commandQueue()->addResidencySet(m_residencySet.get());
    commandQueue()->addResidencySet(metalLayer()->residencySet());

    m_residencySet->commit();

    return true;
}

void Textures::onSetupUi(const GameTimer& timer)
{
    // TODO: Re-enable once Metal 4 support is added to ImGUI
    //    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    //    ImGui::SetNextWindowPos(ImVec2(10, 20));
    //    ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver);
    //    ImGui::Begin("Metal Example", nullptr,
    //        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    //    ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(window()), timer.framesPerSecond());
    //    if (ImGui::Combo(" ", &m_selectedTexture, g_comboItems.data(), g_comboItems.size()))
    //    {
    //        /// Update argument buffer index
    //        for (const auto& buffer : m_argumentBuffer)
    //        {
    //            auto* contents = static_cast<FragmentArgumentBuffer*>(buffer->contents());
    //            contents->textureIndex = m_selectedTexture;
    //        }
    //    }
    // #if defined(SDL_PLATFORM_MACOS)
    //    ImGui::Text("Press Esc to quit");
    // #endif
    //    ImGui::End();
    //    ImGui::PopStyleVar();
}

void Textures::onUpdate(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.elapsedSeconds());

    if (mouse().isLeftPressed())
    {
        m_rotationY += static_cast<float>(mouse().relativeX()) * elapsed;
    }

    updateUniforms();
}

void Textures::onResize(const uint32_t width, const uint32_t height)
{
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0F);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;
    m_mainCamera->setProjection(fov, aspect, near, far);
}

MTL::Texture* Textures::newTextureFromFile(const std::string& fileName) const
{
    MTL::Texture* texture = nullptr;

    try
    {
        const File file(fileName);
        const auto bytes = file.readAll();

        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_uc* imageData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()),
            static_cast<int>(bytes.size()), &width, &height, &channels, 4);

        if (imageData == nullptr)
        {
            std::println("Failed to load image via stb_image: {}", stbi_failure_reason());
            return nullptr;
        }

        constexpr MTL::TextureType type = MTL::TextureType2D;
        constexpr MTL::PixelFormat pixelFormat = MTL::PixelFormatRGBA8Unorm_sRGB;

        NS::SharedPtr<MTL::TextureDescriptor> textureDescriptor
            = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
        textureDescriptor->setTextureType(type);
        textureDescriptor->setPixelFormat(pixelFormat);
        textureDescriptor->setWidth(width);
        textureDescriptor->setHeight(height);
        textureDescriptor->setDepth(1);
        textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
        textureDescriptor->setStorageMode(MTL::StorageModeShared);
        textureDescriptor->setArrayLength(1);
        textureDescriptor->setMipmapLevelCount(1);

        texture = device()->newTexture(textureDescriptor.get());
        if (texture != nullptr)
        {
            const NS::UInteger bytesPerRow = static_cast<NS::UInteger>(width) * 4;
            const NS::UInteger bytesPerImage = bytesPerRow * static_cast<NS::UInteger>(height);
            texture->replaceRegion(
                MTL::Region(0, 0, width, height), 0, 0, imageData, bytesPerRow, bytesPerImage);
        }

        stbi_image_free(imageData);
    }
    catch (const std::runtime_error& error)
    {
        std::println("{}", error.what());
    }

    return texture;
}

void Textures::onRender(CA::MetalDrawable* drawable,
    MTL4::CommandBuffer*                   commandBuffer,
    [[maybe_unused]] const GameTimer&      timer)
{

    const auto currentFrameIndex = frameIndex();

    NS::SharedPtr<MTL4::RenderPassDescriptor> renderPassDescriptor
        = NS::TransferPtr(defaultRenderPassDescriptor(drawable));

    MTL4::RenderCommandEncoder* commandEncoder
        = commandBuffer->renderCommandEncoder(renderPassDescriptor.get());

    commandEncoder->pushDebugGroup(MTLSTR("Texture Rendering"));

    commandEncoder->setRenderPipelineState(m_pipelineState.get());
    commandEncoder->setDepthStencilState(depthStencilState());
    commandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
    commandEncoder->setCullMode(MTL::CullModeNone);
    commandEncoder->setArgumentTable(m_argumentTable.get(), MTL::RenderStageVertex);

    m_argumentTable->setAddress(m_vertexBuffer->gpuAddress(), 0);
    m_argumentTable->setAddress(m_argumentBuffer[currentFrameIndex]->gpuAddress(), 1);

    commandEncoder->setArgumentTable(m_argumentTable.get(), MTL::RenderStageFragment);

    m_argumentTable->setAddress(m_argumentBuffer[currentFrameIndex]->gpuAddress(), 2);

    commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
        m_indexBuffer->length() / sizeof(uint16_t), MTL::IndexTypeUInt16,
        m_indexBuffer->gpuAddress(), m_indexBuffer->length(), s_instanceCount);

    commandEncoder->popDebugGroup();
    commandEncoder->endEncoding();
}

void Textures::createResidencySet()
{
    NS::Error* error = nullptr;

    NS::SharedPtr<MTL::ResidencySetDescriptor> residencySetDescriptor
        = NS::TransferPtr(MTL::ResidencySetDescriptor::alloc()->init());
    m_residencySet
        = NS::TransferPtr(device()->newResidencySet(residencySetDescriptor.get(), &error));
    if (error != nullptr)
    {
        throw std::runtime_error(std::format(
            "Failed to create residence set: {}", error->localizedFailureReason()->utf8String()));
    }
}

void Textures::createArgumentTable()
{
    NS::Error* error = nullptr;

    NS::SharedPtr<MTL4::ArgumentTableDescriptor> argTableDescriptor
        = NS::TransferPtr(MTL4::ArgumentTableDescriptor::alloc()->init());
    argTableDescriptor->setMaxBufferBindCount(3);

    m_argumentTable = NS::TransferPtr(device()->newArgumentTable(argTableDescriptor.get(), &error));
    if (error != nullptr)
    {
        throw std::runtime_error(std::format(
            "Failed to create argument table: {}", error->localizedFailureReason()->utf8String()));
    }
}

void Textures::createPipelineState()
{
    NS::SharedPtr<MTL::VertexDescriptor> vertexDescriptor
        = NS::TransferPtr(MTL::VertexDescriptor::alloc()->init());

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

    NS::SharedPtr<MTL::RenderPipelineDescriptor> pipelineDescriptor
        = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
    pipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(s_defaultPixelFormat);
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
    pipelineDescriptor->setVertexFunction(shaderLibrary()->newFunction(
        NS::String::string("texture_vertex", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setFragmentFunction(shaderLibrary()->newFunction(
        NS::String::string("texture_fragment", NS::ASCIIStringEncoding)));
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor.get());
    pipelineDescriptor->setSampleCount(s_multisampleCount);

    NS::Error* error = nullptr;
    m_pipelineState
        = NS::TransferPtr(device()->newRenderPipelineState(pipelineDescriptor.get(), &error));
    if (error != nullptr)
    {
        throw std::runtime_error(std::format(
            "Failed to create pipeline state: {}", error->localizedFailureReason()->utf8String()));
    }
}

void Textures::createBuffers()
{
    constexpr std::array vertices
        = std::to_array<Vertex>({ { .position = { -1, -1, 0, 1 }, .texCoord = { 0, 0 } },
            { .position = { -1, 1, 0, 1 }, .texCoord = { 0, 1 } },
            { .position = { 1, -1, 0, 1 }, .texCoord = { 1, 0 } },
            { .position = { 1, 1, 0, 1 }, .texCoord = { 1, 1 } } });
    constexpr size_t vertexBufferLength = vertices.size() * sizeof(Vertex);

    constexpr std::array indices = std::to_array<uint16_t>({ 0, 1, 2, 2, 1, 3 });
    constexpr size_t     indexBufferLength = indices.size() * sizeof(uint16_t);

    m_vertexBuffer = NS::TransferPtr(device()->newBuffer(
        vertices.data(), vertexBufferLength, MTL::ResourceCPUCacheModeDefaultCache));
    m_vertexBuffer->setLabel(NS::String::string("Vertices", NS::ASCIIStringEncoding));

    m_indexBuffer = NS::TransferPtr(device()->newBuffer(
        indices.data(), indexBufferLength, MTL::ResourceCPUCacheModeDefaultCache));
    m_indexBuffer->setLabel(NS::String::string("Indices", NS::ASCIIStringEncoding));

    constexpr size_t instanceDataSize
        = static_cast<unsigned long>(s_bufferCount * s_instanceCount) * sizeof(Matrix);
    for (const auto [index, buffer] : std::views::zip(std::views::iota(0u), m_instanceBuffer))
    {
        const auto                      label = std::format("Instance Buffer: {}", index);
        const NS::SharedPtr<NS::String> nsLabel
            = NS::TransferPtr(NS::String::string(label.c_str(), NS::ASCIIStringEncoding));

        buffer = NS::TransferPtr(
            device()->newBuffer(instanceDataSize, MTL::ResourceCPUCacheModeDefaultCache));
        buffer->setLabel(nsLabel.get());
    }
}

void Textures::updateUniforms() const
{
    const auto currentFrameIndex = frameIndex();

    MTL::Buffer* instanceBuffer = m_instanceBuffer[currentFrameIndex].get();

    auto*             instanceData = static_cast<Matrix*>(instanceBuffer->contents());
    std::span<Matrix> instanceSpan(instanceData, s_bufferCount);
    for (auto [index, data] : std::views::zip(std::views::iota(0u), instanceSpan))
    {
        auto position = Vector3(-5.0F + 5.0F * static_cast<float>(index), 0.0F, -8.0F);
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

        data = model * cameraUniforms.viewProjection;
    }
}

void Textures::createTextureHeap()
{
    std::vector<NS::SharedPtr<MTL::Texture>> textures;
    textures.resize(g_textureCount);
    for (auto [i, texture] : std::views::zip(std::views::iota(0u), textures))
    {
        const auto fileName = std::format("00{}_basecolor.png", i + 1);

        // Load PNG textures using stb_image
        texture = NS::TransferPtr(newTextureFromFile(fileName));
    }

    MTL::HeapDescriptor* heapDescriptor = MTL::HeapDescriptor::alloc()->init();
    heapDescriptor->setType(MTL::HeapTypeAutomatic);
    heapDescriptor->setStorageMode(MTL::StorageModePrivate);
    heapDescriptor->setSize(0);

    // Allocate space in the heap for each texture
    NS::UInteger heapSize = 0;
    for (const auto& texturePtr : textures)
    {
        const MTL::Texture* texture = texturePtr.get();
        if (texture == nullptr)
        {
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

        auto [size, align] = device()->heapTextureSizeAndAlign(textureDescriptor);
        heapSize += size;

        textureDescriptor->release();
    }
    heapDescriptor->setSize(heapSize);

    m_textureHeap = NS::TransferPtr(device()->newHeap(heapDescriptor));
    heapDescriptor->release();

    NS::SharedPtr<MTL::CommandQueue>  _commandQueue = NS::TransferPtr(device()->newCommandQueue());
    NS::SharedPtr<MTL::CommandBuffer> _commandBuffer
        = NS::TransferPtr(_commandQueue->commandBuffer());
    MTL::BlitCommandEncoder* blitCommandEncoder = _commandBuffer->blitCommandEncoder();
    for (const auto& texturePtr : textures)
    {
        const MTL::Texture*     texture = texturePtr.get();
        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setTextureType(texture->textureType());
        textureDescriptor->setPixelFormat(texture->pixelFormat());
        textureDescriptor->setWidth(texture->width());
        textureDescriptor->setHeight(texture->height());
        textureDescriptor->setDepth(texture->depth());
        textureDescriptor->setMipmapLevelCount(texture->mipmapLevelCount());
        textureDescriptor->setSampleCount(texture->sampleCount());
        textureDescriptor->setStorageMode(m_textureHeap->storageMode());

        NS::SharedPtr<MTL::Texture> heapTexture
            = NS::TransferPtr(m_textureHeap->newTexture(textureDescriptor));
        textureDescriptor->release();

        auto blitRegion = MTL::Region(0, 0, texture->width(), texture->height());
        for (auto level : std::views::iota(0uL, texture->mipmapLevelCount()))
        {
            for (auto slice : std::views::iota(0uL, texture->arrayLength()))
            {
                blitCommandEncoder->copyFromTexture(texture, slice, level, blitRegion.origin,
                    blitRegion.size, heapTexture.get(), slice, level, blitRegion.origin);
            }

            blitRegion.size.width /= 2;
            blitRegion.size.height /= 2;

            if (blitRegion.size.width == 0)
            {
                blitRegion.size.width = 1;
            }

            if (blitRegion.size.height == 0)
            {
                blitRegion.size.height = 1;
            }
        }

        m_heapTextures.push_back(heapTexture);
    }

    blitCommandEncoder->endEncoding();
    _commandBuffer->commit();
    _commandBuffer->waitUntilCompleted();

    blitCommandEncoder->release();
    m_residencySet->addAllocation(m_textureHeap.get());
}

void Textures::createArgumentBuffers()
{
    // Tier 2 argument buffers
    if (device()->argumentBuffersSupport() == MTL::ArgumentBuffersTier2)
    {
        for (auto [i, argumentBuffer] : std::views::zip(std::views::iota(0u), m_argumentBuffer))
        {
            constexpr auto size = sizeof(FragmentArgumentBuffer);
            argumentBuffer
                = NS::TransferPtr(device()->newBuffer(size, MTL::ResourceCPUCacheModeDefaultCache));

            NS::String* label = NS::String::string(
                std::format("Argument Buffer {}", i).c_str(), NS::UTF8StringEncoding);
            argumentBuffer->setLabel(label);
            label->release();

            auto* buffer = static_cast<FragmentArgumentBuffer*>(argumentBuffer->contents());
            // Bind each texture's GPU id into argument buffer for access in fragment shader
            buffer->transforms = reinterpret_cast<Matrix*>(m_instanceBuffer[i]->gpuAddress());
            for (auto [j, texture] : std::views::zip(std::views::iota(0u), m_heapTextures))
            {
                buffer->textures[j] = texture->gpuResourceID();
            }

            m_residencySet->addAllocation(argumentBuffer.get());
            m_argumentTable->setAddress(argumentBuffer->gpuAddress(), 0);
        }
    }
}

extern "C" {

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    try
    {
        auto* example = new Textures();
        if (!example->startup())
        {
            delete example;
            return SDL_APP_FAILURE;
        }
        *appstate = example;
        return SDL_APP_CONTINUE;
    }
    catch (const std::exception& e)
    {
        std::println("Error during initialization: {}", e.what());
        return SDL_APP_FAILURE;
    }
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* example = static_cast<Textures*>(appstate);
    example->processEvent(*event);
    if (!example->isRunning())
    {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* example = static_cast<Textures*>(appstate);
    if (!example->isRunning())
    {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, [[maybe_unused]] SDL_AppResult result)
{
    if (appstate != nullptr)
    {
        auto* example = static_cast<Textures*>(appstate);
        delete example;
    }
}
}
