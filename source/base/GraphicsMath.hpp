////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

#ifdef __APPLE__
#ifdef __arm__
#define _XM_ARM_NEON_INTRINSICS_ // Apple architecture
#endif
#endif

#define PAL_STDCPP_COMPAT // Needed to avoid issues with __null definition on Xcode 16+
#include <DirectXColors.h>
#include <DirectXMath.h>

#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// Needed due to DirectXMath/SAL headers re-defining NULL
// to avoid strict header ordering
#undef NULL
#define NULL 0