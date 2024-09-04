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

    void updateUniforms();

#ifdef SDL_PLATFORM_MACOS
    NS::Menu* createMenuBar() override;
#endif

    void loadAsset(const char* filePath);

    std::unique_ptr<Camera>    m_mainCamera;
    std::unique_ptr<GLTFAsset> m_asset;
    float                      m_rotationX = 0.0f;
    float                      m_rotationY = 0.0f;
    int                        m_selectedTexture = 0;
    float                      m_animationTime = 0.0f;
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
    const float aspect = (float)width / (float)height;
    const float fov = XMConvertToRadians(75.0f);
    const float near = 0.01f;
    const float far = 1000.0f;
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
        = NS::String::string("quit ", UTF8StringEncoding)->stringByAppendingString(appName);
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
              auto*          appInstance = NS::Application::sharedApplication();
              auto           window = (NS::Window*)appInstance->windows()->object(0);
              NS::OpenPanel* panel = NS::OpenPanel::openPanel();
              panel->beginSheetModal(window, ^(NS::ModalResponseType response) {
                  auto urls = panel->urls();
                  auto numUrls = urls->count();
                  for (NS::UInteger i = 0; i < numUrls; ++i)
                  {
                      auto url = (NS::URL*)urls->object(i);
                      auto path = url->fileSystemRepresentation();

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
    m_asset = std::make_unique<GLTFAsset>(std::filesystem::path(filePath));
}

bool Skinning::onLoad()
{
    int32_t width;
    int32_t height;
    SDL_GetWindowSizeInPixels(m_window, &width, &height);
    const float aspect = (float)width / (float)height;
    const float fov = XMConvertToRadians(75.0f);
    const float near = 0.01f;
    const float far = 1000.0f;

    m_mainCamera = std::make_unique<Camera>(
        Vector3::Zero, Vector3::Forward, Vector3::Up, fov, aspect, near, far);

    createBuffers();

    createPipelineState();

    createArgumentBuffers();

    try
    {
        m_asset = std::make_unique<GLTFAsset>(std::string("alien-bug.glb"));
        fmt::println("");
    }
    catch (std::exception& e)
    {
    }

    return true;
}

void Skinning::onSetupUi(const GameTimer& timer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0);
    ImGui::SetNextWindowPos(ImVec2(10, 20));
    ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Metal Example", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("%s (%.1d fps)", SDL_GetWindowTitle(m_window), timer.framesPerSecond());
    std::vector<std::string> animations = m_asset->animations();
    if (ImGui::Combo(
            "Animation", &m_selectedTexture,
            [](void* data, int index) -> const char* {
                auto names = reinterpret_cast<std::vector<std::string>*>(data);
                return names->at(index).c_str();
            },
            &animations, (int)animations.size()))
    {
        //		/// Update argument buffer index
        //		for (const auto& buffer: ArgumentBuffer) {
        //			auto* contents =
        // reinterpret_cast<FragmentArgumentBuffer*>(buffer->contents());
        // contents->TextureIndex = SelectedTexture;
        //		}
        m_animationTime = 0.0f;
    }

    const auto animation = m_asset->getAnimation(m_selectedTexture);
    if (animation != nullptr)
    {
        if (ImGui::SliderFloat(
                "##", &m_animationTime, 0.0f, animation->channels[0].sampler->input->max[0]))
        {
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
    // RotationX += elapsed;
    if (m_mouse->isLeftPressed())
    {
        m_rotationY += static_cast<float>(m_mouse->relativeX()) * elapsed;
    }

    if (m_gamepad)
    {
        m_rotationY += static_cast<float>(m_gamepad->leftThumbstickHorizontal()) * elapsed;
    }
}

void Skinning::onRender(MTL::RenderCommandEncoder* commandEncoder, const GameTimer& timer)
{
    updateUniforms();

    // TODO: Render skinned mesh with specified animation
}

void Skinning::createPipelineState()
{
    // TODO: Create pipeline state suitable for GLTF mesh
}

void Skinning::createBuffers()
{
    // TODO: Create necessary instance buffers
}

void Skinning::updateUniforms()
{
    // TODO: Update any uniform data
}

void Skinning::createArgumentBuffers()
{
    // TODO: Create any necessary argument buffers
}

int main(int argc, char** argv)
{
    int result = EXIT_FAILURE;
    try
    {
        auto example = std::make_unique<Skinning>();
        result = example->run(argc, argv);
    }
    catch (const std::runtime_error& error)
    {
        fmt::println("Exiting...");
    }

    return result;
}
