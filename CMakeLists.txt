cmake_minimum_required(VERSION 3.24)
project(Metal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
enable_language(OBJC OBJCXX)

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    list (APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/Frameworks/iOS")
    set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ThirdParty/Frameworks/iOS")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    list (APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/Frameworks/tvOS")
    set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ThirdParty/Frameworks/tvOS")
else()
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/Frameworks/macOS")
    set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ThirdParty/Frameworks/macOS")
endif()

# Global include
include_directories(
    ${CMAKE_SOURCE_DIR}/thirdparty/DirectXMath/Inc
    ${CMAKE_SOURCE_DIR}/thirdparty/metalcpp
    ${CMAKE_SOURCE_DIR}/examples/base)

# Examples
add_subdirectory(examples/base)
add_subdirectory(examples/triangle)
#add_subdirectory(examples/texture)
#add_subdirectory(examples/instancing)
#add_subdirectory(examples/spritefont)
#add_subdirectory(examples/gltf)
