#ifndef BIFORST_PLATFORM_H
#define BIFORST_PLATFORM_H

#define PLATFORM_WIN32      0
#define PLATFORM_WIN64      0
#define PLATFORM_WINDOWS    0
#define PLATFORM_ANDROID    0
#define PLATFORM_MACOS      0
#define PLATFORM_IOS        0
#define PLATFORM_EMSCRIPTEN 0

#ifdef _WIN32

  #undef  PLATFORM_WINDOWS
  #define PLATFORM_WINDOWS 1
  
  #undef  PLATFORM_WIN32
  #define PLATFORM_WIN32 1

  #ifdef _WIN64
    #undef  PLATFORM_WIN64
    #define PLATFORM_WIN64 1
  #endif

#elif __APPLE__
  #include "TargetConditionals.h"
  #if TARGET_IPHONE_SIMULATOR
    #undef PLATFORM_IOS
    #define PLATFORM_IOS 1
  #elif TARGET_OS_IPHONE
    #undef PLATFORM_IOS
    #define PLATFORM_IOS 1
  #elif TARGET_OS_MAC
    #undef PLATFORM_MACOS
    #define PLATFORM_MACOS 1
	#define PLATFORM_MAC 1
  #else
    #   error "Unknown Apple platform"
  #endif
#elif __ANDROID__
  #undef PLATFORM_ANDROID
  #define PLATFORM_ANDROID 1
#elif __linux
  // linux
#elif __unix // all unices not caught above
             // Unix
#elif __posix
// POSIX
#endif

#if __EMSCRIPTEN__
  #undef PLATFORM_EMSCRIPTEN
  #define PLATFORM_EMSCRIPTEN 1
#endif

#if (PLATFORM_IOS || PLATFORM_ANDROID)
#define OPENGL_ES         1
#define OPENGL            0
#else
#define OPENGL_ES         0
#define OPENGL            1
#endif

#endif /* BIFORST_PLATFORM_H */
