cmake_minimum_required(VERSION 3.24)
project(Metal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
enable_language(OBJC OBJCXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchExternal)

include(CompileShaders)

file(GLOB_RECURSE METAL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.metal)
CompileMetalShaders("${METAL_SHADERS}")

# Examples
add_subdirectory(examples)

#add_subdirectory(examples/base)
#add_subdirectory(examples/triangle)
#add_subdirectory(examples/texture)
#add_subdirectory(examples/instancing)
#add_subdirectory(examples/spritefont)
#add_subdirectory(examples/gltf)
