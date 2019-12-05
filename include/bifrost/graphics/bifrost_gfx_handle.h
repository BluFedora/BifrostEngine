#ifndef BIFROST_GFX_HANDLE_H
#define BIFROST_GFX_HANDLE_H

#if __cplusplus
extern "C" {
#endif
#define BIFROST_DECLARE_HANDLE(T) \
  struct bf##T##_t;               \
  typedef struct bf##T##_t bf##T; \
  typedef bf##T*           bf##T##Handle

BIFROST_DECLARE_HANDLE(GfxContext);
BIFROST_DECLARE_HANDLE(GfxDevice);
BIFROST_DECLARE_HANDLE(GfxCommandList);
BIFROST_DECLARE_HANDLE(Buffer);
BIFROST_DECLARE_HANDLE(VertexLayoutSet);  //!< Maybe this should be a POD rather than a handle?
BIFROST_DECLARE_HANDLE(DescriptorSet);    //!< Maybe this should be a POD rather than a handle?
BIFROST_DECLARE_HANDLE(Renderpass);
BIFROST_DECLARE_HANDLE(ShaderModule);
BIFROST_DECLARE_HANDLE(ShaderProgram);
BIFROST_DECLARE_HANDLE(Texture);
BIFROST_DECLARE_HANDLE(Framebuffer);
BIFROST_DECLARE_HANDLE(Pipeline);

  typedef void* bfGfxBaseHandle;
#undef BIFROST_DECLARE_HANDLE

#define BIFROST_DEFINE_HANDLE(T) struct bf##T##_t
#define BIFROST_NULL_HANDLE (void*)0
#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_HANDLE_H */
