cmake_minimum_required(VERSION 3.10)
project(Metal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

include(FetchContent)

# DirectXMath
# Normally used for DirectX graphics programming, it also
# supports Arm Neon intrinsics which allows its vector
# optimized linear algebra structures to be used directly
# with Metal.
# Added benefit of this is being able to use a single
# math library across both Microsoft and Apple platforms.
FetchContent_Declare(
    DirectXMath
    GIT_REPOSITORY https://github.com/Microsoft/DirectXMath
    GIT_TAG main
)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG 2.0.22
)

FetchContent_MakeAvailable(SDL DirectXMath)

# Global include
include_directories(${CMAKE_SOURCE_DIR}/examples/base)

# Examples
add_subdirectory(examples/base)
add_subdirectory(examples/triangle)
add_subdirectory(examples/coretext)
