
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
)

set_target_properties(base PROPERTIES
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 14.0
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO)

target_compile_definitions(base PRIVATE -DIMGUI_IMPL_METAL_CPP)
target_include_directories(base PUBLIC .)
target_compile_options(base PUBLIC -fno-exceptions -Wall -Werror -Wextra)
target_link_libraries(base PUBLIC fmt::fmt SDL3::SDL3 Microsoft::DirectXMath AppleFrameworksCpp
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore")
