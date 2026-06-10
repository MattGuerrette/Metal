
add_library(base STATIC
        Camera.cpp
        Keyboard.cpp
        Keyboard.hpp
        Mouse.cpp
        GameTimer.cpp
        GameTimer.cpp
        SimpleMath.cpp
        Example.cpp
        Example.hpp
        Gamepad.cpp
        File.cpp
        File.hpp
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
)

set_target_properties(base PROPERTIES
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO)

target_compile_definitions(base PRIVATE -DIMGUI_IMPL_METAL_CPP)
target_include_directories(base PUBLIC .)
target_link_libraries(base PUBLIC fmt::fmt SDL3::SDL3 Microsoft::DirectXMath metal-cpp::metal-cpp
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore")
