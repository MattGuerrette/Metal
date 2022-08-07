//
//  graphics_math.h
//  base
//
//  Created by Matthew Guerrette on 7/25/22.
//
#pragma once

#include <cstdint>

#ifdef __APPLE__
#ifdef __arm__
#define _XM_ARM_NEON_INTRINSICS_ // Apple architecture
#endif
#endif

#include <DirectXMath.h>
