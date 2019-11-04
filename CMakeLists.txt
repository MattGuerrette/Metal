cmake_minimum_required(VERSION 3.10)
project(Metal)

# Global include
include_directories(${CMAKE_SOURCE_DIR}/examples/base)

# Examples
add_subdirectory(examples/base)
add_subdirectory(examples/triangle)