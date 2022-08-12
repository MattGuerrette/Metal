/*
 *    Metal Examples
 *    Copyright(c) 2019 Matt Guerrette
 *
 *    This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

// NOTE:
//
//	Define platform macros, used for precompiler blocks when
//	writing platform specific routines
//

#if defined(__APPLE__)
#include <TargetConditionals.h>

// macOS
#if TARGET_OS_OSX
#define SYSTEM_MACOS
#endif

// iOS devices
#if TARGET_OS_IOS
#define SYSTEM_IOS
#endif

// tvOS devices
#ifdef TARGET_OS_TV
#define SYSTEM_TVOS
#endif

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/std_types.h>

#else
#error System is not supported
#endif

#include <cstdint>
#include <cstdlib>


