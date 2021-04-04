/*!
 * @file   bf_standard_renderer.hpp
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
 * @copyright Copyright (c) 2020-2021
 */
#ifndef BF_STANDARD_RENDERER_HPP
#define BF_STANDARD_RENDERER_HPP

#include "bf/PlatformFwd.h"                              /* bfWindow                */
#include "bf/bf_gfx_api.h"                               /* bf C Gfx API            */
#include "bf/bifrost_math.hpp"                           /* Vec3f, Vec2f, bfColor4u */
#include "bf/data_structures/bifrost_array.hpp"          /* Array<T>                */
#include "bf/data_structures/bifrost_intrusive_list.hpp" /* List<T>                 */
#include "bifrost_glsl_compiler.hpp"                     /* GLSLCompiler            */

namespace bf
{
  //
  // Constants
  //

  static constexpr int         k_GfxCameraSetIndex               = 0;
  static constexpr int         k_GfxLightSetIndex                = 1;
  static constexpr int         k_GfxMaterialSetIndex             = 2;
  static constexpr int         k_GfxObjectSetIndex               = 3;
  static constexpr int         k_GfxNumGBufferAttachments        = 2;
  static constexpr int         k_GfxNumSSAOBufferAttachments     = 2;
  static constexpr int         k_GfxSSAOKernelSize               = 128; /* Matches the constant defined in "assets/shaders/standard/ssao.frag.glsl"                                    */
  static constexpr int         k_GfxSSAONoiseTextureDim          = 4;   /* Matches the constant defined in "assets/shaders/standard/ssao_blur.frag.glsl"                               */
  static constexpr int         k_GfxMaxPunctualLightsOnScreen    = 512; /* (Technically x2 this value) Matches the constant defined in "assets/shaders/standard/pbr_lighting.frag.gls" */
  static constexpr int         k_GfxMaxDirectionalLightsOnScreen = 16;  /* Matches the constant defined in "assets/shaders/standard/pbr_lighting.frag.glsl"                            */
  static constexpr int         k_GfxSSAONoiseTextureNumElements  = k_GfxSSAONoiseTextureDim * k_GfxSSAONoiseTextureDim;
  static constexpr int         k_GfxMaxLightsOnScreen            = k_GfxMaxPunctualLightsOnScreen + k_GfxMaxDirectionalLightsOnScreen;
  static constexpr std::size_t k_GfxMaxVertexBones               = 4;
  static constexpr std::size_t k_GfxMaxTotalBones                = 128;

  //
  // Forward Declarations
  //

  struct RenderView;
  class Light;
  class MaterialAsset;
  class Engine;
  class Entity;

  //
  // Simple Structs
  //

  struct StandardVertex
  {
    Vector3f  pos;
    Vector3f  normal;
    Vector3f  tangent;
    bfColor4u color;
    Vector2f  uv;
  };

  struct VertexBoneData
  {
    std::uint8_t bone_idx[k_GfxMaxVertexBones];
    float        bone_weights[k_GfxMaxVertexBones];
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
  //   (binding = 0) mat4x4 u_NormalModelView;                //
  // }                                                        //
  //                                                          //
  // -------------------------------------------------------- //
  //                                                          //
  // Standard GBuffer Layout:                                 //
  //   This Engine Uses View Space Lighting Calculations      //
  //                                                          //
  // GBuffer0 [normal.x, normal.y, roughness, metallic      ] // (BF_IMAGE_FORMAT_R16G16B16A16_UNORM)
  // GBuffer2 [albedo.r, albedo.g, albedo.b,  ao            ] // (BF_IMAGE_FORMAT_R8G8B8A8_UNORM)
  // DS       [depth 24, stencil 8                          ] // (BF_IMAGE_FORMAT_D24_UNORM_S8_UINT)
  //                                                          //
  // -------------------------------------------------------- //
  //
  // Pipeline:
  //   (in: VertexData + Material Textures) -> GBuffer -> (out: View Space Buffers)
  //   (in: ) ->  SSAO -> (out: )
  //
  // -------------------------------------------------------- //

