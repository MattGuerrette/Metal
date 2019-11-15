cmake_minimum_required(VERSION 3.10)
project(Metal)

set(CMAKE_CXX_STANDARD 17)

# Global include
include_directories(${CMAKE_SOURCE_DIR}/examples/base)

# Examples
add_subdirectory(examples/base)
add_subdirectory(examples/triangle)
add_subdirectory(examples/coretext)
