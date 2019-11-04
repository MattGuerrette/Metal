/*
*    Metal Examples
*    Copyright(c) 2019 Matt Guerrette
*
*    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "platform.h"
#include <string>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

class Example
{
public:
    Example(std::string title, uint32_t width, uint32_t height);

    virtual ~Example();

    virtual void update() = 0;
    virtual void render() = 0;
    
    int run();
    
protected:
    CAMetalLayer* metal_layer_;

private:
    std::string title_;
    uint32_t width_;
    uint32_t height_;
    id<NSApplicationDelegate> delegate_;
    
};
