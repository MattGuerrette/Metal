////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include <memory>

#include <AppKit/AppKit.hpp>
#include <Metal/Metal.hpp>

#include <fmt/format.h>
#include <imgui.h>

#include "Camera.hpp"
#include "Example.hpp"
#include "File.hpp"
#include "GLTFAsset.hpp"

#include <SDL3/SDL_main.h>

XM_ALIGNED_STRUCT(16) InstanceData
{
    Matrix transform;
};

XM_ALIGNED_STRUCT(16) SkinnedMeshArgumentBuffer
{
    InstanceData* data;
    Matrix        bone;
};

class Skinning : public Example
{
public:
    static Skinning* s_example;

    Skinning();

    ~Skinning() override;

    bool onLoad() override;

    void onUpdate(const GameTimer& timer) override;

    void onSetupUi(const GameTimer& timer) override;

    void onResize(uint32_t width, uint32_t height) override;

    void onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer) override;

private:
    void createBuffers();

    void createPipelineState();

    void createArgumentBuffers();

    void updateUniforms() const;

#ifdef SDL_PLATFORM_MACOS
    NS::Menu* createMenuBar() override;
#endif

    void loadAsset(const char* filePath);

    std::unique_ptr<Camera>                               m_mainCamera;
    std::unique_ptr<GLTFAsset>                            m_asset;
    NS::SharedPtr<MTL::RenderPipelineState>               m_pipelineState;
    std::array<NS::SharedPtr<MTL::Buffer>, s_bufferCount> m_instanceBuffer;
    std::array<NS::SharedPtr<MTL::Buffer>, s_bufferCount> m_argumentBuffer;
    float                                                 m_rotationX = 0.0F;
    float                                                 m_rotationY = 0.0F;
    int                                                   m_selectedTexture = 0;
    float                                                 m_animationTime = 0.0F;
    std::vector<Matrix>                                   m_bones;
};

Skinning* Skinning::s_example = nullptr;

Skinning::Skinning()
    : Example("GLTF Skinning", 800, 600)
{
    s_example = this;
}

Skinning::~Skinning() = default;

void Skinning::onResize(uint32_t width, uint32_t height)
{
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0F);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;
    m_mainCamera->setProjection(fov, aspect, near, far);
}

#ifdef SDL_PLATFORM_MACOS
NS::Menu* Skinning::createMenuBar()
{
    using NS::StringEncoding::UTF8StringEncoding;
    auto* mainMenu = NS::Menu::alloc()->init();
    auto* appMenu = NS::Menu::alloc()->init(NS::String::string("Appname", UTF8StringEncoding));
    auto* appMenuItem = NS::MenuItem::alloc()->init();
    auto* appName = NS::RunningApplication::currentApplication()->localizedName();
    NS::String* quitItemName
        = NS::String::string("Quit ", UTF8StringEncoding)->stringByAppendingString(appName);
    // callback object pointer that closes application
    SEL quitCallBackFunc = NS::MenuItem::registerActionCallback(
        "appQuit", [](void*, SEL, const NS::Object* pSender) {
            auto* appInstance = NS::Application::sharedApplication();
            appInstance->terminate(pSender);
        });

    NS::MenuItem* appQuitItem = appMenu->addItem(
        quitItemName, quitCallBackFunc, NS::String::string("q", UTF8StringEncoding));
    appQuitItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    appMenuItem->setSubmenu(appMenu);

    auto* fileMenu = NS::Menu::alloc()->init(NS::String::string("File", UTF8StringEncoding));
    auto* fileMenuItem = NS::MenuItem::alloc()->init();

    SEL openFileCallback
        = NS::MenuItem::registerActionCallback("openFile", [](void*, SEL, const NS::Object*) {
              auto* appInstance = NS::Application::sharedApplication();
              auto* window = reinterpret_cast<NS::Window*>(appInstance->windows()->object(0));
              NS::OpenPanel* panel = NS::OpenPanel::openPanel();
              panel->beginSheetModal(window, ^(NS::ModalResponseType response) {
                  const auto* urls = panel->urls();
                  const auto  numUrls = urls->count();
                  for (NS::UInteger i = 0; std::cmp_less(i, numUrls); ++i)
                  {
                      const auto* url = static_cast<NS::URL*>(urls->object(i));
                      const auto* path = url->fileSystemRepresentation();

                      s_example->loadAsset(path);
                  }
                  // urls->release();
              });
          });

    auto* openFileItem = fileMenu->addItem(NS::String::string("Open", UTF8StringEncoding),
        openFileCallback, NS::String::string("o", UTF8StringEncoding));
    openFileItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    fileMenuItem->setSubmenu(fileMenu);

    // add menu items to main menu
    mainMenu->addItem(appMenuItem);
    mainMenu->addItem(fileMenuItem);

    // release memory
    appMenuItem->release();
    fileMenuItem->release();
    appMenu->release();
    fileMenu->release();

    // return ptr holding main menu
    return mainMenu->autorelease();
}
#endif

