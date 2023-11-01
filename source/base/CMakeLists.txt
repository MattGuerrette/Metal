
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
        Example.cpp)

set_target_properties(base PROPERTIES
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO)

target_include_directories(base PUBLIC .)
target_link_libraries(base PUBLIC SDL2::SDL2-static SDL2_ttf::SDL2_ttf-static
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore")
