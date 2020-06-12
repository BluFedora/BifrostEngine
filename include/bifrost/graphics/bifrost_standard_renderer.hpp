/*!
 * @file   bifrost_standard_renderer.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   This is the reference renderer that all more specific
 *   renderers should look to for how the layout of
 *   graphics resources are expected to be.
 *
 *   References:
 *     [https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/]
 *     [https://www.derschmale.com/2014/01/26/reconstructing-positions-from-the-depth-buffer/]
 *     [http://ogldev.atspace.co.uk/www/tutorial46/tutorial46.html]
 *
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_STANDARD_RENDERER_HPP
#define BIFROST_STANDARD_RENDERER_HPP

#include "bifrost/bifrost_math.hpp"                           /* Vec3f, Vec2f, bfColor4u */
#include "bifrost/data_structures/bifrost_array.hpp"          /* Array<T>                */
#include "bifrost/data_structures/bifrost_intrusive_list.hpp" /* List<T>                 */
#include "bifrost_gfx_api.h"                                  /* Bifrost C Gfx API       */
#include "bifrost_glsl_compiler.hpp"                          /* GLSLCompiler            */

class BifrostEngine;

namespace bifrost
{
  class Light;
  class Material;
  class Entity;

  struct VertexPos2D final
  {
    Vec2f pos;
  };

  struct VertexPos2DUV final
  {
    Vec2f pos;
    Vec2f uv;
  };

  struct VertexPos3DColor4u final
  {
    Vector3f  pos;
    bfColor4u color;
  };

  struct StandardVertex final
  {
    Vector3f  pos;
    Vector3f  normal;
    Vector3f  tangent;
    bfColor4u color;
    Vec2f     uv;
  };

  // -------------------------------------------------------- //
  //                                                          //
  // Standard Shader Layout:                                  //
  //                                                          //
  // descriptor_set0                                          //
  // {                                                        //
  //   (binding = 0) mat4x4 u_CameraProjection;               //
  //   (binding = 0) mat4x4 u_CameraViewProjection;           //
  //   (binding = 0) mat4x4 u_CameraView;                     //
  //   (binding = 0) vec3   u_CameraPosition;                 //
  // }                                                        //
  //                                                          //
  // descriptor_set1                                          //
  // {                                                        //
  //   (binding = 0) vec3 u_LightColor;                       //
  //   (binding = 1) vec3 u_LightPosition;                    //
  //   (binding = 2) vec3 u_LightDirection;                   //
  // }                                                        //
  //                                                          //
  // descriptor_set2                                          //
  // {                                                        //
  //   (binding = 0) sampler2D u_AlbedoTexture;               //
  //   (binding = 1) sampler2D u_NormalTexture;               //
  //   (binding = 2) sampler2D u_MetallicTexture;             //
  //   (binding = 3) sampler2D u_RoughnessTexture;            //
  //   (binding = 4) sampler2D u_AmbientOcclusionTexture;     //
  // }                                                        //
  //                                                          //
  // descriptor_set3                                          //
  // {                                                        //
  //   (binding = 0) mat4x4 u_ModelTransform;                 //
  //   (binding = 0) mat4x4 u_ModelView;                      //
  //   (binding = 0) mat4x4 u_NormalModelView;                // transpose(M^-1) * Nobject
  // }                                                        //
  //                                                          //
  // -------------------------------------------------------- //
  //                                                          //
  // Standard GBuffer Layout:                                 //
  //   This Engine Uses View Space Lighting Calculations      //
  //                                                          //
  // GBuffer0 [normal.x, normal.y, roughness, metallic      ] // (BIFROST_IMAGE_FORMAT_R16G16B16A16_UNORM)
  // GBuffer2 [albedo.r, albedo.g, albedo.b,  ao            ] // (BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM)
  // DS       [depth 24, stencil 8                          ] // (BIFROST_IMAGE_FORMAT_D24_UNORM_S8_UINT)
  //                                                          //
  // -------------------------------------------------------- //
  //
  // Pipeline:
  //   (in: VertexData + Material Textures) -> GBuffer -> (out: View Space Buffers)
  //   (in: ) ->  SSAO -> (out: )
  //
  // -------------------------------------------------------- //

