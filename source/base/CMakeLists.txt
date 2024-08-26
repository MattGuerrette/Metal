
add_library(base STATIC
        LinkImpl.cpp
        Camera.hpp
        Camera.cpp
        Keyboard.hpp
        Keyboard.cpp
        Mouse.hpp
        Mouse.cpp
        GameTimer.cpp
        GameTimer.cpp
        SimpleMath.cpp
        Example.hpp
        Example.cpp
        File.hpp
        File.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
)

set_target_properties(base PROPERTIES
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 14.0
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO)

target_compile_definitions(base PRIVATE -DIMGUI_IMPL_METAL_CPP)
target_include_directories(base PUBLIC .)
target_link_libraries(base PUBLIC ktx fmt SDL3::SDL3-static
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore")