  //
  // Shader Struct Mappings
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
  // Shader Uniform Mappings
  //

  struct CameraUniformData final
  {
    Mat4x4   u_CameraProjection;
    Mat4x4   u_CameraInvViewProjection;
    Mat4x4   u_CameraViewProjection;
    Mat4x4   u_CameraView;
    Vector3f u_CameraForwardAndTime;  // [u_CameraForward, u_Time]
    Vector3f u_CameraPosition;        // [u_CameraPosition, u_CameraAspect]
    Vector3f u_CameraAmbient;
  };

  struct CameraOverlayUniformData final
  {
    Mat4x4 u_CameraProjection;
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
    bfTextureHandle color_attachments[k_GfxNumGBufferAttachments];
    bfTextureHandle depth_attachment;
    bfClearValue    clear_values[k_GfxNumGBufferAttachments + 1];

    void init(bfGfxDeviceHandle device, int width, int height);
    void setupAttachments(bfRenderpassInfo& renderpass_info, uint16_t subpass_index = 0);
    void deinit(bfGfxDeviceHandle device);

    bfTextureHandle* attachments() { return color_attachments; }
  };

  struct SSAOBuffer final
  {
    bfTextureHandle color_attachments[k_GfxNumSSAOBufferAttachments]; /* normal, blurred */
    bfClearValue    clear_values[k_GfxNumSSAOBufferAttachments];
    bfTextureHandle noise;
    bfBufferHandle  kernel_uniform;

    void init(bfGfxDeviceHandle device, int width, int height);
    void setupAttachments(bfRenderpassInfo& renderpass_info, uint16_t ao_subpass_index, int color_attachment_idx);
    void deinit(bfGfxDeviceHandle device);
  };

  // This class if good for non resizing allocations that need to be safe across frames.
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
      flushCurrent(info, elementAlignedSize());
    }

