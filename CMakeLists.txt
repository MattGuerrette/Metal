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
FetchContent_MakeAvailable(DirectXMath)

# DirectXTK12
# Normally used exclusively for DirectX graphics programming,
# this library provides some useful math utilities in the
# SimpleMath.h header file
# These utilities do however rely on Windows platform specific
# structures that must be redefined on Apple platforms.
# See the graphics_math.h header for details.
FetchContent_Declare(
    DirectXTK12
    GIT_REPOSITORY https://github.com/Microsoft/DirectXTK12
    GIT_TAG main
)
FetchContent_Populate(DirectXTK12)


# Global include
include_directories(${CMAKE_SOURCE_DIR}/examples/base)

# Examples
add_subdirectory(examples/base)
add_subdirectory(examples/triangle)
add_subdirectory(examples/coretext)
