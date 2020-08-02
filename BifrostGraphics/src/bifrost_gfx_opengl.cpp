#include "bifrost/graphics/bifrost_gfx_api.h"

#include "bifrost/graphics/bifrost_gfx_object_cache.hpp"

#include <bifrost/memory/bifrost_freelist_allocator.hpp>
#include <bifrost/memory/bifrost_stl_allocator.hpp>

#include <bifrost/platform/bifrost_platform_gl.h>

#include <stb/stb_image.h>

#include <cassert>       /* assert              */
#include <cstdint>       /* uint32_t            */
#include <memory>        /* unique_ptr<T>       */
#include <unordered_map> /* unordered_map<K, V> */
#include <vector>        /* vector<T>           */

#define USE_OPENGL_ES_STANDARD (BIFROST_PLATFORM_EMSCRIPTEN || BIFROST_PLATFORM_IOS || BIFROST_PLATFORM_ANDROID)
#define USE_WEBGL_STANDARD BIFROST_PLATFORM_EMSCRIPTEN

#if USE_OPENGL_ES_STANDARD
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glGenVertexArrays glGenVertexArraysOES
#define glIsVertexArray glIsVertexArrayOES
//#define glMapBufferRange glMapBufferRangeEXT
//#define glUnmapBuffer glUnmapBufferOES
//#define glCopyBufferSubData glCopyBufferSubDataNV
#define GL_COPY_READ_BUFFER GL_COPY_READ_BUFFER_NV
#define GL_COPY_WRITE_BUFFER GL_COPY_WRITE_BUFFER_NV
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
#define GL_DEPTH_STENCIL GL_DEPTH_STENCIL_OES
#define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT_24_8_OES
const GLenum GL_DEPTH_STENCIL_ATTACHMENT = 0x821A;
#define glDrawBuffers glDrawBuffersEXT

// Taken From Spec [https://www.khronos.org/registry/webgl/specs/latest/2.0/]
const GLenum GL_READ_BUFFER                                   = 0x0C02;
const GLenum GL_UNPACK_ROW_LENGTH                             = 0x0CF2;
const GLenum GL_UNPACK_SKIP_ROWS                              = 0x0CF3;
const GLenum GL_UNPACK_SKIP_PIXELS                            = 0x0CF4;
const GLenum GL_PACK_ROW_LENGTH                               = 0x0D02;
const GLenum GL_PACK_SKIP_ROWS                                = 0x0D03;
const GLenum GL_PACK_SKIP_PIXELS                              = 0x0D04;
const GLenum GL_COLOR                                         = 0x1800;
const GLenum GL_DEPTH                                         = 0x1801;
const GLenum GL_STENCIL                                       = 0x1802;
const GLenum GL_RED                                           = 0x1903;
const GLenum GL_RGB8                                          = 0x8051;
const GLenum GL_RGBA8                                         = 0x8058;
const GLenum GL_RGB10_A2                                      = 0x8059;
const GLenum GL_TEXTURE_BINDING_3D                            = 0x806A;
const GLenum GL_UNPACK_SKIP_IMAGES                            = 0x806D;
const GLenum GL_UNPACK_IMAGE_HEIGHT                           = 0x806E;
const GLenum GL_TEXTURE_WRAP_R                                = 0x8072;
const GLenum GL_MAX_3D_TEXTURE_SIZE                           = 0x8073;
const GLenum GL_UNSIGNED_INT_2_10_10_10_REV                   = 0x8368;
const GLenum GL_MAX_ELEMENTS_VERTICES                         = 0x80E8;
const GLenum GL_MAX_ELEMENTS_INDICES                          = 0x80E9;
const GLenum GL_TEXTURE_MIN_LOD                               = 0x813A;
const GLenum GL_TEXTURE_MAX_LOD                               = 0x813B;
const GLenum GL_TEXTURE_BASE_LEVEL                            = 0x813C;
const GLenum GL_TEXTURE_MAX_LEVEL                             = 0x813D;
const GLenum GL_MAX_TEXTURE_LOD_BIAS                          = 0x84FD;
const GLenum GL_TEXTURE_COMPARE_MODE                          = 0x884C;
const GLenum GL_TEXTURE_COMPARE_FUNC                          = 0x884D;
const GLenum GL_CURRENT_QUERY                                 = 0x8865;
const GLenum GL_QUERY_RESULT                                  = 0x8866;
const GLenum GL_QUERY_RESULT_AVAILABLE                        = 0x8867;
const GLenum GL_STREAM_READ                                   = 0x88E1;
const GLenum GL_STREAM_COPY                                   = 0x88E2;
const GLenum GL_STATIC_READ                                   = 0x88E5;
const GLenum GL_STATIC_COPY                                   = 0x88E6;
const GLenum GL_DYNAMIC_READ                                  = 0x88E9;
const GLenum GL_DYNAMIC_COPY                                  = 0x88EA;
const GLenum GL_MAX_DRAW_BUFFERS                              = 0x8824;
const GLenum GL_DRAW_BUFFER0                                  = 0x8825;
const GLenum GL_DRAW_BUFFER1                                  = 0x8826;
const GLenum GL_DRAW_BUFFER2                                  = 0x8827;
const GLenum GL_DRAW_BUFFER3                                  = 0x8828;
const GLenum GL_DRAW_BUFFER4                                  = 0x8829;
const GLenum GL_DRAW_BUFFER5                                  = 0x882A;
const GLenum GL_DRAW_BUFFER6                                  = 0x882B;
const GLenum GL_DRAW_BUFFER7                                  = 0x882C;
const GLenum GL_DRAW_BUFFER8                                  = 0x882D;
const GLenum GL_DRAW_BUFFER9                                  = 0x882E;
const GLenum GL_DRAW_BUFFER10                                 = 0x882F;
const GLenum GL_DRAW_BUFFER11                                 = 0x8830;
const GLenum GL_DRAW_BUFFER12                                 = 0x8831;
const GLenum GL_DRAW_BUFFER13                                 = 0x8832;
const GLenum GL_DRAW_BUFFER14                                 = 0x8833;
const GLenum GL_DRAW_BUFFER15                                 = 0x8834;
const GLenum GL_MAX_FRAGMENT_UNIFORM_COMPONENTS               = 0x8B49;
const GLenum GL_MAX_VERTEX_UNIFORM_COMPONENTS                 = 0x8B4A;
const GLenum GL_SAMPLER_3D                                    = 0x8B5F;
const GLenum GL_SAMPLER_2D_SHADOW                             = 0x8B62;
const GLenum GL_FRAGMENT_SHADER_DERIVATIVE_HINT               = 0x8B8B;
const GLenum GL_PIXEL_PACK_BUFFER                             = 0x88EB;
const GLenum GL_PIXEL_UNPACK_BUFFER                           = 0x88EC;
const GLenum GL_PIXEL_PACK_BUFFER_BINDING                     = 0x88ED;
const GLenum GL_PIXEL_UNPACK_BUFFER_BINDING                   = 0x88EF;
const GLenum GL_FLOAT_MAT2x3                                  = 0x8B65;
const GLenum GL_FLOAT_MAT2x4                                  = 0x8B66;
const GLenum GL_FLOAT_MAT3x2                                  = 0x8B67;
const GLenum GL_FLOAT_MAT3x4                                  = 0x8B68;
const GLenum GL_FLOAT_MAT4x2                                  = 0x8B69;
const GLenum GL_FLOAT_MAT4x3                                  = 0x8B6A;
const GLenum GL_SRGB                                          = 0x8C40;
const GLenum GL_SRGB8                                         = 0x8C41;
const GLenum GL_SRGB8_ALPHA8                                  = 0x8C43;
const GLenum GL_COMPARE_REF_TO_TEXTURE                        = 0x884E;
const GLenum GL_RGBA32F                                       = 0x8814;
const GLenum GL_RGB32F                                        = 0x8815;
const GLenum GL_RGBA16F                                       = 0x881A;
const GLenum GL_RGB16F                                        = 0x881B;
const GLenum GL_VERTEX_ATTRIB_ARRAY_INTEGER                   = 0x88FD;
const GLenum GL_MAX_ARRAY_TEXTURE_LAYERS                      = 0x88FF;
const GLenum GL_MIN_PROGRAM_TEXEL_OFFSET                      = 0x8904;
const GLenum GL_MAX_PROGRAM_TEXEL_OFFSET                      = 0x8905;
const GLenum GL_MAX_VARYING_COMPONENTS                        = 0x8B4B;
const GLenum GL_TEXTURE_BINDING_2D_ARRAY                      = 0x8C1D;
const GLenum GL_R11F_G11F_B10F                                = 0x8C3A;
const GLenum GL_UNSIGNED_INT_10F_11F_11F_REV                  = 0x8C3B;
const GLenum GL_RGB9_E5                                       = 0x8C3D;
const GLenum GL_UNSIGNED_INT_5_9_9_9_REV                      = 0x8C3E;
const GLenum GL_TRANSFORM_FEEDBACK_BUFFER_MODE                = 0x8C7F;
const GLenum GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS    = 0x8C80;
const GLenum GL_TRANSFORM_FEEDBACK_VARYINGS                   = 0x8C83;
const GLenum GL_TRANSFORM_FEEDBACK_BUFFER_START               = 0x8C84;
const GLenum GL_TRANSFORM_FEEDBACK_BUFFER_SIZE                = 0x8C85;
const GLenum GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN         = 0x8C88;
const GLenum GL_RASTERIZER_DISCARD                            = 0x8C89;
const GLenum GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS = 0x8C8A;
const GLenum GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS       = 0x8C8B;
const GLenum GL_INTERLEAVED_ATTRIBS                           = 0x8C8C;
const GLenum GL_SEPARATE_ATTRIBS                              = 0x8C8D;
const GLenum GL_TRANSFORM_FEEDBACK_BUFFER                     = 0x8C8E;
const GLenum GL_TRANSFORM_FEEDBACK_BUFFER_BINDING             = 0x8C8F;
const GLenum GL_RGBA32UI                                      = 0x8D70;
const GLenum GL_RGB32UI                                       = 0x8D71;
const GLenum GL_RGBA16UI                                      = 0x8D76;
const GLenum GL_RGB16UI                                       = 0x8D77;
const GLenum GL_RGBA8UI                                       = 0x8D7C;
const GLenum GL_RGB8UI                                        = 0x8D7D;
const GLenum GL_RGBA32I                                       = 0x8D82;
const GLenum GL_RGB32I                                        = 0x8D83;
const GLenum GL_RGBA16I                                       = 0x8D88;
const GLenum GL_RGB16I                                        = 0x8D89;
const GLenum GL_RGBA8I                                        = 0x8D8E;
const GLenum GL_RGB8I                                         = 0x8D8F;
const GLenum GL_RED_INTEGER                                   = 0x8D94;
const GLenum GL_RGB_INTEGER                                   = 0x8D98;
const GLenum GL_RGBA_INTEGER                                  = 0x8D99;
const GLenum GL_SAMPLER_2D_ARRAY                              = 0x8DC1;
const GLenum GL_SAMPLER_2D_ARRAY_SHADOW                       = 0x8DC4;
const GLenum GL_SAMPLER_CUBE_SHADOW                           = 0x8DC5;
const GLenum GL_UNSIGNED_INT_VEC2                             = 0x8DC6;
const GLenum GL_UNSIGNED_INT_VEC3                             = 0x8DC7;
const GLenum GL_UNSIGNED_INT_VEC4                             = 0x8DC8;
const GLenum GL_INT_SAMPLER_2D                                = 0x8DCA;
const GLenum GL_INT_SAMPLER_3D                                = 0x8DCB;
const GLenum GL_INT_SAMPLER_CUBE                              = 0x8DCC;
const GLenum GL_INT_SAMPLER_2D_ARRAY                          = 0x8DCF;
const GLenum GL_UNSIGNED_INT_SAMPLER_2D                       = 0x8DD2;
const GLenum GL_UNSIGNED_INT_SAMPLER_3D                       = 0x8DD3;
const GLenum GL_UNSIGNED_INT_SAMPLER_CUBE                     = 0x8DD4;
const GLenum GL_UNSIGNED_INT_SAMPLER_2D_ARRAY                 = 0x8DD7;
const GLenum GL_DEPTH_COMPONENT32F                            = 0x8CAC;
const GLenum GL_DEPTH32F_STENCIL8                             = 0x8CAD;
const GLenum GL_FLOAT_32_UNSIGNED_INT_24_8_REV                = 0x8DAD;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING         = 0x8210;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE         = 0x8211;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE               = 0x8212;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE             = 0x8213;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE              = 0x8214;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE             = 0x8215;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE             = 0x8216;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE           = 0x8217;
const GLenum GL_FRAMEBUFFER_DEFAULT                           = 0x8218;
const GLenum GL_UNSIGNED_NORMALIZED                           = 0x8C17;
const GLenum GL_DRAW_FRAMEBUFFER_BINDING                      = 0x8CA6; /* Same as FRAMEBUFFER_BINDING */
const GLenum GL_READ_FRAMEBUFFER                              = 0x8CA8;
const GLenum GL_DRAW_FRAMEBUFFER                              = 0x8CA9;
const GLenum GL_READ_FRAMEBUFFER_BINDING                      = 0x8CAA;
const GLenum GL_RENDERBUFFER_SAMPLES                          = 0x8CAB;
const GLenum GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER          = 0x8CD4;
const GLenum GL_MAX_COLOR_ATTACHMENTS                         = 0x8CDF;
const GLenum GL_COLOR_ATTACHMENT1                             = 0x8CE1;
const GLenum GL_COLOR_ATTACHMENT2                             = 0x8CE2;
const GLenum GL_COLOR_ATTACHMENT3                             = 0x8CE3;
const GLenum GL_COLOR_ATTACHMENT4                             = 0x8CE4;
const GLenum GL_COLOR_ATTACHMENT5                             = 0x8CE5;
const GLenum GL_COLOR_ATTACHMENT6                             = 0x8CE6;
const GLenum GL_COLOR_ATTACHMENT7                             = 0x8CE7;
const GLenum GL_COLOR_ATTACHMENT8                             = 0x8CE8;
const GLenum GL_COLOR_ATTACHMENT9                             = 0x8CE9;
const GLenum GL_COLOR_ATTACHMENT10                            = 0x8CEA;
const GLenum GL_COLOR_ATTACHMENT11                            = 0x8CEB;
const GLenum GL_COLOR_ATTACHMENT12                            = 0x8CEC;
const GLenum GL_COLOR_ATTACHMENT13                            = 0x8CED;
const GLenum GL_COLOR_ATTACHMENT14                            = 0x8CEE;
const GLenum GL_COLOR_ATTACHMENT15                            = 0x8CEF;
const GLenum GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE            = 0x8D56;
const GLenum GL_MAX_SAMPLES                                   = 0x8D57;
const GLenum GL_HALF_FLOAT                                    = 0x140B;
const GLenum GL_RG                                            = 0x8227;
const GLenum GL_RG_INTEGER                                    = 0x8228;
const GLenum GL_R8                                            = 0x8229;
const GLenum GL_RG8                                           = 0x822B;
const GLenum GL_R16F                                          = 0x822D;
const GLenum GL_R32F                                          = 0x822E;
const GLenum GL_RG16F                                         = 0x822F;
const GLenum GL_RG32F                                         = 0x8230;
const GLenum GL_R8I                                           = 0x8231;
const GLenum GL_R8UI                                          = 0x8232;
const GLenum GL_R16I                                          = 0x8233;
const GLenum GL_R16UI                                         = 0x8234;
const GLenum GL_R32I                                          = 0x8235;
const GLenum GL_R32UI                                         = 0x8236;
const GLenum GL_RG8I                                          = 0x8237;
const GLenum GL_RG8UI                                         = 0x8238;
const GLenum GL_RG16I                                         = 0x8239;
const GLenum GL_RG16UI                                        = 0x823A;
const GLenum GL_RG32I                                         = 0x823B;
const GLenum GL_RG32UI                                        = 0x823C;
const GLenum GL_VERTEX_ARRAY_BINDING                          = 0x85B5;
const GLenum GL_RGB8_SNORM                                    = 0x8F96;
const GLenum GL_SIGNED_NORMALIZED                             = 0x8F9C;
const GLenum GL_COPY_READ_BUFFER_BINDING                      = 0x8F36; /* Same as COPY_READ_BUFFER */
const GLenum GL_COPY_WRITE_BUFFER_BINDING                     = 0x8F37; /* Same as COPY_WRITE_BUFFER */
const GLenum GL_UNIFORM_BUFFER                                = 0x8A11;
const GLenum GL_UNIFORM_BUFFER_BINDING                        = 0x8A28;
const GLenum GL_UNIFORM_BUFFER_START                          = 0x8A29;
const GLenum GL_UNIFORM_BUFFER_SIZE                           = 0x8A2A;
const GLenum GL_MAX_VERTEX_UNIFORM_BLOCKS                     = 0x8A2B;
const GLenum GL_MAX_FRAGMENT_UNIFORM_BLOCKS                   = 0x8A2D;
const GLenum GL_MAX_COMBINED_UNIFORM_BLOCKS                   = 0x8A2E;
const GLenum GL_MAX_UNIFORM_BUFFER_BINDINGS                   = 0x8A2F;
const GLenum GL_MAX_UNIFORM_BLOCK_SIZE                        = 0x8A30;
const GLenum GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS        = 0x8A31;
const GLenum GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS      = 0x8A33;
const GLenum GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT               = 0x8A34;
const GLenum GL_ACTIVE_UNIFORM_BLOCKS                         = 0x8A36;
const GLenum GL_UNIFORM_TYPE                                  = 0x8A37;
const GLenum GL_UNIFORM_SIZE                                  = 0x8A38;
const GLenum GL_UNIFORM_BLOCK_INDEX                           = 0x8A3A;
const GLenum GL_UNIFORM_OFFSET                                = 0x8A3B;
const GLenum GL_UNIFORM_ARRAY_STRIDE                          = 0x8A3C;
const GLenum GL_UNIFORM_MATRIX_STRIDE                         = 0x8A3D;
const GLenum GL_UNIFORM_IS_ROW_MAJOR                          = 0x8A3E;
const GLenum GL_UNIFORM_BLOCK_BINDING                         = 0x8A3F;
const GLenum GL_UNIFORM_BLOCK_DATA_SIZE                       = 0x8A40;
const GLenum GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS                 = 0x8A42;
const GLenum GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES          = 0x8A43;
const GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER     = 0x8A44;
const GLenum GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER   = 0x8A46;
const GLenum GL_INVALID_INDEX                                 = 0xFFFFFFFF;
const GLenum GL_MAX_VERTEX_OUTPUT_COMPONENTS                  = 0x9122;
const GLenum GL_MAX_FRAGMENT_INPUT_COMPONENTS                 = 0x9125;
const GLenum GL_MAX_SERVER_WAIT_TIMEOUT                       = 0x9111;
const GLenum GL_OBJECT_TYPE                                   = 0x9112;
const GLenum GL_SYNC_CONDITION                                = 0x9113;
const GLenum GL_SYNC_STATUS                                   = 0x9114;
const GLenum GL_SYNC_FLAGS                                    = 0x9115;
const GLenum GL_SYNC_FENCE                                    = 0x9116;
const GLenum GL_SYNC_GPU_COMMANDS_COMPLETE                    = 0x9117;
const GLenum GL_UNSIGNALED                                    = 0x9118;
const GLenum GL_SIGNALED                                      = 0x9119;
const GLenum GL_ALREADY_SIGNALED                              = 0x911A;
const GLenum GL_TIMEOUT_EXPIRED                               = 0x911B;
const GLenum GL_CONDITION_SATISFIED                           = 0x911C;
const GLenum GL_WAIT_FAILED                                   = 0x911D;
const GLenum GL_SYNC_FLUSH_COMMANDS_BIT                       = 0x00000001;
const GLenum GL_VERTEX_ATTRIB_ARRAY_DIVISOR                   = 0x88FE;
const GLenum GL_ANY_SAMPLES_PASSED                            = 0x8C2F;
const GLenum GL_ANY_SAMPLES_PASSED_CONSERVATIVE               = 0x8D6A;
const GLenum GL_SAMPLER_BINDING                               = 0x8919;
const GLenum GL_RGB10_A2UI                                    = 0x906F;
const GLenum GL_INT_2_10_10_10_REV                            = 0x8D9F;
const GLenum GL_TRANSFORM_FEEDBACK_PAUSED                     = 0x8E23;
const GLenum GL_TRANSFORM_FEEDBACK_ACTIVE                     = 0x8E24;
const GLenum GL_TRANSFORM_FEEDBACK_BINDING                    = 0x8E25;
const GLenum GL_TEXTURE_IMMUTABLE_FORMAT                      = 0x912F;
const GLenum GL_MAX_ELEMENT_INDEX                             = 0x8D6B;

