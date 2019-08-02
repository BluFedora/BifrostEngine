#ifndef BIFROST_PLATFORM_GL_H
#define BIFROST_PLATFORM_GL_H

#include "bifrost_platform.h"

#if PLATFORM_ANDROID
#include <GLES3/gl3.h>
#elif PLATFORM_IOS
#include <OpenGLES/ES3/gl.h>
#elif PLATFORM_MACOS
#include <OpenGL/gl3.h>
#elif PLATFORM_EMSCRIPTEN
#include <GLES2/gl2.h>
#elif PLATFORM_WINDOWS
/* TODO: Other Platforms */
#endif

#endif /* BIFROST_PLATFORM_GL_H */
