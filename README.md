# Metal C++ examples

A collection of open source C++ examples for [Metal](https://developer.apple.com/metal)

## Building

These example applications use [CMake](https://www.cmake.org) and [Xcode](https://developer.apple.com/xcode/).

```
   git clone https://github.com/MattGuerrette/Metal.git
   cd Metal
   mkdir cmake-build-xcode
   cd cmake-build-xcode
   cmake .. -GXcode -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=<Apple Developer ID>
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

Showcases usage of [MTLHeap](https://developer.apple.com/documentation/metal/mtlheap) and [Argument Buffers](https://developer.apple.com/documentation/metal/buffers/improving_cpu_performance_by_using_argument_buffers) to bindlessly render multiple textures from GPU memory.