extern "C" GL_APICALL void* GL_APIENTRY     glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
extern "C" GL_APICALL GLboolean GL_APIENTRY glUnmapBuffer(GLenum target);
extern "C" GL_APICALL void GL_APIENTRY      glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
#endif

#if USE_WEBGL_STANDARD
extern "C" {
void         bfWebGLInitContext(void);
unsigned int bfWebGL_getUniformBlockIndex(GLuint program_id, const char* name);
void         bfWebGL_uniformBlockBinding(GLuint program_id, unsigned int index, unsigned int binding);
void         bfWebGL_bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
void         bfWebGL_handleResize(void);
}

#define glGetUniformBlockIndex bfWebGL_getUniformBlockIndex
#define glUniformBlockBinding bfWebGL_uniformBlockBinding
#define glBindBufferRange bfWebGL_bindBufferRange
#endif

using namespace bf;

//
// Memory
//

static char              s_GraphicsMemoryBacking[16384];
static FreeListAllocator s_GraphicsMemory{s_GraphicsMemoryBacking, sizeof(s_GraphicsMemoryBacking)};

template<typename T, typename... Args>
static T* allocate(Args&&... args);

template<typename T>
static void deallocate(T* ptr);

template<typename K, typename V, typename Hasher = std::hash<K>, class KeyEq = std::equal_to<K>>
using StdUnorderedMap = std::unordered_map<K, V, Hasher, KeyEq, StlAllocator<std::pair<const K, V>>>;

template<typename T>
using StdVector = std::vector<T, StlAllocator<T>>;

template<typename T>
struct StdUniquePtrDeleter final
{
  void operator()(T* ptr) const
  {
    deallocate(ptr);
  }
};

template<typename T>
using StdUniquePtr = std::unique_ptr<T, StdUniquePtrDeleter<T>>;

//
// Handle Definitions
//

BIFROST_DEFINE_HANDLE(GfxContext)
{
  const bfGfxContextCreateParams* params;                // Only valid during initialization
  std::uint32_t                   max_frames_in_flight;  // TODO(Shareef): Make customizable at runtime
  bfGfxDeviceHandle               logical_device;        // The APU
  bfFrameCount_t                  frame_count;           // Total Number of Frames that have been Rendered
  bfFrameCount_t                  frame_index;           // frame_count % max_frames_in_flight
};

BIFROST_DEFINE_HANDLE(GfxDevice)
{
  GfxRenderpassCache     cache_renderpass;
  VulkanPipelineCache    cache_pipeline;
  VulkanFramebufferCache cache_framebuffer;
  VulkanDescSetCache     cache_descriptor_set;
  BifrostGfxObjectBase*  cached_resources; /* Linked List */
};

BIFROST_DEFINE_HANDLE(Texture)
{
  BifrostGfxObjectBase   super;
  bfGfxDeviceHandle      parent;
  BifrostTexFeatureFlags flags;
  // CPU Side Data
  BifrostTextureType image_type;
  int32_t            image_width;
  int32_t            image_height;
  int32_t            image_depth;
  uint32_t           image_miplevels;
  // GPU Side Data
  GLuint                     tex_image; /* For Depth Textures this is an RBO */
  bfTextureSamplerProperties tex_sampler;
  BifrostSampleFlags         tex_samples;
};

BIFROST_DEFINE_HANDLE(Renderpass)
{
  BifrostGfxObjectBase super;
  bfRenderpassInfo     info;
};

