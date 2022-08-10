cmake_minimum_required(VERSION 3.10)
project(Metal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
enable_language(OBJC OBJCXX)

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

FetchContent_MakeAvailable(DirectXMath)

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS" OR ${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    FetchContent_Declare(
            SDL
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG 2.0.22
    )

    FetchContent_MakeAvailable(SDL)
else ()
    find_package(SDL2 REQUIRED)
endif ()


# Global include
include_directories(${CMAKE_SOURCE_DIR}/examples/base)

# Examples
add_subdirectory(examples/base)
add_subdirectory(examples/triangle)
add_subdirectory(examples/texture)
add_subdirectory(examples/instancing)
add_subdirectory(examples/gltf)
