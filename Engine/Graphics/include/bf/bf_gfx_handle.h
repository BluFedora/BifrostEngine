#ifndef BF_GFX_HANDLE_H
#define BF_GFX_HANDLE_H

#if __cplusplus
extern "C" {
#endif
#define BF_DECLARE_HANDLE(T) \
  struct bf##T##_t;               \
  typedef struct bf##T##_t bf##T; \
  typedef bf##T*           bf##T##Handle

BF_DECLARE_HANDLE(GfxContext);
BF_DECLARE_HANDLE(GfxDevice);
BF_DECLARE_HANDLE(GfxCommandList);
BF_DECLARE_HANDLE(Buffer);
BF_DECLARE_HANDLE(VertexLayoutSet);  // TODO(SR): Maybe this should be a POD rather than a handle?
BF_DECLARE_HANDLE(DescriptorSet);    // TODO(SR): Maybe this should be a POD rather than a handle?
BF_DECLARE_HANDLE(Renderpass);
BF_DECLARE_HANDLE(ShaderModule);
BF_DECLARE_HANDLE(ShaderProgram);
BF_DECLARE_HANDLE(Texture);
BF_DECLARE_HANDLE(Framebuffer);
BF_DECLARE_HANDLE(Pipeline);
BF_DECLARE_HANDLE(WindowSurface);
typedef void* bfGfxBaseHandle;

#undef BF_DECLARE_HANDLE

/* clang-format off */

#define BF_DEFINE_GFX_HANDLE(T) struct bf##T##_t
#define BF_NULL_GFX_HANDLE      (void*)0

/* clang-format on */

#if __cplusplus
}
#endif

#endif /* BF_GFX_HANDLE_H */
