#ifndef BIFORST_PLATFORM_H
#define BIFORST_PLATFORM_H

// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html

/* clang-format off */
#ifndef PLATFORM_WIN32
#define BIFROST_PLATFORM_WIN32         0
#define BIFROST_PLATFORM_WIN64         0
#define BIFROST_PLATFORM_WINDOWS       0
#define BIFROST_PLATFORM_ANDROID       0
#define BIFROST_PLATFORM_MACOS         0
#define BIFROST_PLATFORM_IOS           0
#define BIFROST_PLATFORM_EMSCRIPTEN    0
#define BIFROST_PLATFORM_LINUX         0
#endif

#ifdef _WIN32

  #undef BIFROST_PLATFORM_WINDOWS
  #define BIFROST_PLATFORM_WINDOWS 1

  #undef BIFROST_PLATFORM_WIN32
  #define BIFROST_PLATFORM_WIN32 1

  #ifdef _WIN64
    #undef BIFROST_PLATFORM_WIN64
    #define BIFROST_PLATFORM_WIN64 1
  #endif

#elif __APPLE__
  #include "TargetConditionals.h"
  #if TARGET_IPHONE_SIMULATOR
    #undef BIFROST_PLATFORM_IOS
    #define BIFROST_PLATFORM_IOS 1
  #elif TARGET_OS_IPHONE
    #undef BIFROST_PLATFORM_IOS
    #define BIFROST_PLATFORM_IOS 1
  #elif TARGET_OS_MAC
    #undef BIFROST_PLATFORM_MACOS
    #define BIFROST_PLATFORM_MACOS 1
	#define BIFROST_PLATFORM_MACOS 1
  #else
    #   error "Unknown Apple platform"
  #endif
#elif __ANDROID__
  #undef BIFROST_PLATFORM_ANDROID
  #define BIFROST_PLATFORM_ANDROID 1
#elif __linux

  #undef BIFROST_PLATFORM_LINUX
  #define BIFROST_PLATFORM_LINUX 1

  // linux
#elif __unix // all unices not caught above
             // Unix
#elif __posix
// POSIX
#endif

#if __EMSCRIPTEN__
  #undef BIFROST_PLATFORM_EMSCRIPTEN
  #define BIFROST_PLATFORM_EMSCRIPTEN 1
#endif

#if (BIFROST_PLATFORM_IOS || BIFROST_PLATFORM_ANDROID)
#define BIFROST_OPENGL_ES 1
#define BIFROST_OPENGL    0
#else
#define BIFROST_OPENGL_ES 0
#define BIFROST_OPENGL    1
#endif

#endif /* BIFORST_PLATFORM_H */
