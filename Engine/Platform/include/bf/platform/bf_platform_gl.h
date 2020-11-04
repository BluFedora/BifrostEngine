#ifndef BF_PLATFORM_GL_H
#define BF_PLATFORM_GL_H

#include "bf_platform.h"

#if BIFROST_PLATFORM_ANDROID
#include <GLES3/gl3.h>
typedef void* (* GLADloadproc)(const char *name);
#elif BIFROST_PLATFORM_IOS
#include <OpenGLES/ES3/gl.h>
typedef void* (* GLADloadproc)(const char *name);
#elif BIFROST_PLATFORM_MACOS
#include <OpenGL/gl3.h>
#include <glad/glad.h>
#elif BIFROST_PLATFORM_EMSCRIPTEN
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif BIFROST_PLATFORM_WINDOWS
#include <glad/glad.h>
#endif

#if __cplusplus
extern "C" {
#endif

BIFROST_PLATFORM_API void         bfWindow_makeGLContextCurrent(bfWindow* self);
BIFROST_PLATFORM_API GLADloadproc bfPlatformGetProcAddress(void);
BIFROST_PLATFORM_API void         bfWindowGL_swapBuffers(bfWindow* self);

#if __cplusplus
}
#endif

#endif /* BF_PLATFORM_GL_H */