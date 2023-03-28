cmake_minimum_required(VERSION 3.24)
project(Metal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
enable_language(OBJC OBJCXX)

# Global include
include_directories(
    ${CMAKE_SOURCE_DIR}/thirdparty/DirectXMath/Inc
    ${CMAKE_SOURCE_DIR}/source/base)

# Examples
add_subdirectory(source/base)
add_subdirectory(source/triangle)
