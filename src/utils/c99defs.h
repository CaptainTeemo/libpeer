//
//  c99defs.h
//  CXXPlayground
//
//  Created by Keven on 2/5/24.
//  
//
	

#ifndef c99defs_h
#define c99defs_h

#pragma once

/*
 * Contains hacks for getting some C99 stuff working in VC, things like
 * bool, stdint
 */

#define UNUSED_PARAMETER(param) (void)param

#ifdef _MSC_VER
#define _OBS_DEPRECATED __declspec(deprecated)
#define OBS_NORETURN __declspec(noreturn)
#define FORCE_INLINE __forceinline
#else
#define _OBS_DEPRECATED __attribute__((deprecated))
#define OBS_NORETURN __attribute__((noreturn))
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

#if defined(SWIG_TYPE_TABLE)
#define OBS_DEPRECATED
#else
#define OBS_DEPRECATED _OBS_DEPRECATED
#endif

#if defined(IS_LIBOBS)
#define OBS_EXTERNAL_DEPRECATED
#else
#define OBS_EXTERNAL_DEPRECATED OBS_DEPRECATED
#endif

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

#ifdef _MSC_VER
#define PRAGMA_WARN_PUSH __pragma(warning(push))
#define PRAGMA_WARN_POP __pragma(warning(pop))
#define PRAGMA_WARN_DEPRECATION __pragma(warning(disable : 4996))
#elif defined(__clang__)
#define PRAGMA_WARN_PUSH _Pragma("clang diagnostic push")
#define PRAGMA_WARN_POP _Pragma("clang diagnostic pop")
#define PRAGMA_WARN_DEPRECATION \
_Pragma("clang diagnostic warning \"-Wdeprecated-declarations\"")
#elif defined(__GNUC__)
#define PRAGMA_WARN_PUSH _Pragma("GCC diagnostic push")
#define PRAGMA_WARN_POP _Pragma("GCC diagnostic pop")
#define PRAGMA_WARN_DEPRECATION \
_Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
#else
#define PRAGMA_WARN_PUSH
#define PRAGMA_WARN_POP
#define PRAGMA_WARN_DEPRECATION
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>


#endif /* c99defs_h */