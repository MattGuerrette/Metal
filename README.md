![example workflow](https://github.com/MattGuerrette/Metal/actions/workflows/cmake-single-platform.yml/badge.svg)

# Metal C++ examples

A collection of open source C++ examples for [Metal](https://developer.apple.com/metal)

### metal-cpp

These examples use a modified version of Metal-CPP originally published by Apple. This modified version is maintained
here:
[metal-cpp](https://github.com/MattGuerrette/metalcpp) and includes some additional API bindings not yet supported by
Apple, however
it is actively updated with the latest fixes and additions from Apple as well.

## Requirements

These samples are developed using **C++20** and target the following platforms and os versions:

| Platform | OS Version | |
|----------|------------|-- |
| macOS    | 14.0+      | |
| iOS      | 17.0+      | |
| tvOS     | 17.0+      |Needs vcpkg support [#12](https://github.com/MattGuerrette/Metal/issues/12) |

## Building

These example applications use [Vcpkg](https://vcpkg.io/en/), [CMake](https://www.cmake.org) and are actively developed using both [CLion](https://www.jetbrains.com/clion/) and [Xcode](https://developer.apple.com/xcode/)

For those wanting to build the projects from a terminal invoking CMake directly or to
generate the Xcode project files here are some example commands:

### Ninja

```
   git clone https://github.com/MattGuerrette/Metal.git
   cd Metal
   mkdir cmake-build-debug
   cd cmake-build-debug
   cmake .. -GNinja -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
   cmake --build .
```

### Xcode

```
   git clone https://github.com/MattGuerrette/Metal.git
   cd Metal
   mkdir cmake-build-xcode
   cd cmake-build-xcode
   cmake .. -GXcode -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=<Apple Developer ID>
   cmake --build .
```

For codesigning purposes, you will want to specify the development team ID as shown above to avoid needing
to manually set the development team for each target project.

## Table of Contents

+ [Examples](#Examples)

## Examples

### Basics

#### [01 - HelloWorld](source/helloworld/)

Basic example for rendering a colored triangle using Metal

#### [02 - Instancing](source/instancing/)

Rendering of multiple cube geometry instances

#### [03 - Textures](source/textures/)

Showcases loading of [KTX](https://www.khronos.org/ktx/) compressed textures in ASTC format into
a [MTLHeap](https://developer.apple.com/documentation/metal/mtlheap) and
using [Argument Buffers](https://developer.apple.com/documentation/metal/buffers/improving_cpu_performance_by_using_argument_buffers)
to bindlessly render multiple textures from GPU memory.