  //
  // Constants
  //

  static constexpr int k_GfxCameraSetIndex               = 0;
  static constexpr int k_GfxLightSetIndex                = 1;
  static constexpr int k_GfxMaterialSetIndex             = 2;
  static constexpr int k_GfxObjectSetIndex               = 3;
  static constexpr int k_GfxNumGBufferAttachments        = 2;
  static constexpr int k_GfxNumSSAOBufferAttachments     = 2;
  static constexpr int k_GfxSSAOKernelSize               = 128; /* Matches the constant defined in "assets/shaders/standard/ssao.frag.glsl"                                    */
  static constexpr int k_GfxSSAONoiseTextureDim          = 4;   /* Matches the constant defined in "assets/shaders/standard/ssao_blur.frag.glsl"                               */
  static constexpr int k_GfxMaxPunctualLightsOnScreen    = 512; /* (Techincally x2 this value) Matches the constant defined in "assets/shaders/standard/pbr_lighting.frag.gls" */
  static constexpr int k_GfxMaxDirectionalLightsOnScreen = 16;  /* Matches the constant defined in "assets/shaders/standard/pbr_lighting.frag.gls"                             */
  static constexpr int k_GfxSSAONoiseTextureNumElements  = k_GfxSSAONoiseTextureDim * k_GfxSSAONoiseTextureDim;
  static constexpr int k_GfxMaxLightsOnScreen            = k_GfxMaxPunctualLightsOnScreen + k_GfxMaxDirectionalLightsOnScreen;
  static const int     k_GfxBaseResolution[]             = {1600, 900};

  //
  // Struct Mappings
  //

  struct LightGPUData final
  {
    bfColor4f color;                          //!< [RGB, Intensity]
    Vec3f     direction_and_inv_radius_pow2;  //!< [Direction, (1.0 / radius)^2]
    Vec3f     position_and_spot_scale;        //!< [Position, 1.0 / max(cos(inner_angle) - cos(outer_angle), k_Epsilon)]
    float     spot_offset;                    //!< [-cos(outer_angle) * spot_scale]
    float     _padd[3];                       //!< Padding for 16 byte alignment
  };

  //
  // Uniform Mappings
  //

  struct CameraUniformData final
  {
    Mat4x4   u_CameraProjection;
    Mat4x4   u_CameraInvViewProjection;
    Mat4x4   u_CameraViewProjection;
    Mat4x4   u_CameraView;
    Vector3f u_CameraForwardAndTime;  // [u_CameraForward, u_Time]
    Vector3f u_CameraPosition;        // [u_CameraPosition, u_Pad0]
    Vector3f u_CameraAmbient;
  };

  struct ObjectUniformData final
  {
    Mat4x4 u_ModelViewProjection;
    Mat4x4 u_Model;
    Mat4x4 u_NormalModel;
  };

  struct SSAOKernelUnifromData final
  {
    Vector3f u_Kernel[k_GfxSSAOKernelSize];
    float    u_SampleRadius;
    float    u_SampleBias;
  };

  template<std::size_t MaxLights>
  struct BaseLightUniformData final
  {
    LightGPUData u_Lights[MaxLights];
    int          u_NumLights;
  };

  using DirectionalLightUniformData = BaseLightUniformData<k_GfxMaxDirectionalLightsOnScreen>;
  using PunctualLightUniformData    = BaseLightUniformData<k_GfxMaxPunctualLightsOnScreen>;

  //
  // Pipeline Buffers
  //

  struct GBuffer final
  {
    // NOTE(Shareef):
    //   Don't mess with the layout of this struct unless you change the way
    //   the [GBuffer::attachments] function works or all uses of it.
    bfTextureHandle   color_attachments[k_GfxNumGBufferAttachments];
    bfTextureHandle   depth_attachment;
    BifrostClearValue clear_values[k_GfxNumGBufferAttachments + 1];

    void init(bfGfxDeviceHandle device, int width, int height);
    void setupAttachments(bfRenderpassInfo& renderpass_info, uint16_t subpass_index = 0);
    void deinit(bfGfxDeviceHandle device);