BIFROST_DEFINE_HANDLE(Framebuffer)
{
  BifrostGfxObjectBase super;
  GLuint               handle;
  bfTextureHandle      attachments[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
};

BIFROST_DEFINE_HANDLE(Pipeline)
{
  BifrostGfxObjectBase super;
};

BIFROST_DEFINE_HANDLE(WindowSurface)
{
  struct bfWindow_t* window;
  bfGfxCommandListHandle  current_cmd_list;
  bfTexture               surface_dummy;
};

BIFROST_DEFINE_HANDLE(GfxCommandList)
{
  bfGfxContextHandle    context;
  bfGfxDeviceHandle     parent;
  bfWindowSurfaceHandle window;
  BifrostScissorRect    render_area;
  bfFramebufferHandle   framebuffer;
  bfPipelineHandle      pipeline;
  bfPipelineCache       pipeline_state;
  BifrostClearValue     clear_colors[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  bfBool32              has_command;
  uint16_t              dynamic_state_dirty;

  BifrostIndexType index_type;
  uint64_t         index_offset;
};

BIFROST_DEFINE_HANDLE(Buffer)
{
  BifrostGfxObjectBase super;
  GLuint               handle;
  GLenum               target;
  GLenum               usage;
  void*                mapped_ptr;
  bfBufferSize         real_size;
};

BIFROST_DEFINE_HANDLE(ShaderModule)
{
  BifrostGfxObjectBase super;
  bfGfxDeviceHandle    parent;
  BifrostShaderType    type;
  GLuint               handle;
  char                 entry_point[BIFROST_GFX_SHADER_ENTRY_POINT_NAME_LENGTH];
};

struct DescSetInfo final
{
  int num_textures;
  int texture_offset;
};

BIFROST_DEFINE_HANDLE(ShaderProgram)
{
  BifrostGfxObjectBase             super;
  bfGfxDeviceHandle                parent;
  GLuint                           handle;
  char                             debug_name[BIFROST_GFX_SHADER_PROGRAM_NAME_LENGTH];
  StdUnorderedMap<uint32_t, GLint> binding_to_uniform_loc;
  DescSetInfo*                     set_info;
  int                              num_sets;

  bfShaderProgram_t(const int num_sets) :
    super{},
    parent{},
    handle{},
    debug_name{},
    binding_to_uniform_loc{s_GraphicsMemory},
    set_info{s_GraphicsMemory.allocateArray<DescSetInfo>(num_sets)},
    num_sets{num_sets}
  {
  }

  ~bfShaderProgram_t()
  {
    s_GraphicsMemory.deallocateArray(set_info);
  }
};

BIFROST_DEFINE_HANDLE(DescriptorSet)
{
  BifrostGfxObjectBase                                                        super;
  bfShaderProgramHandle                                                       shader_program;
  uint32_t                                                                    set_index;
  StdVector<std::pair<GLuint, bfTextureHandle>>                               textures;        /* <Uniform, Texture>              */
  StdVector<std::tuple<uint32_t, bfBufferSize, bfBufferSize, bfBufferHandle>> ubos;            /* <Binding, Offset, Size, Buffer> */
  StdVector<std::pair<GLuint, bfTextureHandle>>                               textures_writes; /* <Uniform, Texture>              */
  StdVector<std::tuple<uint32_t, bfBufferSize, bfBufferSize, bfBufferHandle>> ubos_writes;     /* <Binding, Offset, Size, Buffer> */

  bfDescriptorSet_t() :
    super{},
    shader_program{},
    set_index{},
    textures{s_GraphicsMemory},
    ubos{s_GraphicsMemory},
    textures_writes{s_GraphicsMemory},
    ubos_writes{s_GraphicsMemory}
  {
  }
};

struct VertexLayoutSetDetail final
{
  int    num_components;
  GLenum component_type;
  void*  offset;
  bool   is_normalized;

  VertexLayoutSetDetail(int num_components, GLenum type, void* offset, bool is_normalized) :
    num_components{num_components},
    component_type{type},
    offset{offset},
    is_normalized{is_normalized}
  {
  }
};

struct VertexBindingDetail final
{
  std::uint32_t                    stride;
  StdVector<VertexLayoutSetDetail> details;

  VertexBindingDetail(std::uint32_t stride) :
    stride{stride},
    details{s_GraphicsMemory}
  {
  }

  void apply()
  {
    for (int i = 0; i < int(details.size()); ++i)
    {
      const VertexLayoutSetDetail& detail = details[i];

      glEnableVertexAttribArray(i);
      glVertexAttribPointer(i, detail.num_components, detail.component_type, detail.is_normalized ? GL_TRUE : GL_FALSE, stride, detail.offset);
    }
  }
};

BIFROST_DEFINE_HANDLE(VertexLayoutSet)
{
  // TOOD(SR): Switch to an array of 'BIFROST_GFX_BUFFERS_MAX_BINDING' size?
  StdUnorderedMap<std::uint32_t, VertexBindingDetail> vertex_bindings;
  bfBufferSize                                        vertex_buffer_offsets[BIFROST_GFX_BUFFERS_MAX_BINDING]; /*  offset in numn vertices */
  GLuint                                              vao_handle;
  uint8_t                                             num_buffer_bindings;
  uint8_t                                             num_attrib_bindings;
  uint8_t                                             num_vertex_buffers;
  std::pair<bfBufferHandle, std::uint32_t>            vertex_buffers[BIFROST_GFX_BUFFERS_MAX_BINDING];

  bfVertexLayoutSet_t() :
    vertex_bindings{s_GraphicsMemory},
    vertex_buffer_offsets{0},
    vao_handle{0},
    num_buffer_bindings{0},
    num_attrib_bindings{0},
    num_vertex_buffers{0},
    vertex_buffers{}
  {
  }

  void bind()
  {
    glBindVertexArray(vao_handle);
    num_vertex_buffers = 0;
  }

  void apply(std::uint32_t index, bfBufferSize byte_offset)
  {
    const auto it = vertex_bindings.find(index);

    num_vertex_buffers = std::max(num_vertex_buffers, uint8_t(index + 1));

    if (it != vertex_bindings.end())
    {
      it->second.apply();
      vertex_buffer_offsets[index] = byte_offset / it->second.stride;
    }
  }

  void apply(bfBufferHandle buffer, std::uint32_t index, bfBufferSize byte_offset)
  {
    vertex_buffers[index].first  = buffer;
    vertex_buffers[index].second = index;

    apply(index, byte_offset);
  }
};

//
// Helpers
//

static GLenum bfGLBufferUsageTarget(bfBufferUsageBits usage);
static GLenum bfGLBufferUsageHint(bfBufferPropertyBits properties, int mode);
static GLenum bfGLConvertShaderType(BifrostShaderType type);
static int    bfGLVertexFormatNumComponents(BifrostVertexFormatAttribute format);
static GLenum bfGLVertexFormatType(BifrostVertexFormatAttribute format);
static GLint  bfGLConvertSamplerAddressMode(BifrostSamplerAddressMode sampler_mode);
static GLint  bfConvertSamplerFilterMode(BifrostSamplerFilterMode filter_mode);
static bool   bfTextureIsDepthStencil(bfTextureHandle texture);
static bool   bfTextureCanBeInput(bfTextureHandle texture);
static GLenum bfConvertDrawMode(BifrostDrawMode draw_mode);
static GLenum bfConvertFrontFace(BifrostFrontFace face);
static GLenum bfGLConvertCmpOp(BifrostCompareOp op);

// bfGfxContext

bfGfxContextHandle bfGfxContext_new(const bfGfxContextCreateParams* params)
{
  const bfGfxContextHandle self = allocate<bfGfxContext>();

  if (self)
  {
    self->params               = params;
    self->max_frames_in_flight = 2;
    self->logical_device       = allocate<bfGfxDevice>();
    self->frame_count          = 0;
    self->frame_index          = 0;

#if USE_WEBGL_STANDARD
    bfWebGLInitContext();
#endif
  }

  return self;
}

bfGfxDeviceHandle bfGfxContext_device(bfGfxContextHandle self)
{
  return self->logical_device;
}

bfWindowSurfaceHandle bfGfxContext_createWindow(bfGfxContextHandle self, struct bfWindow_t* bf_window)
{
  (void)self;

  const bfWindowSurfaceHandle self_surface = allocate<bfWindowSurface>();

  if (self_surface)
  {
    self_surface->window           = bf_window;
    self_surface->current_cmd_list = nullptr;

#if !USE_WEBGL_STANDARD
    bfWindow_makeGLContextCurrent(bf_window);
    const int glad_successful = gladLoadGLLoader(bfPlatformGetProcAddress());
    assert(glad_successful != 0);
    (void)glad_successful;
#endif
  }

  return self_surface;
}

void bfGfxWindow_markResized(bfGfxContextHandle self, bfWindowSurfaceHandle window_handle)
{
  (void)self;
  (void)window_handle;
}

void bfGfxContext_destroyWindow(bfGfxContextHandle self, bfWindowSurfaceHandle window_handle)
{
  (void)self;

  deallocate(window_handle);
}

bfBool32 bfGfxContext_beginFrame(bfGfxContextHandle self, bfWindowSurfaceHandle window)
{
  (void)self;

  bfWindow_makeGLContextCurrent(window->window);

#if USE_WEBGL_STANDARD
  bfWebGL_handleResize();
#endif

  return true;
}

bfGfxFrameInfo bfGfxContext_getFrameInfo(bfGfxContextHandle self)
{
  return {self->frame_index, self->frame_count, self->max_frames_in_flight};
}

bfGfxCommandListHandle bfGfxContext_requestCommandList(bfGfxContextHandle self, bfWindowSurfaceHandle window, uint32_t thread_index)
{
  assert(thread_index == 0u && "Single theaded only.");

  if (window->current_cmd_list)
  {
    return window->current_cmd_list;
  }

  bfGfxCommandListHandle list = allocate<bfGfxCommandList>();

  list->context        = self;
  list->parent         = self->logical_device;
  list->window         = window;
  list->render_area    = {};
  list->framebuffer    = nullptr;
  list->pipeline       = nullptr;
  list->pipeline_state = {};
  list->has_command    = bfFalse;
  std::memset(list->clear_colors, 0x0, sizeof(list->clear_colors));
  std::memset(&list->pipeline_state, 0x0, sizeof(list->pipeline_state));  // Consistent hashing behavior + Memcmp is used for the cache system.

  bfGfxCmdList_setDefaultPipeline(list);

  window->current_cmd_list = list;

  return list;
}

template<typename T, typename TCache>
static void bfGfxContext_removeFromCache(TCache& cache, BifrostGfxObjectBase* object)
{
  cache.remove(object->hash_code, reinterpret_cast<T>(object));
}

void bfGfxContext_endFrame(bfGfxContextHandle self)
{
  BifrostGfxObjectBase* prev         = nullptr;
  BifrostGfxObjectBase* curr         = self->logical_device->cached_resources;
  BifrostGfxObjectBase* release_list = nullptr;

  while (curr)
  {
    BifrostGfxObjectBase* next = curr->next;

    if ((self->frame_count - curr->last_frame_used & bfFrameCountMax) >= 60)
    {
      if (prev)
      {
        prev->next = next;
      }
      else
      {
        self->logical_device->cached_resources = next;
      }

      curr->next   = release_list;
      release_list = curr;

      curr = next;

      if (curr)
      {
        next = curr->next;
      }
    }

    prev = curr;
    curr = next;
  }

  if (release_list)
  {
    while (release_list)
    {
      BifrostGfxObjectBase* next = release_list->next;

      switch (release_list->type)
      {
        case BIFROST_GFX_OBJECT_RENDERPASS:
        {
          bfGfxContext_removeFromCache<bfRenderpassHandle>(self->logical_device->cache_renderpass, release_list);
          break;
        }
        case BIFROST_GFX_OBJECT_PIPELINE:
        {
          bfGfxContext_removeFromCache<bfPipelineHandle>(self->logical_device->cache_pipeline, release_list);
          break;
        }
        case BIFROST_GFX_OBJECT_FRAMEBUFFER:
        {
          bfGfxContext_removeFromCache<bfFramebufferHandle>(self->logical_device->cache_framebuffer, release_list);
          break;
        }
        case BIFROST_GFX_OBJECT_DESCRIPTOR_SET:
        {
          bfGfxContext_removeFromCache<bfDescriptorSetHandle>(self->logical_device->cache_descriptor_set, release_list);
          break;
        }
          bfInvalidDefaultCase();
      }

      bfGfxDevice_release(self->logical_device, release_list);
      release_list = next;
    }
  }

  ++self->frame_count;
  self->frame_index = self->frame_count % self->max_frames_in_flight;
}

void bfGfxContext_delete(bfGfxContextHandle self)
{
  const auto device = self->logical_device;

  BifrostGfxObjectBase* curr = device->cached_resources;

  while (curr)
  {
    BifrostGfxObjectBase* next = curr->next;
    bfGfxDevice_release(self->logical_device, curr);
    curr = next;
  }

  deallocate(self->logical_device);
  deallocate(self);
}

void bfGfxDevice_flush(bfGfxDeviceHandle self)
{
  (void)self;
  glFlush();
}

bfBufferHandle bfGfxDevice_newBuffer(bfGfxDeviceHandle self, const bfBufferCreateParams* params)
{
  const bfBufferHandle buffer = allocate<bfBuffer>();

  if (buffer)
  {
    BifrostGfxObjectBase_ctor(&buffer->super, BIFROST_GFX_OBJECT_BUFFER);

    buffer->target    = bfGLBufferUsageTarget(params->usage);
    buffer->usage     = bfGLBufferUsageHint(params->allocation.properties, 0);
    buffer->real_size = params->allocation.size;

    glGenBuffers(1, &buffer->handle);

    glBindBuffer(buffer->target, buffer->handle);
    glBufferData(buffer->target, params->allocation.size, nullptr, buffer->usage);
  }

  return buffer;
}

bfRenderpassHandle bfGfxDevice_newRenderpass(bfGfxDeviceHandle self, const bfRenderpassCreateParams* params)
{
  const bfRenderpassHandle renderpass = allocate<bfRenderpass>();

  if (renderpass)
  {
    BifrostGfxObjectBase_ctor(&renderpass->super, BIFROST_GFX_OBJECT_RENDERPASS);

    renderpass->info = *params;
  }

  return renderpass;
}

bfShaderModuleHandle bfGfxDevice_newShaderModule(bfGfxDeviceHandle self, BifrostShaderType type)
{
  static const char k_GLEntryPoint[] = "main";

  bfShaderModuleHandle shader = allocate<bfShaderModule>();

  if (shader)
  {
    const GLenum gl_type = bfGLConvertShaderType(type);

    BifrostGfxObjectBase_ctor(&shader->super, BIFROST_GFX_OBJECT_SHADER_MODULE);

    shader->parent = self;
    shader->type   = type;
    shader->handle = glCreateShader(gl_type);

    for (int i = 0; i < int(sizeof(k_GLEntryPoint)); ++i)
    {
      shader->entry_point[i] = k_GLEntryPoint[i];
    }
  }

  return shader;
}

bfShaderProgramHandle bfGfxDevice_newShaderProgram(bfGfxDeviceHandle self, const bfShaderProgramCreateParams* params)
{
  bfShaderProgramHandle shader = allocate<bfShaderProgram>(params->num_desc_sets);

  if (shader)
  {
    BifrostGfxObjectBase_ctor(&shader->super, BIFROST_GFX_OBJECT_SHADER_PROGRAM);

    shader->parent = self;
    shader->handle = glCreateProgram();
    // ReSharper disable CppDeprecatedEntity
    std::strncpy(shader->debug_name, params->debug_name ? params->debug_name : "NO_DEBUG_NAME", bfCArraySize(shader->debug_name));
    // ReSharper restore CppDeprecatedEntity
  }

  return shader;
}

bfTextureHandle bfGfxDevice_newTexture(bfGfxDeviceHandle self, const bfTextureCreateParams* params)
{
  bfTextureHandle texture = allocate<bfTexture>();

  if (texture)
  {
    BifrostGfxObjectBase_ctor(&texture->super, BIFROST_GFX_OBJECT_TEXTURE);

    texture->parent          = self;
    texture->flags           = params->flags;
    texture->image_type      = params->type;
    texture->image_width     = params->width;
    texture->image_height    = params->height;
    texture->image_depth     = params->depth;
    texture->image_miplevels = params->generate_mipmaps;
    texture->tex_image       = 0;
    texture->tex_sampler     = bfTextureSamplerProperties{};
    texture->tex_samples     = BIFROST_SAMPLE_1;

    if (bfTextureIsDepthStencil(texture) && !bfTextureCanBeInput(texture))
    {
      glGenRenderbuffers(1, &texture->tex_image);
    }
    else
    {
      glGenTextures(1, &texture->tex_image);
    }
  }

  return texture;
}

bfTextureHandle bfGfxDevice_requestSurface(bfWindowSurfaceHandle window)
{
  bfWindow_getSize(window->window, &window->surface_dummy.image_width, &window->surface_dummy.image_height);

  return &window->surface_dummy;
}

bfDeviceLimits bfGfxDevice_limits(bfGfxDeviceHandle self)
{
  const bfDeviceLimits limits{0x100};
  return limits;
}

void bfGfxDevice_release(bfGfxDeviceHandle self, bfGfxBaseHandle resource)
{
  if (resource)
  {
    BifrostGfxObjectBase* const obj = static_cast<BifrostGfxObjectBase*>(resource);

    switch (obj->type)
    {
      case BIFROST_GFX_OBJECT_BUFFER:
      {
        bfBufferHandle buffer = reinterpret_cast<bfBufferHandle>(obj);

        glDeleteBuffers(1, &buffer->handle);

        self->cache_descriptor_set.forEach([buffer](bfDescriptorSetHandle desc_set, bfDescriptorSetInfo& config_data) {
          (void)desc_set;

          for (uint32_t i = 0; i < config_data.num_bindings; ++i)
          {
            bfDescriptorElementInfo* const binding_a = &config_data.bindings[i];

            for (uint32_t j = 0; j < binding_a->num_handles; ++j)
            {
              if (binding_a->handles[j] == buffer)
              {
                binding_a->handles[j] = nullptr;
              }
            }
          }
        });

        deallocate(buffer);

        break;
      }
      case BIFROST_GFX_OBJECT_RENDERPASS:
      {
        const bfRenderpassHandle renderpass = reinterpret_cast<bfRenderpassHandle>(obj);

        deallocate(renderpass);
        break;
      }
      case BIFROST_GFX_OBJECT_SHADER_MODULE:
      {
        const bfShaderModuleHandle shader_module = reinterpret_cast<bfShaderModuleHandle>(obj);

        if (shader_module->handle)
        {
          glDeleteShader(shader_module->handle);
        }

        deallocate(shader_module);
        break;
      }
      case BIFROST_GFX_OBJECT_SHADER_PROGRAM:
      {
        const bfShaderProgramHandle shader_program = reinterpret_cast<bfShaderProgramHandle>(obj);

        glDeleteProgram(shader_program->handle);

        deallocate(shader_program);
        break;
      }
      case BIFROST_GFX_OBJECT_DESCRIPTOR_SET:
      {
        const bfDescriptorSetHandle desc_set = reinterpret_cast<bfDescriptorSetHandle>(obj);

        deallocate(desc_set);
        break;
      }
      case BIFROST_GFX_OBJECT_TEXTURE:
      {
        bfTextureHandle texture = reinterpret_cast<bfTextureHandle>(obj);

        if (bfTextureIsDepthStencil(texture) && !bfTextureCanBeInput(texture))
        {
          glDeleteRenderbuffers(1, &texture->tex_image);
        }
        else
        {
          glDeleteTextures(1, &texture->tex_image);
        }

        self->cache_descriptor_set.forEach([texture](bfDescriptorSetHandle desc_set, bfDescriptorSetInfo& config_data) {
          (void)desc_set;

          for (uint32_t i = 0; i < config_data.num_bindings; ++i)
          {
            bfDescriptorElementInfo* const binding_a = &config_data.bindings[i];

            for (uint32_t j = 0; j < binding_a->num_handles; ++j)
            {
              if (binding_a->handles[j] == texture)
              {
                binding_a->handles[j] = nullptr;
              }
            }
          }
        });

        self->cache_framebuffer.forEach([texture](bfFramebufferHandle fb, bfFramebufferState& config_data) {
          for (uint32_t i = 0; i < config_data.num_attachments; ++i)
          {
            if (config_data.attachments[i] == texture)
            {
              config_data.attachments[i] = nullptr;
              fb->attachments[i]         = nullptr;
            }
          }
        });

        deallocate(texture);

        break;
      }
      case BIFROST_GFX_OBJECT_FRAMEBUFFER:
      {
        const bfFramebufferHandle framebuffer = reinterpret_cast<bfFramebufferHandle>(obj);

        glDeleteFramebuffers(1, &framebuffer->handle);

        deallocate(framebuffer);

        break;
      }
      case BIFROST_GFX_OBJECT_PIPELINE:
      {
        const bfPipelineHandle pipeline = reinterpret_cast<bfPipelineHandle>(obj);

        deallocate(pipeline);
        break;
      }
      default:
      {
        assert(!"Invalid object type.");
        break;
      }
    }
  }
}

/* Buffer */
bfBufferSize bfBuffer_size(bfBufferHandle self)
{
  return self->real_size;
}

void* bfBuffer_mappedPtr(bfBufferHandle self)
{
  return self->mapped_ptr;
}

void* bfBuffer_map(bfBufferHandle self, bfBufferSize offset, bfBufferSize size)
{
#if USE_WEBGL_STANDARD
  const GLbitfield access_flags = /*0x1A | */ 0xA;
#else
  const GLbitfield access_flags = GL_MAP_WRITE_BIT/* | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT*/;
#endif

  const auto whole_size = static_cast<GLsizeiptr>(std::min(self->real_size - offset, size));

  glBindBuffer(self->target, self->handle);
  self->mapped_ptr = glMapBufferRange(self->target, static_cast<GLintptr>(offset), whole_size, access_flags);
  return self->mapped_ptr;
}

void bfBuffer_invalidateRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges)
{
  glBindBuffer(self->target, self->handle);

#if USE_OPENGL_ES_STANDARD
  glBufferData(self->target, self->real_size, nullptr, self->usage);
#else
  for (uint32_t i = 0; i < num_ranges; ++i)
  {
    glInvalidateBufferSubData(self->handle, offsets[i], sizes[i]);
  }
#endif
}

void bfBuffer_copyCPU(bfBufferHandle self, bfBufferSize dst_offset, const void* data, bfBufferSize num_bytes)
{
  // std::memcpy(static_cast<char*>(self->mapped_ptr) + dst_offset, data, std::size_t(num_bytes));
}

void bfBuffer_copyGPU(bfBufferHandle src, bfBufferSize src_offset, bfBufferHandle dst, bfBufferSize dst_offset, bfBufferSize num_bytes)
{
  assert(src_offset + num_bytes <= src->real_size);
  assert(dst_offset + num_bytes <= dst->real_size);

  glBindBuffer(GL_COPY_READ_BUFFER, src->handle);
  glBindBuffer(GL_COPY_WRITE_BUFFER, dst->handle);
  glCopyBufferSubData(GL_COPY_READ_BUFFER,
                      GL_COPY_WRITE_BUFFER,
                      GLintptr(src_offset),
                      GLintptr(dst_offset),
                      GLsizeiptr(num_bytes));
}

void bfBuffer_flushRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges)
{
#if !USE_WEBGL_STANDARD
  for (uint32_t i = 0; i < num_ranges; ++i)
  {
    glFlushMappedBufferRange(self->target, offsets[i], sizes[i]);
  }
#endif
}

