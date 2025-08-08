cmake_minimum_required(VERSION 3.24)
project(Metal)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
enable_language(OBJC OBJCXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchExternal)

find_package(fmt CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(directxmath CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)

find_path(CGLTF_INCLUDE_DIRS "cgltf.h")
include_directories(${CGLTF_INCLUDE_DIRS})

# Add custom shader compilation for generators other than Xcode
# For Xcode, each target will include the Metal shaders as source
# to be compiled into the default library through Xcode
if (NOT CMAKE_GENERATOR MATCHES "Xcode")
    include(CompileShaders)

    file(GLOB_RECURSE METAL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.metal)
    CompileMetalShaders("${METAL_SHADERS}")
endif ()

# Examples
add_subdirectory(source)

