#include "bifrost/graphics/bifrost_gfx_api.h"

// TODO(Shareef): Replace with custom logging solution.
#include <cstdio> /* printf */
// TODO(Shareef): Replace with custom memory solution.
#include <cstdlib> /* malloc, free */

#include <glad/glad.h>
#include <vector>

// Type Definitions

BIFROST_DEFINE_HANDLE(GfxContext)
{
  const char*       app_name;
  uint32_t          app_version;
  uint32_t          width;
  uint32_t          height;
  bfGfxDeviceHandle logical_device;

  // Debug Checking
  bool frame_did_start;
};

BIFROST_DEFINE_HANDLE(GfxDevice)
{
  std::vector<bfBufferHandle> buffers = {};
};

BIFROST_DEFINE_HANDLE(Buffer)
{
  GLuint               handle     = 0;
  bfBufferCreateParams params     = {};
  void*                mapped_ptr = nullptr;
};

BIFROST_DEFINE_HANDLE(Framebuffer){

};

// OpenGL Functions Used:
/*
 * glFlush
 * glGenBuffers
 * glBufferData
 * glMapBufferRange
 * glBindBuffer
 * glCopyBufferSubData
 * glUnmapBuffer
 */

namespace
{
  template<typename T>
  T* allocate()
  {
    return new (std::malloc(sizeof(T))) T();
  }

  template<typename T>
  void deallocate(T* obj)
  {
    obj->~T();
    std::free(obj);
  }

  GLenum bufferTarget(bfBufferHandle buffer);
  GLenum bufferAllocHint(bfBufferHandle buffer, int mode);
  void   bufferBind(bfBufferHandle buffer);
}  // namespace

/* Context */

bfGfxContextHandle bfGfxContext_new(const bfGfxContextCreateParams* params)
{
  const auto self = allocate<bfGfxContext>();

  self->app_name       = params->app_name;
  self->app_version    = params->app_version;
  self->width          = 0u;
  self->height         = 0u;
  self->logical_device = allocate<bfGfxDevice>();

  std::printf("Initializing OpenGL Application (%s v%i)\n", self->app_name, static_cast<int>(self->app_version));

  return self;
}

bfGfxDeviceHandle bfGfxContext_device(bfGfxContextHandle self)
{
  return self->logical_device;
}

void bfGfxContext_onResize(bfGfxContextHandle self, uint32_t width, uint32_t height)
{
  self->width  = width;
  self->height = height;
}

bfBool32 bfGfxContext_beginFrame(bfGfxContextHandle self)
{
  (void)self;
  return bfTrue;
}

void bfGfxContext_endFrame(bfGfxContextHandle self)
{
  (void)self;
}

void bfGfxContext_delete(bfGfxContextHandle self)
{
  printf("Destroying OpenGL Application (%s v%i)\n", self->app_name, static_cast<int>(self->app_version));
  deallocate(self);
}

/* Logical Device */

void bfGfxDevice_flush(bfGfxDeviceHandle self)
{
  glFinish();
}

bfBufferHandle bfGfxDevice_newBuffer(bfGfxDeviceHandle self, const bfBufferCreateParams* params)
{
  const auto buffer = allocate<bfBuffer>();

  glGenBuffers(1, &buffer->handle);
  buffer->params = *params;
  bufferBind(buffer);
  glBufferData(bufferTarget(buffer), GLsizeiptr(params->size), nullptr, bufferAllocHint(buffer, 0));

  self->buffers.push_back(buffer);
  return buffer;
}

void bfGfxDevice_deleteBuffer(bfGfxDeviceHandle self, bfBufferHandle buffer)
{
  self->buffers.erase(std::find(self->buffers.cbegin(), self->buffers.cend(), buffer));
  deallocate(buffer);
}

/* Buffer */
bfBufferSize bfBuffer_size(bfBufferHandle self)
{
  return self->params.size;
}

void* bfBuffer_map(bfBufferHandle self, bfBufferSize offset, bfBufferSize size)
{
  bufferBind(self);
  self->mapped_ptr = glMapBufferRange(bufferTarget(self), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
  return self->mapped_ptr;
}

void bfBuffer_copyCPU(bfBufferHandle self, bfBufferSize dst_offset, const void* data, bfBufferSize num_bytes)
{
  if (!self->mapped_ptr)
  {
    // C-API cannot throw.
    // throw "Attempt to write to a un mapped buffer";
  }

  memcpy(static_cast<char*>(self->mapped_ptr) + dst_offset, data, std::size_t(num_bytes));
}

void bfBuffer_copyGPU(bfBufferHandle src, bfBufferSize src_offset, bfBufferHandle dst, bfBufferSize dst_offset, bfBufferSize num_bytes)
{
  if (!dst->handle || !src->handle)
  {
    // C-API cannot throw.
    // throw "bfBuffer_copyGPU error";
  }

  glBindBuffer(GL_COPY_WRITE_BUFFER, dst->handle);
  glBindBuffer(GL_COPY_READ_BUFFER, src->handle);
  glCopyBufferSubData(GL_COPY_READ_BUFFER,
                      GL_COPY_WRITE_BUFFER,
                      GLintptr(src_offset),
                      GLintptr(dst_offset),
                      GLsizeiptr(num_bytes));
}

void bfBuffer_unMap(bfBufferHandle self)
{
  if (!self->mapped_ptr)
  {
    // C-API cannot throw.
    // throw "Attempt to un map a buffer that was not mapped.";
  }

  glUnmapBuffer(bufferTarget(self));
  self->mapped_ptr = nullptr;
}

// Helpers

namespace
{
  GLenum bufferTarget(bfBufferHandle buffer)
  {
    const auto usage = buffer->params.usage;

    GLenum target;

    if (usage & BIFROST_BUF_VERTEX_BUFFER)
    {
      target = GL_ARRAY_BUFFER;
    }
    else if (usage & BIFROST_BUF_INDEX_BUFFER)
    {
      target = GL_ELEMENT_ARRAY_BUFFER;
    }

    throw "The OpenGL graphics backend does not support this type of buffer usage";
  }

  GLenum bufferAllocHint(bfBufferHandle buffer, int mode)
  {
    const auto properties = buffer->params.properties;
    const bool is_static  = properties & BIFROST_BPF_DEVICE_LOCAL;

    switch (mode & 0x3)
    {
      case 0:  // Draw
      {
        if (is_static)
        {
          return GL_STATIC_DRAW;
        }

        return GL_STREAM_DRAW;
      }
      case 1:  // Read
      {
        if (is_static)
        {
          return GL_STATIC_READ;
        }

        return GL_STREAM_READ;
      }
      case 2:  // Write
      {
        if (is_static)
        {
          return GL_STATIC_COPY;
        }

        return GL_STREAM_COPY;
      }
      case 3:
      {
        [[fallthrough]];
      }
      default:
      {
        return GL_STREAM_DRAW;
      }
    }
  }

  void bufferBind(bfBufferHandle buffer)
  {
    glBindBuffer(bufferTarget(buffer), buffer->handle);
  }

}  // namespace