void bfBuffer_unMap(bfBufferHandle self)
{
  assert(self->mapped_ptr);

  glUnmapBuffer(self->target);
  self->mapped_ptr = nullptr;
}

/* Vertex Binding */
bfVertexLayoutSetHandle bfVertexLayout_new()
{
  bfVertexLayoutSetHandle self = allocate<bfVertexLayoutSet>();

  if (self)
  {
    glGenVertexArrays(1, &self->vao_handle);
  }

  return self;
}

void bfVertexLayout_addVertexBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t sizeof_vertex)
{
  self->vertex_bindings.emplace(binding, VertexBindingDetail{sizeof_vertex});
}

void bfVertexLayout_addInstanceBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t stride)
{
  assert(!"NOT IMPLEMENTED IN THE OPENGL BACKEND");
}

void bfVertexLayout_addVertexLayout(bfVertexLayoutSetHandle self, uint32_t binding, BifrostVertexFormatAttribute format, uint32_t offset)
{
  const auto it = self->vertex_bindings.find(binding);

  if (it != self->vertex_bindings.end())
  {
    VertexBindingDetail& detail        = it->second;
    const int            num_comps     = bfGLVertexFormatNumComponents(format);
    const GLenum         type          = bfGLVertexFormatType(format);
    const bool           is_normalized = format == BIFROST_VFA_UCHAR8_4_UNORM;

    detail.details.emplace_back(num_comps, type, reinterpret_cast<void*>(uintptr_t(offset)), is_normalized);
  }
}

void bfVertexLayout_delete(bfVertexLayoutSetHandle self)
{
  glDeleteVertexArrays(1, &self->vao_handle);
  deallocate(self);
}

/* Shader Program + Module */

BifrostShaderType bfShaderModule_type(bfShaderModuleHandle self)
{
  return self->type;
}

bfBool32 bfShaderModule_loadData(bfShaderModuleHandle self, const char* source, size_t source_length)
{
  const char* sources[] =
  {
#if USE_OPENGL_ES_STANDARD
    "#version 300 es\n",
    "precision mediump float;\n",
#endif
    source,
  };

  const GLint source_lengths[] =
  {
#if USE_OPENGL_ES_STANDARD
    sizeof("#version 300 es\n") - 1,
    sizeof("precision mediump float;\n") - 1,
#endif

    GLint(source_length),
  };

  glShaderSource(self->handle, (GLsizei)bfCArraySize(sources), sources, source_lengths);
  glCompileShader(self->handle);

  int  success;
  char info_log[512];
  glGetShaderiv(self->handle, GL_COMPILE_STATUS, &success);

  if (!success)
  {
    glGetShaderInfoLog(self->handle, 512, nullptr, info_log);
    printf("%s\n", info_log);
    assert(false);
  }

  return success;
}

void bfShaderProgram_addModule(bfShaderProgramHandle self, bfShaderModuleHandle module)
{
  glAttachShader(self->handle, module->handle);
}

void bfShaderProgram_link(bfShaderProgramHandle self)
{
  int texture_offset = 0;

  for (int i = 0; i < self->num_sets; ++i)
  {
    self->set_info[i].texture_offset = texture_offset;
    texture_offset += self->set_info[i].num_textures;
  }

  glLinkProgram(self->handle);

  int  success;
  char info_log[512];

  glGetProgramiv(self->handle, GL_LINK_STATUS, &success);

  if (!success)
  {
    glGetProgramInfoLog(self->handle, 512, nullptr, info_log);
    printf("%s\n", info_log);
    assert(false);
  }
}

