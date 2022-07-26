//
//  graphics_math.h
//  base
//
//  Created by Matthew Guerrette on 7/25/22.
//
#pragma once

typedef struct RECT {
    unsigned long left;
    unsigned long right;
    unsigned long top;
    unsigned long bottom;
} RECT;
typedef uint32_t UINT;

#ifdef __arm__
#define _XM_ARM_NEON_INTRINSICS_ // Apple architecture
#endif
#include "SimpleMath.h"
