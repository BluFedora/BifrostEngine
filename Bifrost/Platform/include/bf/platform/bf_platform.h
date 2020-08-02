/******************************************************************************/
/*!
 * @file   bf_platform.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Handles the windowing and event layer for the engine.
 *   References:
 *     [https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html]
 *
 * @version 0.0.1
 * @date    2020-07-05
 *
 * @copyright Copyright (c) 2020 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef BF_PLATFORM_H
#define BF_PLATFORM_H

#include "bf_platform_export.h"

#include <stddef.h> /* size_t   */
#include <stdint.h> /* uint32_t */

/* clang-format off */
#ifndef BIFROST_PLATFORM_WIN32
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
/* clang-format on */

#if __cplusplus
extern "C" {
#endif
struct bfEvent_t;
typedef struct bfEvent_t bfEvent;

typedef enum
{
  BIFROST_PLATFORM_GFX_VUlKAN,
  BIFROST_PLATFORM_GFX_OPENGL,

} bfPlatformGfxAPI;

typedef void* (*bfPlatformAllocator)(void* ptr, size_t old_size, size_t new_size, void* user_data);

typedef struct bfPlatformInitParams_t
{
  int                 argc;      /*!< Argc from the main function, could be 0.                                                                                    */
  char**              argv;      /*!< Argv from the main function, allowed to be NULL                                                                             */
  bfPlatformAllocator allocator; /*!< Custom allocator if you wanted to control where the platform gets it's memory from, if NULL will use the default allocator. */
  void*               user_data; /*!< User data for keeping tack some some global state, could be NULL.                                                           */

} bfPlatformInitParams;

struct bfWindow_t;
typedef void (*bfWindowEventFn)(struct bfWindow_t* window, bfEvent* event);
typedef void (*bfWindowFrameFn)(struct bfWindow_t* window);

#define BIFROST_WINDOW_FLAG_IS_RESIZABLE (1 << 0)
#define BIFROST_WINDOW_FLAG_IS_VISIBLE (1 << 1)
#define BIFROST_WINDOW_FLAG_IS_DECORATED (1 << 2)
#define BIFROST_WINDOW_FLAG_IS_MAXIMIZED (1 << 3)
#define BIFROST_WINDOW_FLAG_IS_FLOATING (1 << 4)
#define BIFROST_WINDOW_FLAG_IS_FOCUSED (1 << 5)
#define BIFROST_WINDOW_FLAG_IS_FOCUSED_ON_SHOW (1 << 6)
#define BIFROST_WINDOW_FLAGS_DEFAULT (BIFROST_WINDOW_FLAG_IS_VISIBLE | BIFROST_WINDOW_FLAG_IS_RESIZABLE | BIFROST_WINDOW_FLAG_IS_MAXIMIZED | BIFROST_WINDOW_FLAG_IS_FOCUSED | BIFROST_WINDOW_FLAG_IS_DECORATED)

typedef struct bfWindow_t
{
  void*           handle;
  void*           user_data;
  void*           renderer_data;
  bfWindowEventFn event_fn;
  bfWindowFrameFn frame_fn;

} bfWindow; /*!< Base class for the window, each backedn can extend it in various ways. */

/*!
 * @brief
 *   Initializes the underlying platform abstraction layer.
 *   Should be called before any other subsystem.
 *
 * @param params
 *   Some configuration parameters for startup.
 *
 * @return
 *   0 (false) - If there were was an error initializing.
 *   1 (true)  - Successfully initialized.
 */
BIFROST_PLATFORM_API int  bfPlatformInit(bfPlatformInitParams params);
BIFROST_PLATFORM_API void bfPlatformPumpEvents(void);
BIFROST_PLATFORM_API bfWindow*        bfPlatformCreateWindow(const char* title, int width, int height, uint32_t flags);
BIFROST_PLATFORM_API int              bfWindow_wantsToClose(bfWindow* self);
BIFROST_PLATFORM_API void             bfWindow_show(bfWindow* self);
BIFROST_PLATFORM_API void             bfWindow_getPos(bfWindow* self, int* x, int* y);
BIFROST_PLATFORM_API void             bfWindow_setPos(bfWindow* self, int x, int y);
BIFROST_PLATFORM_API void             bfWindow_getSize(bfWindow* self, int* x, int* y);
BIFROST_PLATFORM_API void             bfWindow_setSize(bfWindow* self, int x, int y);
BIFROST_PLATFORM_API void             bfWindow_focus(bfWindow* self);
BIFROST_PLATFORM_API int              bfWindow_isFocused(bfWindow* self);
BIFROST_PLATFORM_API int              bfWindow_isMinimized(bfWindow* self);
BIFROST_PLATFORM_API int              bfWindow_isHovered(bfWindow* self);
BIFROST_PLATFORM_API void             bfWindow_setTitle(bfWindow* self, const char* title);
BIFROST_PLATFORM_API void             bfWindow_setAlpha(bfWindow* self, float value);
BIFROST_PLATFORM_API void             bfPlatformDestroyWindow(bfWindow* window);
BIFROST_PLATFORM_API void             bfPlatformQuit(void);
BIFROST_PLATFORM_API bfPlatformGfxAPI bfPlatformGetGfxAPI(void);
BIFROST_PLATFORM_API void*            bfPlatformDefaultAllocator(void* ptr, size_t old_size, size_t new_size, void* user_data);
BIFROST_PLATFORM_API void*            bfPlatformAlloc(size_t size);
BIFROST_PLATFORM_API void*            bfPlatformRealloc(void* ptr, size_t old_size, size_t new_size);
BIFROST_PLATFORM_API void             bfPlatformFree(void* ptr, size_t old_size);
BIFROST_PLATFORM_API void             bfPlatformDoMainLoop(bfWindow* main_window);
#if __cplusplus
}
#endif

#endif /* BF_PLATFORM_H */