void bfShaderProgram_addAttribute(bfShaderProgramHandle self, const char* name, uint32_t binding)
{
  glBindAttribLocation(self->handle, binding, name);
}

void bfShaderProgram_addUniformBuffer(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, BifrostShaderStageBits stages)
{
  // NOTE FROM LearnOpenGL, We cannot support this just yet...
  //
  // From OpenGL version 4.2 and onwards it is also possible to store the binding point of a uniform block explicitly in the shader by adding another layout specifier,
  // saving us the calls to glGetUniformBlockIndex and glUniformBlockBinding.The following code sets the binding point of the Lights uniform block explicitly :
  //
  //   layout(std140, binding = 2) uniform Lights{...};
  //

  // assert(how_many == 1 && "??? IDK how to map this concept");

  (void)set;
  (void)how_many;
  (void)stages;

  const unsigned int ubo_index = glGetUniformBlockIndex(self->handle, name);

  glUniformBlockBinding(self->handle, ubo_index, binding);
}

void bfShaderProgram_addImageSampler(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, BifrostShaderStageBits stages)
{
  // assert(set == 0 && "OpenGL backend only supports one flat descriptor set");
  // assert(how_many == 1 && "??? IDK how to map this concept");

  (void)how_many;
  (void)stages;

  self->binding_to_uniform_loc[binding] = glGetUniformLocation(self->handle, name);

  self->set_info[set].num_textures += 1;
}

void bfShaderProgram_compile(bfShaderProgramHandle self)
{
  /* NO-OP By Design */
}

bfDescriptorSetHandle bfShaderProgram_createDescriptorSet(bfShaderProgramHandle self, uint32_t index)
{
  const bfDescriptorSetHandle desc_set = allocate<bfDescriptorSet>();

  if (desc_set)
  {
    BifrostGfxObjectBase_ctor(&desc_set->super, BIFROST_GFX_OBJECT_DESCRIPTOR_SET);

    desc_set->shader_program = self;
    desc_set->set_index      = index;
  }

  return desc_set;
}

void bfDescriptorSet_setCombinedSamplerTextures(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures)
{
  const auto it = self->shader_program->binding_to_uniform_loc.find(binding);

  if (it != self->shader_program->binding_to_uniform_loc.end())
  {
    const auto base_uniform = it->second + array_element_start;

    for (uint32_t i = 0; i < num_textures; ++i)
    {
      self->textures_writes.emplace_back(base_uniform + i, textures[i]);
    }
  }
  else
  {
    assert(false);
  }
}

void bfDescriptorSet_setUniformBuffers(bfDescriptorSetHandle self, uint32_t binding, const bfBufferSize* offsets, const bfBufferSize* sizes, bfBufferHandle* buffers, uint32_t num_buffers)
{
  for (uint32_t i = 0; i < num_buffers; ++i)
  {
    self->ubos_writes.emplace_back(binding, offsets[i], sizes[i], buffers[i]);
  }
}

void bfDescriptorSet_flushWrites(bfDescriptorSetHandle self)
{
  self->textures.clear();
  self->ubos.clear();

  std::swap(self->textures, self->textures_writes);
  std::swap(self->ubos, self->ubos_writes);
}

/* Texture */

uint32_t bfTexture_width(bfTextureHandle self)
{
  return self->image_width;
}

uint32_t bfTexture_height(bfTextureHandle self)
{
  return self->image_height;
}

uint32_t bfTexture_depth(bfTextureHandle self)
{
  return self->image_depth;
}

uint32_t bfTexture_numMipLevels(bfTextureHandle self)
{
  return self->image_miplevels;
}

BifrostImageLayout bfTexture_layout(bfTextureHandle self)
{
  return BIFROST_IMAGE_LAYOUT_GENERAL;
}

static constexpr size_t k_NumReqComps = 4;

bfBool32 bfTexture_loadFile(bfTextureHandle self, const char* file)
{
  int      num_components = 0;
  stbi_uc* texture_data   = stbi_load(file, &self->image_width, &self->image_height, &num_components, STBI_rgb_alpha);

  if (texture_data)
  {
    const size_t num_req_bytes = size_t(self->image_width) * size_t(self->image_height) * k_NumReqComps * sizeof(char);

    bfTexture_loadData(self, reinterpret_cast<const char*>(texture_data), num_req_bytes);

    stbi_image_free(texture_data);

    return bfTrue;
  }

  return bfFalse;
}

void bfTexture_loadBuffer(bfTextureHandle self, bfBufferHandle buffer);

bfBool32 bfTexture_loadData(bfTextureHandle self, const char* pixels, size_t pixels_length)
{
  const bool is_indefinite =
   self->image_width == BIFROST_TEXTURE_UNKNOWN_SIZE ||
   self->image_height == BIFROST_TEXTURE_UNKNOWN_SIZE ||
   self->image_depth == BIFROST_TEXTURE_UNKNOWN_SIZE;

  assert(!is_indefinite && "Texture_setData: The texture dimensions should be defined by this point.");

  self->image_miplevels = self->image_miplevels ? 1 + uint32_t(std::floor(std::log2(float(std::max(std::max(self->image_width, self->image_height), self->image_depth))))) : 1;

  if (pixels)
  {
    assert(size_t(self->image_width) * size_t(self->image_height) * 4 * sizeof(char) == pixels_length && "Not enough texture data");
  }

  if (bfTextureIsDepthStencil(self))
  {
    if (bfTextureCanBeInput(self))
    {
      const GLenum internal_format = (self->flags & BIFROST_TEX_IS_STENCIL_ATTACHMENT) ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24;

      glBindTexture(GL_TEXTURE_2D, self->tex_image);
      glTexImage2D(
       GL_TEXTURE_2D,
       0,
       internal_format,
       self->image_width,
       self->image_height,
       0,
       GL_DEPTH_STENCIL,
       GL_UNSIGNED_INT_24_8,
       pixels);
    }
    else
    {
      const GLenum internal_format = (self->flags & BIFROST_TEX_IS_STENCIL_ATTACHMENT) ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24;

      glBindRenderbuffer(GL_RENDERBUFFER, self->tex_image);

      // Same as "glRenderbufferStorageMultisample" w/ samples set to 0.
      glRenderbufferStorage(GL_RENDERBUFFER, internal_format, self->image_width, self->image_height);
    }
  }
  else
  {
    glBindTexture(GL_TEXTURE_2D, self->tex_image);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->image_width, self->image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    if (self->image_miplevels > 1)
    {
      glGenerateMipmap(GL_TEXTURE_2D);
    }
  }

  return bfTrue;
}

void bfTexture_setSampler(bfTextureHandle self, const bfTextureSamplerProperties* sampler_properties)
{
  if (sampler_properties && !(self->flags & (BIFROST_TEX_IS_DEPTH_ATTACHMENT | BIFROST_TEX_IS_STENCIL_ATTACHMENT)))
  {
    glBindTexture(GL_TEXTURE_2D, self->tex_image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, bfGLConvertSamplerAddressMode(sampler_properties->u_address));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, bfGLConvertSamplerAddressMode(sampler_properties->v_address));
#if !USE_OPENGL_ES_STANDARD
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, bfGLConvertSamplerAddressMode(sampler_properties->w_address));
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bfConvertSamplerFilterMode(sampler_properties->min_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bfConvertSamplerFilterMode(sampler_properties->mag_filter));

#if !USE_OPENGL_ES_STANDARD
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, sampler_properties->min_lod);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, sampler_properties->max_lod);
#endif
  }
}

static void UpdateResourceFrame(bfGfxContextHandle ctx, BifrostGfxObjectBase* obj)
{
  obj->last_frame_used = ctx->frame_count;
}

static void AddCachedResource(bfGfxDeviceHandle device, BifrostGfxObjectBase* obj, std::uint64_t hash_code)
{
  obj->hash_code           = hash_code;
  obj->next                = device->cached_resources;
  device->cached_resources = obj;
}

bfWindowSurfaceHandle bfGfxCmdList_window(bfGfxCommandListHandle self)
{
  return self->window;
}

bfBool32 bfGfxCmdList_begin(bfGfxCommandListHandle self)
{
  self->dynamic_state_dirty = 0xFFFF;

  return bfTrue;
}

void bfGfxCmdList_pipelineBarriers(bfGfxCommandListHandle self, BifrostPipelineStageBits src_stage, BifrostPipelineStageBits dst_stage, const bfPipelineBarrier* barriers, uint32_t num_barriers, bfBool32 reads_same_pixel)
{
  /* NO-OP By Design */
}

void bfGfxCmdList_setRenderpass(bfGfxCommandListHandle self, bfRenderpassHandle renderpass)
{
  self->pipeline_state.renderpass = renderpass;
  UpdateResourceFrame(self->context, &renderpass->super);
}

void bfGfxCmdList_setRenderpassInfo(bfGfxCommandListHandle self, const bfRenderpassInfo* renderpass_info)
{
  const uint64_t hash_code = gfx_hash::hash(0x0, renderpass_info);

  bfRenderpassHandle rp = self->parent->cache_renderpass.find(hash_code, *renderpass_info);

  if (!rp)
  {
    rp = bfGfxDevice_newRenderpass(self->parent, renderpass_info);
    self->parent->cache_renderpass.insert(hash_code, rp, *renderpass_info);
    AddCachedResource(self->parent, &rp->super, hash_code);
  }

  bfGfxCmdList_setRenderpass(self, rp);
}

void bfGfxCmdList_setClearValues(bfGfxCommandListHandle self, const BifrostClearValue* clear_values)
{
  const std::size_t num_clear_colors = self->pipeline_state.renderpass->info.num_attachments;

  for (std::size_t i = 0; i < num_clear_colors; ++i)
  {
    self->clear_colors[i] = clear_values[i];
  }
}

void bfGfxCmdList_setAttachments(bfGfxCommandListHandle self, bfTextureHandle* attachments)
{
  const uint32_t num_attachments = self->pipeline_state.renderpass->info.num_attachments;
  const uint64_t hash_code       = gfx_hash::hash(0x0, attachments, num_attachments);

  bfFramebufferState fb_state;
  fb_state.num_attachments = num_attachments;

  for (uint32_t i = 0; i < num_attachments; ++i)
  {
    fb_state.attachments[i] = attachments[i];
  }

  bfFramebufferHandle fb = self->parent->cache_framebuffer.find(hash_code, fb_state);

  if (!fb)
  {
    fb = allocate<bfFramebuffer>();

    BifrostGfxObjectBase_ctor(&fb->super, BIFROST_GFX_OBJECT_FRAMEBUFFER);

    if (attachments[0] != &self->window->surface_dummy)
    {
      glGenFramebuffers(1, &fb->handle);
      glBindFramebuffer(GL_FRAMEBUFFER, fb->handle);

      for (uint32_t i = 0; i < num_attachments; ++i)
      {
        fb->attachments[i] = attachments[i];

        if (bfTextureIsDepthStencil(attachments[i]))
        {
          if (bfTextureCanBeInput(attachments[i]))
          {
            if ((attachments[i]->flags & BIFROST_TEX_IS_STENCIL_ATTACHMENT) && (attachments[i]->flags & BIFROST_TEX_IS_DEPTH_ATTACHMENT))
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, attachments[i]->tex_image, 0);
            }
            else if (attachments[i]->flags & BIFROST_TEX_IS_STENCIL_ATTACHMENT)
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, attachments[i]->tex_image, 0);
            }
            else
            {
              glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, attachments[i]->tex_image, 0);
            }
          }
          else
          {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, attachments[i]->tex_image);
          }
        }
        else
        {
          glFramebufferTexture2D(
           GL_FRAMEBUFFER,
           GL_COLOR_ATTACHMENT0 + i,
           GL_TEXTURE_2D,
           attachments[i]->tex_image,
           0);
        }
      }
    }
    else
    {
      fb->attachments[0] = attachments[0];
      fb->handle         = 0;  // Default Framebuffer
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    self->parent->cache_framebuffer.insert(hash_code, fb, fb_state);
    AddCachedResource(self->parent, &fb->super, hash_code);
  }

  self->framebuffer = fb;
  UpdateResourceFrame(self->context, &fb->super);
}

void bfGfxCmdList_setRenderAreaAbs(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
  self->render_area.x      = x;
  self->render_area.y      = y;
  self->render_area.width  = width;
  self->render_area.height = height;

  const float depths[2] = {0.0f, 1.0f};
  bfGfxCmdList_setViewport(self, float(x), float(y), float(width), float(height), depths);
  bfGfxCmdList_setScissorRect(self, x, y, width, height);
}

extern "C" void bfGfxCmdList_setRenderAreaRelImpl(bfTextureHandle texture, bfGfxCommandListHandle self, float x, float y, float width, float height);

