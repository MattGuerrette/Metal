////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

#ifdef __APPLE__
#ifdef __arm__
#define _XM_ARM_NEON_INTRINSICS_ // Apple architecture
#endif
#endif

#include <DirectXMath.h>
#include <DirectXColors.h>

#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

