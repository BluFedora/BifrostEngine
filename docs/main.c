#if __EMSCRIPTEN__
#include <bifrost/platform/bifrost_platform_gl.h>
// TODO(SR): The API should define what header to include.
// On desktop we can use GLAD for Win, Mac, Nix.
#include "bifrost/render/bifrost_video_api.h"

#include <emscripten.h>        // EM_ASM
#include <emscripten/html5.h>  //

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE s_GlCtx;

void updateFromWebGL()
{
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

extern void basicDrawing(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE);

#endif

#if __EMSCRIPTEN__
  EmscriptenWebGLContextAttributes ctx_attribs;
  emscripten_webgl_init_context_attributes(&ctx_attribs);
  ctx_attribs.alpha                        = bfFalse;
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
  // ctx_attribs.proxyContextToMainThread = bfFalse; // default

  s_GlCtx = emscripten_webgl_create_context("#kanvas", &ctx_attribs);

  if (!s_GlCtx)
  {
    printf("Failed to create context\n");
    return -5;
  }

  emscripten_webgl_make_context_current(s_GlCtx);

  basicDrawing(s_GlCtx);

  argc    = 2;
  argv[1] = "test_script.bts";

  // const int positionAttributeLocation = glGetAttribLocation(shader);

  //EM_ASM(alert("Inline Javascript from C++!"););
#endif
