cmake_minimum_required(VERSION 3.24)
project(Metal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
enable_language(OBJC OBJCXX)

# DXMathKit
add_subdirectory(thirdparty/DXMathKit)

# Examples
add_subdirectory(source/triangle)
