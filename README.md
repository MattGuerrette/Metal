# Metal Objective-C/C++ examples and demos

A collection of open source Objective-C++ examples for [Metal](https://developer.apple.com/metal)

## Building

These example applications use [CMake](https://www.cmake.org) and [Xcode](https://developer.apple.com/xcode/). 

**Xcode is a requirement. Other CMake generators will not work**

```
   git clone --recursive https://github.com/MattGuerrette/Metal.git
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

#### [01 - Triangle](examples/triangle/)

Basic example for rendering a colored triangle using Metal

#### [02 - Texture](examples/texture/)

Simple textured quad using MTKTextureLoader to load a sample png

#### [03 - Instancing](examples/instancing/)

Rendering of multiple cube geometry instances