void bfGfxCmdList_setRenderAreaRel(bfGfxCommandListHandle self, float x, float y, float width, float height)
{
  bfGfxCmdList_setRenderAreaRelImpl(self->framebuffer->attachments[0], self, x, y, width, height);
}

void bfGfxCmdList_beginRenderpass(bfGfxCommandListHandle self)
{
  bfRenderpassInfo& rp_info         = self->pipeline_state.renderpass->info;
  const uint32_t    num_attachments = rp_info.num_attachments;

  glBindFramebuffer(GL_FRAMEBUFFER, self->framebuffer->handle);

  for (uint32_t i = 0; i < num_attachments; ++i)
  {
    GLbitfield clear_mask = 0x0;

    if (rp_info.clear_ops & bfBit(i))
    {
      clear_mask |= GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    }

    if (rp_info.stencil_clear_ops & bfBit(i))
    {
      clear_mask |= GL_STENCIL_BUFFER_BIT;
    }

    if (clear_mask)
    {
      GLuint draw_buffers[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS] =
       {
        GL_NONE,
        GL_NONE,
        GL_NONE,
        GL_NONE,
        GL_NONE,
        GL_NONE,
        GL_NONE,
        GL_NONE,
       };

      const auto&                     clear_value   = self->clear_colors[i];
      const float*                    colors        = clear_value.color.float32;
      const BifrostClearDepthStencil& depth_stencil = clear_value.depthStencil;

      draw_buffers[i] = GL_BACK;  // GL_COLOR_ATTACHMENT0 + i;

      glDrawBuffers(num_attachments, draw_buffers);
      glClearColor(colors[0], colors[1], colors[2], colors[3]);
      glClearDepthf(depth_stencil.depth);
      glClearStencil(depth_stencil.stencil);
      glClear(clear_mask);
    }
  }

  self->pipeline_state.subpass_index = 0;
  bfGfxCmdList_nextSubpass(self);
}

void bfGfxCmdList_nextSubpass(bfGfxCommandListHandle self)
{
  GLuint draw_buffers[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS] =
   {
    GL_NONE,
    GL_NONE,
    GL_NONE,
    GL_NONE,
    GL_NONE,
    GL_NONE,
    GL_NONE,
    GL_NONE,
   };

  bfRenderpassInfo& rp_info         = self->pipeline_state.renderpass->info;
  const uint32_t    num_attachments = rp_info.num_attachments;
  bfSubpassCache&   subpass         = rp_info.subpasses[self->pipeline_state.subpass_index];

  for (uint16_t i = 0; i < subpass.num_out_attachment_refs; ++i)
  {
    const int16_t att_idx = subpass.out_attachment_refs[i].attachment_index;

    draw_buffers[att_idx] = GL_BACK;  // GL_COLOR_ATTACHMENT0 + att_idx;
  }

  assert(subpass.num_in_attachment_refs == 0 && "Input attachments not supported by OpenGL Backend");

  glDrawBuffers(num_attachments, draw_buffers);

  ++self->pipeline_state.subpass_index;
}

#define state(self) self->pipeline_state.state

void bfGfxCmdList_setDrawMode(bfGfxCommandListHandle self, BifrostDrawMode draw_mode)
{
  state(self).draw_mode = draw_mode;
}

void bfGfxCmdList_setFrontFace(bfGfxCommandListHandle self, BifrostFrontFace front_face)
{
  state(self).front_face = front_face;

  glFrontFace(bfConvertFrontFace(front_face));
}

void bfGfxCmdList_setCullFace(bfGfxCommandListHandle self, BifrostCullFaceFlags cull_face)
{
  state(self).cull_face = cull_face;

  if (cull_face)
  {
    glEnable(GL_CULL_FACE);

    GLenum gl_face = 0;

    switch (cull_face)
    {
      case BIFROST_CULL_FACE_FRONT: gl_face = GL_FRONT; break;
      case BIFROST_CULL_FACE_BACK: gl_face = GL_BACK; break;
      case BIFROST_CULL_FACE_BOTH: gl_face = GL_FRONT_AND_BACK; break;
      default:
        assert(false);
        break;
    }

    glCullFace(gl_face);
  }
  else
  {
    glDisable(GL_CULL_FACE);
  }
}

void bfGfxCmdList_setDepthTesting(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_depth_test = value;

  if (value)
  {
    glEnable(GL_DEPTH_TEST);
  }
  else
  {
    glDisable(GL_DEPTH_TEST);
  }
}

void bfGfxCmdList_setDepthWrite(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).depth_write = value;

  glDepthMask(value ? GL_TRUE : GL_FALSE);
}

void bfGfxCmdList_setDepthTestOp(bfGfxCommandListHandle self, BifrostCompareOp op)
{
  state(self).depth_test_op = op;

  glDepthFunc(bfGLConvertCmpOp(op));
}

void bfGfxCmdList_setStencilTesting(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_stencil_test = value;

  if (value)
  {
    glEnable(GL_STENCIL_TEST);
  }
  else
  {
    glDisable(GL_STENCIL_TEST);
  }
}

void bfGfxCmdList_setPrimitiveRestart(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).primitive_restart = value;

  assert(!value && "I need to do some research on primitive restart for OpenGL.");
}

void bfGfxCmdList_setRasterizerDiscard(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).rasterizer_discard = value;

#if USE_WEBGL_STANDARD
  assert(!value && "Not supported on WebGL");
#else
  if (value)
  {
    glEnable(GL_RASTERIZER_DISCARD);
  }
  else
  {
    glDisable(GL_RASTERIZER_DISCARD);
  }
#endif
}

void bfGfxCmdList_setDepthBias(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_depth_bias = value;

  // glPolygonOffset(1.0, 1.0);
}

void bfGfxCmdList_setSampleShading(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_sample_shading = value;
}

void bfGfxCmdList_setAlphaToCoverage(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).alpha_to_coverage = value;
}

void bfGfxCmdList_setAlphaToOne(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).alpha_to_one = value;
}

void bfGfxCmdList_setLogicOp(bfGfxCommandListHandle self, BifrostLogicOp op)
{
  state(self).logic_op = op;
}

void bfGfxCmdList_setPolygonFillMode(bfGfxCommandListHandle self, BifrostPolygonFillMode fill_mode)
{
  state(self).fill_mode = fill_mode;
}

#undef state

void bfGfxCmdList_setColorWriteMask(bfGfxCommandListHandle self, uint32_t output_attachment_idx, uint8_t color_mask)
{
  self->pipeline_state.blending[output_attachment_idx].color_write_mask = color_mask;
}

void bfGfxCmdList_setColorBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendOp op)
{
  self->pipeline_state.blending[output_attachment_idx].color_blend_op = op;
}

void bfGfxCmdList_setBlendSrc(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].color_blend_src = factor;
}

void bfGfxCmdList_setBlendDst(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].color_blend_dst = factor;
}

void bfGfxCmdList_setAlphaBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendOp op)
{
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_op = op;
}

void bfGfxCmdList_setBlendSrcAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_src = factor;
}

void bfGfxCmdList_setBlendDstAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_dst = factor;
}

void bfGfxCmdList_setStencilFailOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op)
{
  if (face == BIFROST_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_fail_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_fail_op = op;
  }
}

void bfGfxCmdList_setStencilPassOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op)
{
  if (face == BIFROST_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_pass_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_pass_op = op;
  }
}

void bfGfxCmdList_setStencilDepthFailOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op)
{
  if (face == BIFROST_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_depth_fail_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_depth_fail_op = op;
  }
}
void bfGfxCmdList_setStencilCompareOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostCompareOp op)
{
  if (face == BIFROST_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_compare_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_compare_op = op;
  }
}

void bfGfxCmdList_setStencilCompareMask(bfGfxCommandListHandle self, BifrostStencilFace face, uint8_t cmp_mask)
{
  if (face == BIFROST_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_compare_mask = cmp_mask;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_compare_mask = cmp_mask;
  }

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK;
}

void bfGfxCmdList_setStencilWriteMask(bfGfxCommandListHandle self, BifrostStencilFace face, uint8_t write_mask)
{
  if (face == BIFROST_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_write_mask = write_mask;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_write_mask = write_mask;
  }

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK;
}

void bfGfxCmdList_setStencilReference(bfGfxCommandListHandle self, BifrostStencilFace face, uint8_t ref_mask)
{
  if (face == BIFROST_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_reference = ref_mask;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_reference = ref_mask;
  }

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_STENCIL_REFERENCE;
}

void bfGfxCmdList_setDynamicStates(bfGfxCommandListHandle self, uint16_t dynamic_states)
{
  auto& s = self->pipeline_state.state;

  const bfBool32 set_dynamic_viewport           = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_VIEWPORT) != 0;
  const bfBool32 set_dynamic_scissor            = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_SCISSOR) != 0;
  const bfBool32 set_dynamic_line_width         = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_LINE_WIDTH) != 0;
  const bfBool32 set_dynamic_depth_bias         = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_DEPTH_BIAS) != 0;
  const bfBool32 set_dynamic_blend_constants    = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_BLEND_CONSTANTS) != 0;
  const bfBool32 set_dynamic_depth_bounds       = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_DEPTH_BOUNDS) != 0;
  const bfBool32 set_dynamic_stencil_cmp_mask   = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK) != 0;
  const bfBool32 set_dynamic_stencil_write_mask = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK) != 0;
  const bfBool32 set_dynamic_stencil_reference  = (dynamic_states & BIFROST_PIPELINE_DYNAMIC_STENCIL_REFERENCE) != 0;

  s.dynamic_viewport           = set_dynamic_viewport;
  s.dynamic_scissor            = set_dynamic_scissor;
  s.dynamic_line_width         = set_dynamic_line_width;
  s.dynamic_depth_bias         = set_dynamic_depth_bias;
  s.dynamic_blend_constants    = set_dynamic_blend_constants;
  s.dynamic_depth_bounds       = set_dynamic_depth_bounds;
  s.dynamic_stencil_cmp_mask   = set_dynamic_stencil_cmp_mask;
  s.dynamic_stencil_write_mask = set_dynamic_stencil_write_mask;
  s.dynamic_stencil_reference  = set_dynamic_stencil_reference;

  self->dynamic_state_dirty = dynamic_states;
}

void bfGfxCmdList_setViewport(bfGfxCommandListHandle self, float x, float y, float width, float height, const float depth[2])
{
  static constexpr float k_DefaultDepth[2] = {0.0f, 1.0f};

  if (depth == nullptr)
  {
    depth = k_DefaultDepth;
  }

  auto& vp     = self->pipeline_state.viewport;
  vp.x         = x;
  vp.y         = y;
  vp.width     = width;
  vp.height    = height;
  vp.min_depth = depth[0];
  vp.max_depth = depth[1];

  glDepthRangef(depth[0], depth[1]);
  glViewport(GLint(x), GLint(y), GLsizei(width), GLsizei(height));

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_VIEWPORT;
}

void bfGfxCmdList_setScissorRect(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
  auto& s = self->pipeline_state.scissor_rect;

  s.x      = x;
  s.y      = y;
  s.width  = width;
  s.height = height;

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_SCISSOR;

  glScissor(x, y, width, height);
}

void bfGfxCmdList_setBlendConstants(bfGfxCommandListHandle self, const float constants[4])
{
  memcpy(self->pipeline_state.blend_constants, constants, sizeof(self->pipeline_state.blend_constants));

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_BLEND_CONSTANTS;

  glBlendColor(constants[0], constants[1], constants[2], constants[3]);
}

void bfGfxCmdList_setLineWidth(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.line_width = value;

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_LINE_WIDTH;

  glLineWidth(value);
}

void bfGfxCmdList_setDepthClampEnabled(bfGfxCommandListHandle self, bfBool32 value)
{
  self->pipeline_state.state.do_depth_clamp = value;
}

void bfGfxCmdList_setDepthBoundsTestEnabled(bfGfxCommandListHandle self, bfBool32 value)
{
  self->pipeline_state.state.do_depth_bounds_test = value;
}

void bfGfxCmdList_setDepthBounds(bfGfxCommandListHandle self, float min, float max)
{
  self->pipeline_state.depth.min_bound = min;
  self->pipeline_state.depth.max_bound = max;

  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_DEPTH_BOUNDS;
}

void bfGfxCmdList_setDepthBiasConstantFactor(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_constant_factor = value;
  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_DEPTH_BIAS;
}

void bfGfxCmdList_setDepthBiasClamp(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_clamp = value;
  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_DEPTH_BIAS;
}

void bfGfxCmdList_setDepthBiasSlopeFactor(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_slope_factor = value;
  self->dynamic_state_dirty |= BIFROST_PIPELINE_DYNAMIC_DEPTH_BIAS;
}

void bfGfxCmdList_setMinSampleShading(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.min_sample_shading = value;
}

void bfGfxCmdList_setSampleMask(bfGfxCommandListHandle self, uint32_t sample_mask)
{
  self->pipeline_state.sample_mask = sample_mask;
}