void Skinning::loadAsset(const char* filePath)
{
    m_asset = std::make_unique<GLTFAsset>(device(), std::filesystem::path(filePath));
}

bool Skinning::onLoad()
{
    const auto      width = windowWidth();
    const auto      height = windowHeight();
    const float     aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr float fov = XMConvertToRadians(75.0F);
    constexpr float near = 0.01F;
    constexpr float far = 1000.0F;

    m_mainCamera = std::make_unique<Camera>(
        Vector3::Zero, Vector3::Forward, Vector3::Up, fov, aspect, near, far);

    createBuffers();

    createPipelineState();

    try
    {
        m_asset = std::make_unique<GLTFAsset>(device(), std::string("SimpleSkin.gltf"));
    }
    catch (std::exception& e)
    {
        fmt::println("Failed to load asset: {}", e.what());
        return false;
    }

    createArgumentBuffers();

    return true;
}

void Skinning::onSetupUi(const GameTimer& timer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    ImGui::SetNextWindowPos(ImVec2(10, 20));
    // ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Metal Example", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(window()), timer.framesPerSecond());
    std::vector<std::string> animations = m_asset->animations();
    if (ImGui::Combo(
            "Animation", &m_selectedTexture,
            [](void* data, int index) -> const char* {
                const auto* names = static_cast<std::vector<std::string>*>(data);
                return names->at(index).c_str();
            },
            &animations, static_cast<int>(animations.size())))
    {
        //		/// Update argument buffer index
        //		for (const auto& buffer: ArgumentBuffer) {
        //			auto* contents =
        // reinterpret_cast<FragmentArgumentBuffer*>(buffer->contents());
        // contents->TextureIndex = SelectedTexture;
        //		}
        m_animationTime = 0.0F;
    }

    const auto* animation = m_asset->getAnimation(m_selectedTexture);
    if (animation != nullptr)
    {
        if (ImGui::SliderFloat(
                "##", &m_animationTime, 0.0F, animation->channels[0].sampler->input->max[0]))
        {
        }

        if (ImGui::TreeNode("Joints"))
        {
            m_asset->drawUi();

            ImGui::TreePop();
        }
    }
#if defined(SDL_PLATFORM_MACOS)
    ImGui::Text("Press Esc to quit");
#endif
    ImGui::End();
    ImGui::PopStyleVar();
}

void Skinning::onUpdate(const GameTimer& timer)
{
    const auto elapsed = static_cast<float>(timer.elapsedSeconds());
    if (mouse().isLeftPressed())
    {
        m_rotationY += static_cast<float>(mouse().relativeX()) * elapsed;
    }

    // TODO: Re-add back gamepad support
    // if (m_gamepad)
    // {
    //     m_rotationY += static_cast<float>(m_gamepad->leftThumbstickHorizontal()) * elapsed;
    // }
}

void Skinning::onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& /*timer*/)
{
    updateUniforms();

    const auto currentFrameIndex = frameIndex();

    commandEncoder->useResource(m_instanceBuffer[currentFrameIndex].get(), MTL::ResourceUsageRead);
    commandEncoder->setRenderPipelineState(m_pipelineState.get());
    commandEncoder->setDepthStencilState(depthStencilState());
    // TODO: Render skinned mesh with specified animation
    if (m_asset)
    {
        commandEncoder->setFragmentBuffer(m_argumentBuffer[currentFrameIndex].get(), 0, 0);
        commandEncoder->setVertexBuffer(m_argumentBuffer[currentFrameIndex].get(), 0, 1);
        m_asset->render(commandEncoder);
    }
}

void Skinning::createPipelineState()
{
    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    // Position
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(0)->setOffset(0);
    vertexDescriptor->attributes()->object(0)->setBufferIndex(0);

    // Color
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(1)->setOffset(offsetof(Vertex, color));
    vertexDescriptor->attributes()->object(1)->setBufferIndex(0);

    // Texcoord
    vertexDescriptor->attributes()->object(2)->setFormat(MTL::VertexFormatFloat2);
    vertexDescriptor->attributes()->object(2)->setOffset(offsetof(Vertex, texcoord));
    vertexDescriptor->attributes()->object(2)->setBufferIndex(0);

    // Joint
    vertexDescriptor->attributes()->object(3)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(3)->setOffset(offsetof(Vertex, joint));
    vertexDescriptor->attributes()->object(3)->setBufferIndex(0);

    // Weight
    vertexDescriptor->attributes()->object(4)->setFormat(MTL::VertexFormatFloat4);
    vertexDescriptor->attributes()->object(4)->setOffset(offsetof(Vertex, weight));
    vertexDescriptor->attributes()->object(4)->setBufferIndex(0);

    vertexDescriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    vertexDescriptor->layouts()->object(0)->setStride(sizeof(Vertex));

    MTL::Function* vertexFunction = shaderLibrary()->newFunction(
        NS::String::string("skinning_vertex", NS::ASCIIStringEncoding));
    MTL::Function* fragmentFunction = shaderLibrary()->newFunction(
        NS::String::string("skinning_fragment", NS::ASCIIStringEncoding));

    MTL::RenderPipelineDescriptor* pipelineDescriptor
        = MTL::RenderPipelineDescriptor::alloc()->init();
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
    pipelineDescriptor->setVertexFunction(vertexFunction);
    pipelineDescriptor->setFragmentFunction(fragmentFunction);
    pipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    pipelineDescriptor->setSampleCount(s_multisampleCount);

    NS::Error* error = nullptr;
    m_pipelineState = NS::TransferPtr(device()->newRenderPipelineState(pipelineDescriptor, &error));
    if (error != nullptr)
    {
        throw std::runtime_error(fmt::format(
            "Failed to create pipeline state: {}", error->localizedFailureReason()->utf8String()));
    }

    vertexDescriptor->release();
    pipelineDescriptor->release();
}

