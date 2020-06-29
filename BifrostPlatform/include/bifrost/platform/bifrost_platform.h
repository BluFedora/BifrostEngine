#ifndef BIFORST_PLATFORM_H
#define BIFORST_PLATFORM_H

#include "bifrost_platform_export.h"

#include <stdint.h> /* size_t, uint32_t */

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


#if __cplusplus
extern "C"
{
#endif
  struct bfEvent_t;
  typedef struct bfEvent_t bfEvent;

  typedef void* (*bfPlatformAllocator)(void* ptr, size_t old_size, size_t new_size, void* user_data);

  typedef enum bfPlatformGfxAPI_t
  {
    BIFROST_PLATFORM_GFX_VUlKAN,
    BIFROST_PLATFORM_GFX_OPENGL,

  } bfPlatformGfxAPI;

  typedef struct bfPlatformInitParams_t
  {
    int                 argc;
    char**              argv;
    bfPlatformAllocator allocator;
    bfPlatformGfxAPI    gfx_api;
    void*               user_data;

  } bfPlatformInitParams;

  struct BifrostWindow_t;
  typedef void (*bfWindowEventFn)(struct BifrostWindow_t* window, bfEvent* event);
  typedef void (*bfWindowFrameFn)(struct BifrostWindow_t* window);

  #define BIFROST_WINDOW_FLAG_IS_RESIZABLE (1 << 0)
  #define BIFROST_WINDOW_FLAG_IS_VISIBLE (1 << 1)
  #define BIFROST_WINDOW_FLAG_IS_DECORATED (1 << 2)
  #define BIFROST_WINDOW_FLAG_IS_MAXIMIZED (1 << 3)
  #define BIFROST_WINDOW_FLAG_IS_FLOATING (1 << 4)
  #define BIFROST_WINDOW_FLAG_IS_FOCUSED (1 << 5)
  #define BIFROST_WINDOW_FLAG_IS_FOCUSED_ON_SHOW (1 << 6)
  #define BIFROST_WINDOW_FLAGS_DEFAULT (BIFROST_WINDOW_FLAG_IS_VISIBLE | BIFROST_WINDOW_FLAG_IS_RESIZABLE | BIFROST_WINDOW_FLAG_IS_MAXIMIZED | BIFROST_WINDOW_FLAG_IS_FOCUSED | BIFROST_WINDOW_FLAG_IS_DECORATED)

  typedef struct BifrostWindow_t
  {
    void*           handle;
    void*           user_data;
    void*           renderer_data;
    bfWindowEventFn event_fn;
    bfWindowFrameFn frame_fn;

  } BifrostWindow;

  /*!
   * @brief
   *   Initializes the underlying platform abstraction layer.
   *   Should be called before any other subsystem.
   *
   * @param params
   *   Some configuration parameters for startup.
   *
   * @return
   *       0 (false) - If there were was an error initializing.
   *   non 0 (true)  - Successfully initialized. 
   */
  BIFROST_PLATFORM_API int            bfPlatformInit(bfPlatformInitParams params);
  BIFROST_PLATFORM_API void           bfPlatformPumpEvents(void);
  BIFROST_PLATFORM_API BifrostWindow* bfPlatformCreateWindow(const char* title, int width, int height, uint32_t flags);
  BIFROST_PLATFORM_API int            bfWindow_wantsToClose(BifrostWindow* self);
  BIFROST_PLATFORM_API void           bfWindow_show(BifrostWindow* self);
  BIFROST_PLATFORM_API void           bfWindow_getPos(BifrostWindow* self, int* x, int* y);
  BIFROST_PLATFORM_API void           bfWindow_setPos(BifrostWindow* self, int x, int y);
  BIFROST_PLATFORM_API void           bfWindow_getSize(BifrostWindow* self, int* x, int* y);
  BIFROST_PLATFORM_API void           bfWindow_setSize(BifrostWindow* self, int x, int y);
  BIFROST_PLATFORM_API void           bfWindow_focus(BifrostWindow* self);
  BIFROST_PLATFORM_API int            bfWindow_isFocused(BifrostWindow* self);
  BIFROST_PLATFORM_API int            bfWindow_isMinimized(BifrostWindow* self);
  BIFROST_PLATFORM_API int            bfWindow_isHovered(BifrostWindow* self);
  BIFROST_PLATFORM_API void           bfWindow_setTitle(BifrostWindow* self, const char* title);
  BIFROST_PLATFORM_API void           bfWindow_setAlpha(BifrostWindow* self, float value);
  BIFROST_PLATFORM_API void           bfPlatformDestroyWindow(BifrostWindow* window);
  BIFROST_PLATFORM_API void           bfPlatformQuit(void);
  BIFROST_PLATFORM_API void*          bfPlatformDefaultAllocator(void* ptr, size_t old_size, size_t new_size, void* user_data);
  BIFROST_PLATFORM_API void*          bfPlatformAlloc(size_t size);
  BIFROST_PLATFORM_API void*          bfPlatformRealloc(void* ptr, size_t old_size, size_t new_size);
  BIFROST_PLATFORM_API void           bfPlatformFree(void* ptr, size_t old_size);
  BIFROST_PLATFORM_API void           bfPlatformDoMainLoop(BifrostWindow* main_window);
#if __cplusplus
}
#endif

#endif /* BIFORST_PLATFORM_H */