    bfTextureHandle* attachments() { return color_attachments; }
  };

  struct SSAOBuffer final
  {
    bfTextureHandle   color_attachments[k_GfxNumSSAOBufferAttachments]; /* normal, blurred */
    BifrostClearValue clear_values[k_GfxNumSSAOBufferAttachments];
    bfTextureHandle   noise;
    bfBufferHandle    kernel_uniform;

    void init(bfGfxDeviceHandle device, int width, int height);
    void setupAttachments(bfRenderpassInfo& renderpass_info, uint16_t ao_subpass_index, int color_attachment_idx);
    void deinit(bfGfxDeviceHandle device);
  };

  struct BaseMultiBuffer
  {
    bfBufferHandle handle;
    bfBufferSize   element_aligned_size;
    bfBufferSize   total_size;

   protected:
    void create(bfGfxDeviceHandle device, bfBufferUsageBits usage, const bfGfxFrameInfo& info, size_t element_size, size_t element_alignment);
    void destroy(bfGfxDeviceHandle device) const;
  };

  template<typename T>
  struct MultiBuffer final : private BaseMultiBuffer
  {
    void create(bfGfxDeviceHandle device, bfBufferUsageBits usage, const bfGfxFrameInfo& info, size_t element_alignment)
    {
      BaseMultiBuffer::create(device, usage, info, elementSize(), element_alignment);
    }

    using BaseMultiBuffer::destroy;

    bfBufferSize        offset(const bfGfxFrameInfo& info) const { return info.frame_index * element_aligned_size; }
    bfBufferHandle&     handle() { return BaseMultiBuffer::handle; }
    static bfBufferSize elementSize() { return sizeof(T); }
    bfBufferSize        elementAlignedSize() const { return element_aligned_size; }
    bfBufferSize        totalSize() const { return total_size; }

    T* currentElement(const bfGfxFrameInfo& info)
    {
      return reinterpret_cast<T*>(static_cast<char*>(bfBuffer_mappedPtr(handle())) + offset(info));
    }

    T* currentElement()
    {
      return reinterpret_cast<T*>(static_cast<char*>(bfBuffer_mappedPtr(handle())));
    }

    void flushCurrent(const bfGfxFrameInfo& info)
    {
      bfBuffer_flushRange(handle(), offset(info), elementAlignedSize());
    }
  };

  //
  // Misc
  //

  struct Renderable final
  {
    bfBufferHandle transform_uniform;

    void create(bfGfxDeviceHandle device);
    void destroy(bfGfxDeviceHandle device) const;
  };

  //
  // Main Renderer
  //

  namespace LightShaders
  {
    enum
    {
      DIR,
      POINT,
      SPOT,
      MAX,
    };
  }

  struct CameraGPUData final
  {
    GBuffer                        geometry_buffer;
    SSAOBuffer                     ssao_buffer;
    bfTextureHandle                composite_buffer;
    MultiBuffer<CameraUniformData> camera_uniform_buffer;
    Mat4x4                         view_projection_cache;

    void init(bfGfxDeviceHandle device, bfGfxFrameInfo frame_info, int initial_width, int initial_height);
    void updateBuffers(BifrostCamera& camera, const bfGfxFrameInfo& frame_info, float global_time, const Vector3f& ambient);
    void bindDescriptorSet(bfGfxCommandListHandle command_list, const bfGfxFrameInfo& frame_info);  // Has to be called after a shader program change.
    void resize(bfGfxDeviceHandle device, int width, int height);
    void deinit(bfGfxDeviceHandle device);
  };

  struct StdPairHash
  {
   public:
    template<typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const
    {
      return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
  };

  class StandardRenderer final
  {
    friend class ::BifrostEngine;  // TODO: Not do this?!?!

   public:
    Vector3f AmbientColor = {0.03f};

   private:
    using CameraObjectPair = std::pair<const CameraGPUData*, Entity*>;

