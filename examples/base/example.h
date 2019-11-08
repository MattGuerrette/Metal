/*
*    Metal Examples
*    Copyright(c) 2019 Matt Guerrette
*
*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "platform.h"
#include <string>

#ifdef SYSTEM_MACOS
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

#import <QuartzCore/QuartzCore.h>

class Example {
public:
    Example(std::string title, uint32_t width, uint32_t height);

    virtual ~Example();

    virtual void init()   = 0;
    virtual void update() = 0;
    virtual void render() = 0;
    int          run(int argc, char* argv[]);

    CAMetalLayer* metalLayer();

    
    CAMetalLayer* metalLayer_;
private:
    std::string title_;
    uint32_t    width_;
    uint32_t    height_;

#ifdef SYSTEM_MACOS
    id<NSApplicationDelegate> delegate_;
#else
    id<UIApplicationDelegate> delegate_;
#endif

};
