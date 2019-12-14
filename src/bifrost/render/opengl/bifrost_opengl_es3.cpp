#include "bifrost/render/bifrost_video_api.h"
#include <assert.h>
#include <stdlib.h>  // malloc, free
// Shader API Impl.
#include "bifrost/platform/bifrost_platform_gl.h"

// bifrost_opengl_shader.h
#include "bifrost/render/bifrost_shader_api.h"

#include <array>
#include <string>
#include <unordered_map>

struct ShaderModule_t
{
  GLuint handle;
};

struct ShaderProgram_t
{
  const LogicalDevice *                   parent   = {};
  std::unordered_map<std::string, GLuint> uniforms = {};
  std::array<ShaderModule, BST_MAX>       shaders  = {};
  GLuint                                  handle   = {};
};

static GLenum bfShaderTypeToGL(BifrostShaderType type);

BifrostShaderProgramHandle BifrostShaderProgram_new(const ShaderProgramCreateParams *params)
{
  BifrostShaderProgramHandle self = new ShaderProgram();
  self->parent                    = params->parent;

  for (size_t i = 0; i < BST_MAX; ++i)
  {
    self->shaders[i].handle = 0u;
  }

  return self;
}

void BifrostShaderProgram_loadData(BifrostShaderProgramHandle self, BifrostShaderType type, const char *code, size_t code_size)
{
  GLuint shader = self->shaders[type].handle = glCreateShader(bfShaderTypeToGL(type));

  const char *const source[] =
   {
    "#version 300 es\n",
    "precision mediump float;\n",
    code,
   };

  const GLint counts[] =
   {
    17,
    26,
    (GLint)code_size,
   };

  glShaderSource(shader, 3, source, counts);
  glCompileShader(shader);

  int  success;
  char infoLog[1024];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
    printf("SHADER_COMPILATION of type: [%i][%s]", type, infoLog);
    assert(0);
  }
}

void BifrostShaderprogram_compile(bfShaderProgram self)
{
  GLuint program = self->handle = glCreateProgram();

  glAttachShader(program, self->shaders[BST_VERTEX].handle);
  glAttachShader(program, self->shaders[BST_FRAGMENT].handle);

  glLinkProgram(program);

  // var success = gl.getProgramParameter(program, gl.LINK_STATUS);
  // console.log(gl.getProgramInfoLog(program));
}

void BifrostShaderProgram_delete(bfShaderProgram self)
{
  for (size_t i = 0; i < BST_MAX; ++i)
  {
    glDeleteShader(self->shaders[i].handle);
  }

  delete self;
}

// bifrost_opengl_conversions.h

static GLenum bfShaderTypeToGL(BifrostShaderType type)
{
  switch (type)
  {
    case BST_VERTEX:
      return GL_VERTEX_SHADER;
    case BST_FRAGMENT:
      return GL_FRAGMENT_SHADER;
    case BST_MAX:
      break;
  }

  assert(!"Invalid BifrostShaderType");
  return 0;
}