   private:
    GLSLCompiler                                              m_GLSLCompiler;
    bfGfxContextHandle                                        m_GfxBackend;
    bfGfxDeviceHandle                                         m_GfxDevice;
    bfGfxFrameInfo                                            m_FrameInfo;
    bfVertexLayoutSetHandle                                   m_StandardVertexLayout;
    bfVertexLayoutSetHandle                                   m_EmptyVertexLayout;
    bfGfxCommandListHandle                                    m_MainCmdList;
    bfTextureHandle                                           m_MainSurface;
    bfShaderProgramHandle                                     m_GBufferShader;
    bfShaderProgramHandle                                     m_SSAOBufferShader;
    bfShaderProgramHandle                                     m_SSAOBlurShader;
    bfShaderProgramHandle                                     m_AmbientLighting;
    bfShaderProgramHandle                                     m_LightShaders[LightShaders::MAX];
    List<Renderable>                                          m_RenderablePool;
    HashTable<CameraObjectPair, Renderable*, 64, StdPairHash> m_RenderableMapping;  // TODO: Make this per Scene.
    Array<bfGfxBaseHandle>                                    m_AutoRelease;
    bfTextureHandle                                           m_WhiteTexture;
    MultiBuffer<DirectionalLightUniformData>                  m_DirectionalLightBuffer;
    MultiBuffer<PunctualLightUniformData>                     m_PunctualLightBuffers[2];  // [Point, Spot]
    float                                                     m_GlobalTime;

   public:
    explicit StandardRenderer(IMemoryManager& memory);

    bfGfxContextHandle      context() const { return m_GfxBackend; }
    bfGfxDeviceHandle       device() const { return m_GfxDevice; }
    bfVertexLayoutSetHandle standardVertexLayout() const { return m_StandardVertexLayout; }
    bfGfxCommandListHandle  mainCommandList() const { return m_MainCmdList; }
    bfTextureHandle         surface() const { return m_MainSurface; }
    GLSLCompiler&           glslCompiler() { return m_GLSLCompiler; }

    void               init(const bfGfxContextCreateParams& gfx_create_params);
    [[nodiscard]] bool frameBegin();
    void               bindMaterial(bfGfxCommandListHandle command_list, const Material& material);
    void               bindObject(bfGfxCommandListHandle command_list, const CameraGPUData& camera, Entity& entity);
    void               addLight(Light& light);
    void               beginGBufferPass(CameraGPUData& camera) const;
    void               beginSSAOPass(CameraGPUData& camera) const;
    void               beginLightingPass(CameraGPUData& camera);
    void               beginScreenPass() const;
    void               endPass() const;
    void               frameEnd() const;
    void               deinit();

    template<typename FGBufferPass, typename FLightOverlayPass>
    void renderCameraTo(BifrostCamera& camera, CameraGPUData& camera_gpu_data, FGBufferPass&& gbuffer_callback, FLightOverlayPass&& overlay_callback)
    {
      camera_gpu_data.updateBuffers(camera, m_FrameInfo, m_GlobalTime, AmbientColor);

      // GBuffer
      beginGBufferPass(camera_gpu_data);
      gbuffer_callback();
      endPass();

      // SSAO
      beginSSAOPass(camera_gpu_data);
      endPass();

      // Lighting
      beginLightingPass(camera_gpu_data);
      overlay_callback();
      endPass();
    }

   private:
    void initShaders();
    void deinitShaders();
  };

  //
  // Helpers for the Verbose C API
  //
  namespace gfx
  {
    bfTextureHandle createAttachment(
     bfGfxDeviceHandle                 device,
     const bfTextureCreateParams&      create_params,
     const bfTextureSamplerProperties& sampler);
    bfTextureHandle createTexture(
     bfGfxDeviceHandle                 device,
     const bfTextureCreateParams&      create_params,
     const bfTextureSamplerProperties& sampler,
     const void*                       data,
     std::size_t                       data_size);
    bfShaderProgramHandle createShaderProgram(
     bfGfxDeviceHandle    device,
     std::uint32_t        num_desc_sets,
     bfShaderModuleHandle vertex_module,
     bfShaderModuleHandle fragment_module,
     const char*          debug_name = nullptr);
  }  // namespace gfx
}  // namespace bifrost

#endif /* BIFROST_STANDARD_RENDERER_HPP */