    void flushCurrent(const bfGfxFrameInfo& info, bfBufferSize size)
    {
      bfBuffer_flushRange(handle(), offset(info), size);
    }
  };

  //
  // Misc
  //

  template<typename TUniformData>
  struct Renderable final
  {
    MultiBuffer<TUniformData> transform_uniform;

    void create(bfGfxDeviceHandle device, const bfGfxFrameInfo& info)
    {
      const auto limits = bfGfxDevice_limits(device);

      transform_uniform.create(device, BF_BUFFER_USAGE_UNIFORM_BUFFER | BF_BUFFER_USAGE_PERSISTENTLY_MAPPED_BUFFER, info, limits.uniform_buffer_offset_alignment);
    }

    void destroy(bfGfxDeviceHandle device) const
    {
      transform_uniform.destroy(device);
    }
  };

  // This class is good for largely varying growing buffers with out the need to realloc a buffer.
  // The cost of doing it this way is batching becomes more complex since this is a linked list of separate buffers.
  //
  // T - Vertex Type
  //
  template<typename T, std::size_t NumVerticesPerBatch, bfBufferUsageBits k_Usage>
  struct GfxLinkedBuffer final
  {
    using TArray = T[NumVerticesPerBatch];

    struct Link final
    {
      MultiBuffer<TArray> gpu_buffer;
      Link*               next;
      int                 vertices_left;

      T* currentVertex()
      {
        return &(*gpu_buffer.currentElement())[numVertices()];
      }

      int numVertices() const
      {
        return NumVerticesPerBatch - vertices_left;
      }

      void bind(bfGfxCommandListHandle command_list, const bfGfxFrameInfo& frame_info)
      {
        const bfBufferSize offset = gpu_buffer.offset(frame_info);

        bfGfxCmdList_bindVertexBuffers(command_list, 0, &gpu_buffer.handle(), 1, &offset);
      }
    };

    bfGfxDeviceHandle gfx_device;
    Link*             free_list;
    Array<Link*>      used_buffers;

    explicit GfxLinkedBuffer(IMemoryManager& memory_manager) :
      gfx_device{nullptr},
      free_list{nullptr},
      used_buffers{memory_manager}
    {
    }

    void init(bfGfxDeviceHandle device)
    {
      gfx_device = device;
    }

    void clear()
    {
      for (Link* const link : used_buffers)
      {
        link->next = free_list;
        free_list  = link;
      }

      used_buffers.clear();
    }

    // Pointer and offset is returned.
    std::pair<T*, int> requestVertices(const bfGfxFrameInfo& frame_info, int vertices)
    {
      assert(vertices < NumVerticesPerBatch && "Could not handle this amount of vetices in one batch.");

      if (used_buffers.isEmpty() || used_buffers.back()->vertices_left < vertices)
      {
        Link* new_link = grabFreeLink(gfx_device, frame_info);

        used_buffers.push(new_link);

        bfBuffer_map(new_link->gpu_buffer.handle(), new_link->gpu_buffer.offset(frame_info), new_link->gpu_buffer.elementAlignedSize());
      }

      Link* buffer_link = used_buffers.back();

      T* data = buffer_link->currentVertex();

      const int offset = buffer_link->numVertices();

      buffer_link->vertices_left -= vertices;

      return {data, offset};
    }

    Link* currentLink() const
    {
      return used_buffers.back();
    }

    void flushLinks(const bfGfxFrameInfo& frame_info)
    {
      for (Link* const link : used_buffers)
      {
        link->gpu_buffer.flushCurrent(frame_info);
        bfBuffer_unMap(link->gpu_buffer.handle());
      }
    }

    void deinit()
    {
      clear();

      while (free_list)
      {
        Link* const next = free_list->next;
        free_list->gpu_buffer.destroy(gfx_device);
        memory().deallocateT(free_list);
        free_list = next;
      }
    }

   private:
    Link* grabFreeLink(bfGfxDeviceHandle gfx_device, const bfGfxFrameInfo& frame_info)
    {
      Link* result = free_list;

      if (!result)
      {
        result = memory().allocateT<Link>();
        result->gpu_buffer.create(
         gfx_device,
         BF_BUFFER_USAGE_TRANSFER_DST | k_Usage,
         frame_info,
         alignof(T));
      }
      else
      {
        free_list = free_list->next;
      }

      result->vertices_left = NumVerticesPerBatch;
      result->next          = nullptr;

      return result;
    }

    IMemoryManager& memory() const
    {
      return used_buffers.memory();
    }
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
    using SceneUBO   = MultiBuffer<CameraUniformData>;
    using OverlayUBO = MultiBuffer<CameraOverlayUniformData>;

    GBuffer         geometry_buffer;
    SSAOBuffer      ssao_buffer;
    bfTextureHandle composite_buffer;
    SceneUBO        camera_uniform_buffer;
    OverlayUBO      camera_screen_uniform_buffer;

    void                init(bfGfxDeviceHandle device, bfGfxFrameInfo frame_info, int initial_width, int initial_height);
    void                updateBuffers(BifrostCamera& camera, const bfGfxFrameInfo& frame_info, float global_time, const Vector3f& ambient);
    bfDescriptorSetInfo getDescriptorSet(bool is_overlay, const bfGfxFrameInfo& frame_info);
    void                bindDescriptorSet(bfGfxCommandListHandle command_list, bool is_overlay, const bfGfxFrameInfo& frame_info);
    void                resize(bfGfxDeviceHandle device, int width, int height);
    void                deinit(bfGfxDeviceHandle device);

   private:
    void createBuffers(bfGfxDeviceHandle device, int width, int height);
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
    friend class Engine;  // TODO: Not do this?!?!

   public:
    Vector3f AmbientColor = {0.64f};

   private:
    using CameraObjectPair  = std::pair<const CameraGPUData*, Entity*>;
    using RenderableMapping = HashTable<CameraObjectPair, Renderable<ObjectUniformData>*, 64, StdPairHash>;

   public:
    GLSLCompiler                             m_GLSLCompiler;
    bfGfxDeviceHandle                        m_GfxDevice;
    bfGfxFrameInfo                           m_FrameInfo;
    bfVertexLayoutSetHandle                  m_StandardVertexLayout;
    bfVertexLayoutSetHandle                  m_SkinnedVertexLayout;
    bfVertexLayoutSetHandle                  m_EmptyVertexLayout;
    bfGfxCommandListHandle                   m_MainCmdList;
    bfTextureHandle                          m_MainSurface;
    bfShaderProgramHandle                    m_GBufferShader;
    bfShaderProgramHandle                    m_GBufferSelectionShader;
    bfShaderProgramHandle                    m_GBufferSkinnedShader;
    bfShaderProgramHandle                    m_SSAOBufferShader;
    bfShaderProgramHandle                    m_SSAOBlurShader;
    bfShaderProgramHandle                    m_AmbientLighting;
    bfShaderProgramHandle                    m_LightShaders[LightShaders::MAX];
    List<Renderable<ObjectUniformData>>      m_RenderablePool;
    RenderableMapping                        m_RenderableMapping;  // TODO: Make this per Scene.
    Array<bfGfxBaseHandle>                   m_AutoRelease;
    bfTextureHandle                          m_WhiteTexture;
    bfTextureHandle                          m_DefaultMaterialTexture;
    MultiBuffer<DirectionalLightUniformData> m_DirectionalLightBuffer;
    MultiBuffer<PunctualLightUniformData>    m_PunctualLightBuffers[2];  // [Point, Spot]
    float                                    m_GlobalTime;
    bfWindowSurfaceHandle                    m_MainWindow;

   public:
    explicit StandardRenderer(IMemoryManager& memory);

    bfGfxDeviceHandle       device() const { return m_GfxDevice; }
    bfVertexLayoutSetHandle standardVertexLayout() const { return m_StandardVertexLayout; }
    bfGfxCommandListHandle  mainCommandList() const { return m_MainCmdList; }
    bfTextureHandle         surface() const { return m_MainSurface; }
    GLSLCompiler&           glslCompiler() { return m_GLSLCompiler; }
    bfGfxFrameInfo          frameInfo() const { return m_FrameInfo; }

    void               init(const bfGfxContextCreateParams& gfx_create_params, bfWindow* main_window);
    [[nodiscard]] bool frameBegin();
    void               addLight(Light& light);
    void               beginGBufferPass(CameraGPUData& camera) const;
    void               beginSSAOPass(CameraGPUData& camera) const;
    void               beginLightingPass(CameraGPUData& camera);
    void               beginScreenPass(bfGfxCommandListHandle command_list) const;
    void               endPass() const;
    void               drawEnd() const;
    void               frameEnd() const;
    void               deinit();

    bfDescriptorSetInfo makeMaterialInfo(const MaterialAsset& material);
    bfDescriptorSetInfo makeObjectTransformInfo(const Mat4x4& view_proj_cache, const CameraGPUData& camera, Entity& entity);

    void renderCameraTo(RenderView& view);

   private:
    void initShaders();
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
    bfTextureHandle createTexturePNG(
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

    bfClearColor makeClearColor(float r, float g, float b, float a);
    bfClearColor makeClearColor(int32_t r, int32_t g, int32_t b, int32_t a);
    bfClearColor makeClearColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a);
  }  // namespace gfx

  //
  // Shader UBO Bindings
  //
  namespace bindings
  {
    void addObject(bfShaderProgramHandle shader, bfShaderStageBits stages);
    void addMaterial(bfShaderProgramHandle shader, bfShaderStageBits stages);
    void addCamera(bfShaderProgramHandle shader, bfShaderStageBits stages);
    void addSSAOInputs(bfShaderProgramHandle shader, bfShaderStageBits stages);
    void addSSAOBlurInputs(bfShaderProgramHandle shader, bfShaderStageBits stages);
    void addLightingInputs(bfShaderProgramHandle shader, bfShaderStageBits stages);
    void addLightBuffer(bfShaderProgramHandle shader, bfShaderStageBits stages);
  }  // namespace bindings
}  // namespace bf

#endif /* BF_STANDARD_RENDERER_HPP */