void Skinning::createBuffers()
{
    constexpr size_t instanceDataSize = s_bufferCount * sizeof(InstanceData);
    for (auto index = 0; std::cmp_less(index, s_bufferCount); ++index)
    {
        const auto                      label = fmt::format("Instance Buffer: {}", index);
        const NS::SharedPtr<NS::String> nsLabel
            = NS::TransferPtr(NS::String::string(label.c_str(), NS::ASCIIStringEncoding));
        m_instanceBuffer[index] = NS::TransferPtr(
            device()->newBuffer(instanceDataSize, MTL::ResourceOptionCPUCacheModeDefault));
        m_instanceBuffer[index]->setLabel(nsLabel.get());
    }
}

void Skinning::updateUniforms() const
{
    const auto   currentFrameIndex = frameIndex();
    MTL::Buffer* instanceBuffer = m_instanceBuffer[currentFrameIndex].get();

    // MTL::Buffer* argumentBuffer = m_argumentBuffer[currentFrameIndex].get();

    auto* instanceData = static_cast<InstanceData*>(instanceBuffer->contents());

    // auto* argData = static_cast<SkinnedMeshArgumentBuffer*>(argumentBuffer->contents());

    for (auto index = 0; std::cmp_less(index, 1); ++index)
    {
        auto position = Vector3(0.0F, -5.0F, -20.0F);
        auto rotationX = m_rotationX;
        auto rotationY = m_rotationY;
        auto scaleFactor = 10.0F;

        const Vector3 xAxis = Vector3::Right;
        const Vector3 yAxis = Vector3::Up;

        const Matrix         xRot = Matrix::CreateFromAxisAngle(xAxis, rotationX);
        const Matrix         yRot = Matrix::CreateFromAxisAngle(yAxis, rotationY);
        const Matrix         rotation = xRot * yRot;
        const Matrix         translation = Matrix::CreateTranslation(position);
        const Matrix         scale = Matrix::CreateScale(scaleFactor);
        const Matrix         model = scale * rotation * translation;
        const CameraUniforms cameraUniforms = m_mainCamera->uniforms();

        instanceData[index].transform = model * cameraUniforms.viewProjection;

        // argData->bone = m_bones.front().Transpose();
        // argData->bones[0] = m_bones[0].Transpose();
        // argData->bones[1] = m_bones[1].Transpose();
    }
}

void Skinning::createArgumentBuffers()
{
    // TODO: Create any necessary argument buffers
    // Tier 2 argument buffers
    if (device()->argumentBuffersSupport() == MTL::ArgumentBuffersTier2)
    {
        for (auto i = 0; std::cmp_less(i, s_bufferCount); ++i)
        {
            constexpr auto size = sizeof(SkinnedMeshArgumentBuffer);
            m_argumentBuffer[i] = NS::TransferPtr(
                device()->newBuffer(size, MTL::ResourceOptionCPUCacheModeDefault));

            NS::String* label = NS::String::string(
                fmt::format("Argument Buffer {}", i).c_str(), NS::UTF8StringEncoding);
            m_argumentBuffer[i]->setLabel(label);
            label->release();

            auto* buffer = static_cast<SkinnedMeshArgumentBuffer*>(m_argumentBuffer[i]->contents());
            buffer->data = reinterpret_cast<InstanceData*>(m_instanceBuffer[i]->gpuAddress());
            // buffer->bone = m_bones.front();
            //             // Bind each texture's GPU id into argument buffer for access in fragment
            //             shader
            //             for (auto j = 0; j < m_heapTextures.size(); j++)
            //             {
            //                 auto texture = m_heapTextures[j];
            //                 buffer->textures[j] = texture->gpuResourceID();
            //
            //                 buffer->transforms = (Matrix*)m_instanceBuffer[i]->gpuAddress();
            //             }
            //             buffer->textureIndex = 0;
        }
    }
    else
    {
        // TODO: Add support for Tier1 argument buffers?
        // Or maybe just wait for Apple to phase out
        // support for Tier1 hardware.
    }
}

int main(int argc, char** argv)
{
    int result = EXIT_FAILURE;
    try
    {
        const auto example = std::make_unique<Skinning>();
        result = example->run(argc, argv);
    }
    catch (const std::runtime_error&)
    {
        fmt::println("Exiting...");
    }

    return result;
}
