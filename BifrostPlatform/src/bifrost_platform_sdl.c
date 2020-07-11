#include "bifrost/platform/bifrost_platform.h"

#include "bifrost/platform/bifrost_platform_event.h"
#include "bifrost/platform/bifrost_platform_vulkan.h"
#include "bifrost/platform/bifrost_platform_gl.h"

#include <sdl/SDL.h>        /* SDL_* */
#include <sdl/SDL_vulkan.h> /* SDL_Vulkan_CreateSurface */

#include <assert.h> /* assert */

#if BIFROST_PLATFORM_EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>  // EmscriptenWebGLContextAttributes etc
#endif

typedef SDL_Window* NativeWindowHandle;

#define EMSCRIPTEN_CANVAS_NAME "#canvas"

extern bfPlatformInitParams g_BifrostPlatform;

#if BIFROST_PLATFORM_EMSCRIPTEN
/*static*/EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_CanvasContext = 0;
#endif

#ifndef bfTrue
#define bfTrue 1
#define bfFalse 0
#endif

// TODO(SR):
//   - SDL_GL_CreateContext
//   - SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
//   - SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
//   - SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
//   - SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//   - SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

int bfPlatformInit(bfPlatformInitParams params)
{
  const int was_success = SDL_Init(SDL_INIT_VIDEO) == 0;

  if (was_success)
  {
    g_BifrostPlatform = params;

    if (!g_BifrostPlatform.allocator)
    {
      g_BifrostPlatform.allocator = &bfPlatformDefaultAllocator;
    }
  }

  return was_success;
}

#include <stdio.h>

static void dispatchEvent(BifrostWindow* window, bfEvent event)
{
  if (window->event_fn)
  {
    window->event_fn(window, &event);
  }
}

void bfPlatformPumpEvents(void)
{
  SDL_Event evt;

  while (SDL_PollEvent(&evt))
  {
    switch (evt.type)
    {
      case SDL_KEYDOWN:
      {
        printf("KEY DOWN EVENT (%i)\n", evt.key.keysym.sym);
        break;
      }
    }

    if (evt.type == SDL_QUIT)
    {
      //quit = true;
    }
  }
}

BifrostWindow* bfPlatformCreateWindow(const char* title, int width, int height, uint32_t flags)
{
  BifrostWindow* const window = bfPlatformAlloc(sizeof(BifrostWindow));

  if (window)
  {
    (void)flags;

#if BIFROST_PLATFORM_EMSCRIPTEN
    assert(g_BifrostPlatform.gfx_api == BIFROST_PLATFORM_GFX_OPENGL && "OpenGL (WebGL) is the only thing supported on the Web.");
    assert(g_CanvasContext == 0 && "Only one window is allowed on web platforms.");

    EmscriptenWebGLContextAttributes ctx_attribs;
    emscripten_webgl_init_context_attributes(&ctx_attribs);
    ctx_attribs.alpha                        = 0;
    ctx_attribs.depth                        = bfTrue;
    ctx_attribs.stencil                      = bfTrue;
    ctx_attribs.antialias                    = bfTrue;
    ctx_attribs.premultipliedAlpha           = bfTrue;
    ctx_attribs.preserveDrawingBuffer        = bfFalse;
    ctx_attribs.powerPreference              = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;  // EM_WEBGL_POWER_PREFERENCE_LOW_POWER or EM_WEBGL_POWER_PREFERENCE_DEFAULT
    ctx_attribs.failIfMajorPerformanceCaveat = bfFalse;
    ctx_attribs.majorVersion                 = 2;
    ctx_attribs.minorVersion                 = 0;
    ctx_attribs.enableExtensionsByDefault    = bfTrue;
    ctx_attribs.explicitSwapControl          = bfFalse;
    ctx_attribs.renderViaOffscreenBackBuffer = bfFalse;
    ctx_attribs.proxyContextToMainThread     = bfFalse;

    g_CanvasContext = emscripten_webgl_create_context(EMSCRIPTEN_CANVAS_NAME, &ctx_attribs);

    if (!g_CanvasContext)
    {
      bfPlatformFree(window, sizeof(BifrostWindow));
      return NULL;
    }

    emscripten_webgl_make_context_current(g_CanvasContext);

    // SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#window");
#else
#endif

    window->handle        = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    window->event_fn      = NULL;
    window->frame_fn      = NULL;
    window->user_data     = NULL;
    window->renderer_data = NULL;
    window->gl_context    = NULL;

   // SDL_SetWindowData(window->handle, "GL_CONTEXT", )

    if (!window->handle)
    {
      bfPlatformFree(window, sizeof(BifrostWindow));
      return NULL;
    }
  }

  return window;
}

int bfWindow_wantsToClose(BifrostWindow* self)
{
  return self == NULL;
}

void bfWindow_getPos(BifrostWindow* self, int* x, int* y);
void bfWindow_setPos(BifrostWindow* self, int x, int y);

void bfWindow_getSize(BifrostWindow* self, int* x, int* y)
{
  SDL_GetWindowSize((NativeWindowHandle)self->handle, x, y);
}

void bfWindow_setSize(BifrostWindow* self, int x, int y);

void bfPlatformDestroyWindow(BifrostWindow* window)
{
  SDL_DestroyWindow((NativeWindowHandle)window->handle);
  bfPlatformFree(window, sizeof(BifrostWindow));
}

void bfPlatformQuit(void)
{
  SDL_Quit();
}

// Platform Extensions

int bfWindow_createVulkanSurface(BifrostWindow* self, VkInstance instance, VkSurfaceKHR* out)
{
  return SDL_Vulkan_CreateSurface(self->handle, instance, out) == SDL_TRUE;
}

void bfWindow_makeGLContextCurrent(BifrostWindow* self)
{
  if (self->gl_context)
  {
    SDL_GL_MakeCurrent(self->handle, self->gl_context);
  }
}

GLADloadproc bfPlatformGetProcAddress(void)
{
  return (GLADloadproc)SDL_GL_GetProcAddress;
}

void bfWindowGL_swapBuffers(BifrostWindow* self)
{
  SDL_GL_SwapWindow(self->handle);
}