void bfGfxCmdList_bindVertexDesc(bfGfxCommandListHandle self, bfVertexLayoutSetHandle vertex_set_layout)
{
  self->pipeline_state.vertex_set_layout = vertex_set_layout;
}

void bfGfxCmdList_bindVertexBuffers(bfGfxCommandListHandle self, uint32_t binding, bfBufferHandle* buffers, uint32_t num_buffers, const uint64_t* offsets)
{
  assert(num_buffers < BIFROST_GFX_BUFFERS_MAX_BINDING);

  self->pipeline_state.vertex_set_layout->bind();

  for (uint32_t i = 0; i < num_buffers; ++i)
  {
    assert(offsets[i] == 0 && "VBO Offsets not supported by the graphics backend.");

    glBindBuffer(buffers[i]->target, buffers[i]->handle);
    self->pipeline_state.vertex_set_layout->apply(buffers[i], binding + i, offsets[i]);
  }
}

void bfGfxCmdList_bindIndexBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, uint64_t offset, BifrostIndexType idx_type)
{
  self->index_type   = idx_type;
  self->index_offset = offset;

  glBindBuffer(buffer->target, buffer->handle);
}

void bfGfxCmdList_bindProgram(bfGfxCommandListHandle self, bfShaderProgramHandle shader)
{
  self->pipeline_state.program = shader;
}

void bfGfxCmdList_bindDescriptorSets(bfGfxCommandListHandle self, uint32_t binding, bfDescriptorSetHandle* desc_sets, uint32_t num_desc_sets)
{
  const bfShaderProgramHandle program = self->pipeline_state.program;

  glUseProgram(program->handle);

  for (uint32_t i = 0; i < num_desc_sets; ++i)
  {
    const int             index      = int(binding) + int(i);
    const int             tex_offset = program->set_info[index].texture_offset;
    bfDescriptorSetHandle desc_set   = desc_sets[i];

    for (const auto& texture : desc_set->textures)
    {
      glActiveTexture(GL_TEXTURE0 + tex_offset);
      glBindTexture(GL_TEXTURE_2D, texture.second->tex_image);
      glUniform1i(texture.first, tex_offset);
    }

    for (const auto& ubo : desc_set->ubos)
    {
      const uint32_t       ubo_binding = std::get<0>(ubo);
      const bfBufferSize   offset      = std::get<1>(ubo);
      const bfBufferSize   size        = std::get<2>(ubo);
      const bfBufferHandle buffer      = std::get<3>(ubo);

      // TODO: Check if this is needed.
      glBindBuffer(buffer->target, buffer->handle);

      glBindBufferRange(buffer->target, ubo_binding, buffer->handle, offset, size);
    }
  }
}

static std::uint64_t hash_bfDescriptorSetInfo(const bfDescriptorSetInfo* desc_set_info)
{
  std::uint64_t self = desc_set_info->num_bindings;

  for (uint32_t i = 0; i < desc_set_info->num_bindings; ++i)
  {
    const bfDescriptorElementInfo* binding = &desc_set_info->bindings[i];

    self = hash::addU32(self, binding->binding);
    self = hash::addU32(self, binding->array_element_start);
    self = hash::addU32(self, binding->num_handles);

    for (uint32_t j = 0; j < binding->num_handles; ++j)
    {
      self = hash::addPointer(self, binding->handles[i]);

      if (binding->type == BIFROST_DESCRIPTOR_ELEMENT_BUFFER)
      {
        self = hash::addU64(self, binding->offsets[j]);
        self = hash::addU64(self, binding->sizes[j]);
      }
    }
  }

  return self;
}

void bfGfxCmdList_bindDescriptorSet(bfGfxCommandListHandle self, uint32_t set_index, const bfDescriptorSetInfo* desc_set_info)
{
  const bfShaderProgramHandle program   = self->pipeline_state.program;
  const std::uint64_t         hash_code = hash_bfDescriptorSetInfo(desc_set_info);
  bfDescriptorSetHandle       desc_set  = self->parent->cache_descriptor_set.find(hash_code, *desc_set_info);

  if (!desc_set)
  {
    desc_set = bfShaderProgram_createDescriptorSet(program, set_index);

    for (uint32_t i = 0; i < desc_set_info->num_bindings; ++i)
    {
      const bfDescriptorElementInfo* binding_info = &desc_set_info->bindings[i];

      switch (binding_info->type)
      {
        case BIFROST_DESCRIPTOR_ELEMENT_TEXTURE:
          bfDescriptorSet_setCombinedSamplerTextures(
           desc_set,
           binding_info->binding,
           binding_info->array_element_start,
           (bfTextureHandle*)binding_info->handles,
           binding_info->num_handles);
          break;
        case BIFROST_DESCRIPTOR_ELEMENT_BUFFER:
          bfDescriptorSet_setUniformBuffers(
           desc_set,
           binding_info->binding,
           binding_info->offsets,
           binding_info->sizes,
           (bfBufferHandle*)binding_info->handles,
           binding_info->num_handles);
          break;
        case BIFROST_DESCRIPTOR_ELEMENT_BUFFER_VIEW:
        case BIFROST_DESCRIPTOR_ELEMENT_DYNAMIC_BUFFER:
        case BIFROST_DESCRIPTOR_ELEMENT_INPUT_ATTACHMENT:
        default:
          assert(!"Not supported yet.");
          break;
      }
    }

    bfDescriptorSet_flushWrites(desc_set);

    self->parent->cache_descriptor_set.insert(hash_code, desc_set, *desc_set_info);
    AddCachedResource(self->parent, &desc_set->super, hash_code);
  }

  bfGfxCmdList_bindDescriptorSets(self, set_index, &desc_set, 1);
  UpdateResourceFrame(self->context, &desc_set->super);
}

GLenum bfGLConvertBlendFactor(BifrostBlendFactor factor)
{
#define CONVERT(X)               \
  case BIFROST_BLEND_FACTOR_##X: \
    return GL_##X

  switch (factor)
  {
    CONVERT(ZERO);
    CONVERT(ONE);
    CONVERT(SRC_COLOR);
    CONVERT(ONE_MINUS_SRC_COLOR);
    CONVERT(DST_COLOR);
    CONVERT(ONE_MINUS_DST_COLOR);
    CONVERT(SRC_ALPHA);
    CONVERT(ONE_MINUS_SRC_ALPHA);
    CONVERT(DST_ALPHA);
    CONVERT(ONE_MINUS_DST_ALPHA);
    CONVERT(CONSTANT_COLOR);
    CONVERT(ONE_MINUS_CONSTANT_COLOR);
    CONVERT(CONSTANT_ALPHA);
    CONVERT(ONE_MINUS_CONSTANT_ALPHA);
    CONVERT(SRC_ALPHA_SATURATE);
#if !USE_OPENGL_ES_STANDARD
    CONVERT(SRC1_COLOR);
    CONVERT(ONE_MINUS_SRC1_COLOR);
    CONVERT(SRC1_ALPHA);
    CONVERT(ONE_MINUS_SRC1_ALPHA);
#endif
    default:
      break;
  }

#undef CONVERT

  assert(0);
  return GL_ZERO;
}

GLenum bfGLConvertBlendOp(BifrostBlendOp factor)
{
  switch (factor)
  {
    case BIFROST_BLEND_OP_ADD: return GL_FUNC_ADD;
    case BIFROST_BLEND_OP_SUB: return GL_FUNC_SUBTRACT;
    case BIFROST_BLEND_OP_REV_SUB: return GL_FUNC_REVERSE_SUBTRACT;
    case BIFROST_BLEND_OP_MIN: return GL_MIN;
    case BIFROST_BLEND_OP_MAX: return GL_MAX;
  }

  assert(0);
  return GL_FUNC_ADD;
}

static void flushPipeline(bfGfxCommandListHandle self)
{
  auto& state = self->pipeline_state;

  // Blending

  bfFramebufferBlending& blend = state.blending[0];

  const bool blend_enable = blend.color_blend_src != BIFROST_BLEND_FACTOR_NONE && blend.color_blend_dst != BIFROST_BLEND_FACTOR_NONE;

  if (blend_enable)
  {
    glEnable(GL_BLEND);

    glBlendFuncSeparate(
     bfGLConvertBlendFactor((BifrostBlendFactor)blend.color_blend_src),
     bfGLConvertBlendFactor((BifrostBlendFactor)blend.color_blend_dst),
     bfGLConvertBlendFactor((BifrostBlendFactor)blend.alpha_blend_src),
     bfGLConvertBlendFactor((BifrostBlendFactor)blend.alpha_blend_dst));
    glBlendEquationSeparate(
     bfGLConvertBlendOp((BifrostBlendOp)blend.color_blend_op),
     bfGLConvertBlendOp((BifrostBlendOp)blend.alpha_blend_op));
  }
  else
  {
    glDisable(GL_BLEND);
  }
}

void bfGfxCmdList_draw(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices)
{
  flushPipeline(self);

  glDrawArrays(
   bfConvertDrawMode((BifrostDrawMode)self->pipeline_state.state.draw_mode),
   first_vertex,
   num_vertices);
}

void bfGfxCmdList_drawInstanced(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices, uint32_t first_instance, uint32_t num_instances)
{
  // GLenum mode, GLint first, GLsizei count, GLsizei instancecount

  assert(first_instance == 0);

#if USE_OPENGL_ES_STANDARD
  assert(!"Not implemented on webgl");
#else
  glDrawArraysInstanced(bfConvertDrawMode((BifrostDrawMode)self->pipeline_state.state.draw_mode), first_vertex, num_vertices, num_instances);
#endif
}

void bfGfxCmdList_drawIndexed(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset)
{
  flushPipeline(self);

  const uint32_t index_size = self->index_type == BIFROST_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t);
  bfBufferHandle tmp_buffers[BIFROST_GFX_BUFFERS_MAX_BINDING];

  if (vertex_offset != 0)
  {
    const auto vertex_state = self->pipeline_state.vertex_set_layout;

    bfBufferCreateParams create_params = {{0, 0}, BIFROST_BUF_VERTEX_BUFFER};

    for (uint32_t i = 0; i < vertex_state->num_vertex_buffers; ++i)
    {
      const auto           vertex_stride       = vertex_state->vertex_bindings.find(vertex_state->vertex_buffers[i].second)->second.stride;
      const bfBufferHandle old_buffer          = vertex_state->vertex_buffers[i].first;
      const auto           vertex_offset_bytes = vertex_offset * vertex_stride;

      create_params.allocation.size = old_buffer->real_size - vertex_offset_bytes;
      tmp_buffers[i]                = bfGfxDevice_newBuffer(self->parent, &create_params);

      bfBuffer_copyGPU(
       old_buffer,
       vertex_offset_bytes,
       tmp_buffers[i],
       0,
       create_params.allocation.size);

      glBindBuffer(tmp_buffers[i]->target, tmp_buffers[i]->handle);
      vertex_state->apply(vertex_state->vertex_buffers[i].second, 0);
    }
  }

  glDrawElements(bfConvertDrawMode((BifrostDrawMode)self->pipeline_state.state.draw_mode),
                 num_indices,
                 self->index_type == BIFROST_INDEX_TYPE_UINT16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                 (const void*)(uintptr_t(index_offset * index_size) + self->index_offset));

  if (vertex_offset != 0)
  {
    const auto vertex_state = self->pipeline_state.vertex_set_layout;

    for (uint32_t i = 0; i < vertex_state->num_vertex_buffers; ++i)
    {
      const bfBufferHandle old_buffer = vertex_state->vertex_buffers[i].first;

      glBindBuffer(old_buffer->target, old_buffer->handle);
      vertex_state->apply(vertex_state->vertex_buffers[i].second, 0);

      bfGfxDevice_release(self->parent, tmp_buffers[i]);
    }
  }
}
void bfGfxCmdList_drawIndexedInstanced(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset, uint32_t first_instance, uint32_t num_instances)
{
  assert(!"Not implemented");
}

void bfGfxCmdList_executeSubCommands(bfGfxCommandListHandle self, bfGfxCommandListHandle* commands, uint32_t num_commands)
{
  assert(!"Not implemented");
}

void bfGfxCmdList_endRenderpass(bfGfxCommandListHandle self)
{
  /* NO-OP By Design */
}

void bfGfxCmdList_end(bfGfxCommandListHandle self)
{
  /* NO-OP By Design */
}

void bfGfxCmdList_updateBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size, const void* data)
{
  glBindBuffer(buffer->target, buffer->handle);
  glBufferSubData(buffer->target, offset, size, data);
}

void bfGfxCmdList_submit(bfGfxCommandListHandle self)
{
#if !USE_WEBGL_STANDARD
  bfWindowGL_swapBuffers(self->window->window);
#endif

  self->window->current_cmd_list = NULL;
  deallocate(self);
}

template<typename T, typename... Args>
static T* allocate(Args&&... args)
{
  return s_GraphicsMemory.allocateT<T>(std::forward<Args>(args)...);
}

