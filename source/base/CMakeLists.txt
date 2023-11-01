
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
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
)

set_target_properties(base PROPERTIES
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO)

target_compile_definitions(base PRIVATE -DIMGUI_IMPL_METAL_CPP)
target_include_directories(base PUBLIC .)
target_link_libraries(base PUBLIC SDL2::SDL2-static SDL2_ttf::SDL2_ttf-static
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore")