template<typename T>
static void deallocate(T* ptr)
{
  s_GraphicsMemory.deallocateT(ptr);
}

static GLenum bfGLBufferUsageTarget(const bfBufferUsageBits usage)
{
  if (usage & BIFROST_BUF_VERTEX_BUFFER)
  {
    return GL_ARRAY_BUFFER;
  }

  if (usage & BIFROST_BUF_UNIFORM_BUFFER)
  {
    return GL_UNIFORM_BUFFER;
  }

#if !USE_OPENGL_ES_STANDARD
  if (usage & BIFROST_BUF_INDIRECT_BUFFER)
  {
    return GL_DRAW_INDIRECT_BUFFER;
  }
#endif

  if (usage & BIFROST_BUF_INDEX_BUFFER)
  {
    return GL_ELEMENT_ARRAY_BUFFER;
  }

  assert(false);
  return 0;

  /*
    BIFROST_BUF_TRANSFER_SRC         = (1 << 0), 
    BIFROST_BUF_TRANSFER_DST          = (1 << 1),
    BIFROST_BUF_UNIFORM_TEXEL_BUFFER = (1 << 2),
    BIFROST_BUF_STORAGE_TEXEL_BUFFER = (1 << 3),
    BIFROST_BUF_STORAGE_BUFFER       = (1 << 5),
  */
}

// mode(0) - draw
// mode(1) - read
// mode(2) - write
static GLenum bfGLBufferUsageHint(const bfBufferPropertyBits properties, int mode)
{
  const bool is_static = properties & BIFROST_BPF_DEVICE_LOCAL;

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
#if !USE_OPENGL_ES_STANDARD

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
#endif
    default:
    {
      assert(!"Invalid configuration");
      return GL_STREAM_DRAW;
    }
  }
}

static GLenum bfGLConvertShaderType(BifrostShaderType type)
{
  switch (type)
  {
    case BIFROST_SHADER_TYPE_VERTEX: return GL_VERTEX_SHADER;
#if !USE_OPENGL_ES_STANDARD
    case BIFROST_SHADER_TYPE_TESSELLATION_CONTROL: return GL_TESS_CONTROL_SHADER;
    case BIFROST_SHADER_TYPE_TESSELLATION_EVALUATION: return GL_TESS_EVALUATION_SHADER;
    case BIFROST_SHADER_TYPE_GEOMETRY: return GL_GEOMETRY_SHADER;
#endif
    case BIFROST_SHADER_TYPE_FRAGMENT: return GL_FRAGMENT_SHADER;
#if !USE_OPENGL_ES_STANDARD
    case BIFROST_SHADER_TYPE_COMPUTE: return GL_COMPUTE_SHADER;
#endif
    default:
      assert(!"Invalid shader type.");
      return 0;
  }
}

static int bfGLVertexFormatNumComponents(const BifrostVertexFormatAttribute format)
{
  switch (format)
  {
    case BIFROST_VFA_UINT32_4:
    case BIFROST_VFA_SINT32_4:
    case BIFROST_VFA_USHORT16_4:
    case BIFROST_VFA_UCHAR8_4:
    case BIFROST_VFA_UCHAR8_4_UNORM:
    case BIFROST_VFA_FLOAT32_4:
    case BIFROST_VFA_SSHORT16_4:
    case BIFROST_VFA_SCHAR8_4:
      return 4;

    case BIFROST_VFA_FLOAT32_3:
    case BIFROST_VFA_SCHAR8_3:
    case BIFROST_VFA_UINT32_3:
    case BIFROST_VFA_SINT32_3:
    case BIFROST_VFA_SSHORT16_3:
    case BIFROST_VFA_USHORT16_3:
    case BIFROST_VFA_UCHAR8_3:
      return 3;

    case BIFROST_VFA_SINT32_2:
    case BIFROST_VFA_FLOAT32_2:
    case BIFROST_VFA_UINT32_2:
    case BIFROST_VFA_USHORT16_2:
    case BIFROST_VFA_SSHORT16_2:
    case BIFROST_VFA_UCHAR8_2:
    case BIFROST_VFA_SCHAR8_2:
      return 2;

    case BIFROST_VFA_SCHAR8_1:
    case BIFROST_VFA_SSHORT16_1:
    case BIFROST_VFA_UINT32_1:
    case BIFROST_VFA_FLOAT32_1:
    case BIFROST_VFA_SINT32_1:
    case BIFROST_VFA_USHORT16_1:
    case BIFROST_VFA_UCHAR8_1:
      return 1;

    default:
      assert(false);
      return -1;
  }
}

static GLenum bfGLVertexFormatType(const BifrostVertexFormatAttribute format)
{
  switch (format)
  {
    case BIFROST_VFA_FLOAT32_4:
    case BIFROST_VFA_FLOAT32_3:
    case BIFROST_VFA_FLOAT32_2:
    case BIFROST_VFA_FLOAT32_1:
      return GL_FLOAT;

    case BIFROST_VFA_UINT32_4:
    case BIFROST_VFA_UINT32_3:
    case BIFROST_VFA_UINT32_2:
    case BIFROST_VFA_UINT32_1:
      return GL_UNSIGNED_INT;

    case BIFROST_VFA_SINT32_4:
    case BIFROST_VFA_SINT32_3:
    case BIFROST_VFA_SINT32_2:
    case BIFROST_VFA_SINT32_1:
      return GL_INT;

    case BIFROST_VFA_USHORT16_4:
    case BIFROST_VFA_USHORT16_3:
    case BIFROST_VFA_USHORT16_2:
    case BIFROST_VFA_USHORT16_1:
      return GL_UNSIGNED_SHORT;

    case BIFROST_VFA_SSHORT16_4:
    case BIFROST_VFA_SSHORT16_3:
    case BIFROST_VFA_SSHORT16_2:
    case BIFROST_VFA_SSHORT16_1:
      return GL_SHORT;

    case BIFROST_VFA_UCHAR8_4:
    case BIFROST_VFA_UCHAR8_3:
    case BIFROST_VFA_UCHAR8_2:
    case BIFROST_VFA_UCHAR8_1:
      return GL_UNSIGNED_BYTE;

    case BIFROST_VFA_SCHAR8_4:
    case BIFROST_VFA_SCHAR8_3:
    case BIFROST_VFA_SCHAR8_2:
    case BIFROST_VFA_SCHAR8_1:
      return GL_BYTE;

    case BIFROST_VFA_UCHAR8_4_UNORM:
      return GL_UNSIGNED_BYTE;

    default:
      assert(false);
      return -1;
  }
}

static GLint bfGLConvertSamplerAddressMode(BifrostSamplerAddressMode sampler_mode)
{
  switch (sampler_mode)
  {
    case BIFROST_SAM_REPEAT: return GL_REPEAT;
    case BIFROST_SAM_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
    case BIFROST_SAM_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;

#if !USE_OPENGL_ES_STANDARD
    case BIFROST_SAM_CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
    case BIFROST_SAM_MIRROR_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE;
#endif

    default:
      assert(false);
      return -1;
  }
}

static GLint bfConvertSamplerFilterMode(BifrostSamplerFilterMode filter_mode)
{
  switch (filter_mode)
  {
    case BIFROST_SFM_NEAREST: return GL_NEAREST;
    case BIFROST_SFM_LINEAR: return GL_LINEAR;
    default:
      assert(false);
      return -1;
  }
}

static bool bfTextureIsDepthStencil(bfTextureHandle texture)
{
  return texture->flags & (BIFROST_TEX_IS_DEPTH_ATTACHMENT | BIFROST_TEX_IS_STENCIL_ATTACHMENT);
}

static bool bfTextureCanBeInput(bfTextureHandle texture)
{
  return texture->flags & (BIFROST_TEX_IS_SAMPLED | BIFROST_TEX_IS_INPUT_ATTACHMENT);
}

static GLenum bfConvertDrawMode(BifrostDrawMode draw_mode)
{
  switch (draw_mode)
  {
    case BIFROST_DRAW_MODE_POINT_LIST: return GL_POINTS;
    case BIFROST_DRAW_MODE_LINE_LIST: return GL_LINES;
    case BIFROST_DRAW_MODE_LINE_STRIP: return GL_LINE_STRIP;
    case BIFROST_DRAW_MODE_TRIANGLE_LIST: return GL_TRIANGLES;
    case BIFROST_DRAW_MODE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
    case BIFROST_DRAW_MODE_TRIANGLE_FAN: return GL_TRIANGLE_FAN;
    default:
      assert(false);
      return -1;
  }
}

static GLenum bfConvertFrontFace(BifrostFrontFace face)
{
  switch (face)
  {
    case BIFROST_FRONT_FACE_CCW: return GL_CCW;
    case BIFROST_FRONT_FACE_CW: return GL_CW;
    default:
      assert(false);
      return -1;
  }
}

static GLenum bfGLConvertCmpOp(BifrostCompareOp op)
{
  switch (op)
  {
    case BIFROST_COMPARE_OP_NEVER: return GL_NEVER;
    case BIFROST_COMPARE_OP_LESS_THAN: return GL_LESS;
    case BIFROST_COMPARE_OP_EQUAL: return GL_EQUAL;
    case BIFROST_COMPARE_OP_LESS_OR_EQUAL: return GL_LEQUAL;
    case BIFROST_COMPARE_OP_GREATER: return GL_GREATER;
    case BIFROST_COMPARE_OP_NOT_EQUAL: return GL_NOTEQUAL;
    case BIFROST_COMPARE_OP_GREATER_OR_EQUAL: return GL_GEQUAL;
    case BIFROST_COMPARE_OP_ALWAYS: return GL_ALWAYS;
    default:
      assert(false);
      return -1;
  }
}

inline bool ComparebfPipelineCache::operator()(const bfPipelineCache& a, const bfPipelineCache& b) const
{
  if (a.program != b.program)
  {
    return false;
  }

  // TODO: Check if this is strictly required.
  if (a.renderpass != b.renderpass)
  {
    return false;
  }

  if (a.vertex_set_layout != b.vertex_set_layout)
  {
    return false;
  }

  std::uint64_t state_bits[4];

  std::memcpy(state_bits + 0, &a.state, sizeof(a.state));
  std::memcpy(state_bits + 2, &b.state, sizeof(b.state));

  state_bits[0] &= bfPipelineCache_state0Mask(&a.state);
  state_bits[1] &= bfPipelineCache_state1Mask(&a.state);
  state_bits[2] &= bfPipelineCache_state0Mask(&b.state);
  state_bits[3] &= bfPipelineCache_state1Mask(&b.state);

  if (std::memcmp(state_bits + 0, state_bits + 2, sizeof(a.state)) != 0)
  {
    return false;
  }

  if (!a.state.dynamic_viewport)
  {
    if (std::memcmp(&a.viewport, &b.viewport, sizeof(a.viewport)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_scissor)
  {
    if (std::memcmp(&a.scissor_rect, &b.scissor_rect, sizeof(a.scissor_rect)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_blend_constants)
  {
    if (std::memcmp(a.blend_constants, b.blend_constants, sizeof(a.blend_constants)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_line_width)
  {
    if (std::memcmp(&a.line_width, &b.line_width, sizeof(a.line_width)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_depth_bias)
  {
    if (a.depth.bias_constant_factor != b.depth.bias_constant_factor)
    {
      return false;
    }

    if (a.depth.bias_clamp != b.depth.bias_clamp)
    {
      return false;
    }

    if (a.depth.bias_slope_factor != b.depth.bias_slope_factor)
    {
      return false;
    }
  }

  if (!a.state.dynamic_depth_bounds)
  {
    if (a.depth.min_bound != b.depth.min_bound)
    {
      return false;
    }

    if (a.depth.max_bound != b.depth.max_bound)
    {
      return false;
    }
  }

  if (a.min_sample_shading != b.min_sample_shading)
  {
    return false;
  }

  if (a.sample_mask != b.sample_mask)
  {
    return false;
  }

  if (a.subpass_index != b.subpass_index)
  {
    return false;
  }

  if (a.subpass_index != b.subpass_index)
  {
    return false;
  }

  const auto num_attachments_a = a.renderpass->info.subpasses[a.subpass_index].num_out_attachment_refs;
  const auto num_attachments_b = b.renderpass->info.subpasses[b.subpass_index].num_out_attachment_refs;

  if (num_attachments_a != num_attachments_b)
  {
    return false;
  }

  for (std::uint32_t i = 0; i < num_attachments_a; ++i)
  {
    uint32_t blend_state_bits_a;
    uint32_t blend_state_bits_b;

    std::memcpy(&blend_state_bits_a, &a.blending[i], sizeof(uint32_t));
    std::memcpy(&blend_state_bits_b, &b.blending[i], sizeof(uint32_t));

    if (blend_state_bits_a != blend_state_bits_b)
    {
      return false;
    }
  }

  return true;
}

#if !USE_WEBGL_STANDARD
#include <glad/glad.c>
#endif
