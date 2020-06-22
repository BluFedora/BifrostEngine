/*!
* @file   bifrost_standard_renderer.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
* @version 0.0.1
* @date    2020-03-22
*
* @copyright Copyright (c) 2020
*/
#include "bifrost/graphics/bifrost_standard_renderer.hpp"

#include "bifrost/asset_io/bifrost_material.hpp"
#include "bifrost/bifrost.hpp"
#include "bifrost/data_structures/bifrost_string.hpp"
#include "bifrost/memory/bifrost_memory_utils.h"

#include <chrono> /* system_clock              */
#include <random> /* uniform_real_distribution */

namespace bifrost
{
  static const bfTextureSamplerProperties k_SamplerNearestRepeat      = bfTextureSamplerProperties_init(BIFROST_SFM_NEAREST, BIFROST_SAM_REPEAT);
  static const bfTextureSamplerProperties k_SamplerNearestClampToEdge = bfTextureSamplerProperties_init(BIFROST_SFM_NEAREST, BIFROST_SAM_CLAMP_TO_EDGE);
  static constexpr bfColor4u              k_ColorWhite4u              = {0xFF, 0xFF, 0xFF, 0xFF};

  void GBuffer::init(bfGfxDeviceHandle device, int width, int height)
  {
    // Function aliases for readability...
    const auto& init_clr_att   = bfTextureCreateParams_initColorAttachment;
    const auto& init_depth_att = bfTextureCreateParams_initDepthAttachment;

    bfTextureCreateParams texture_create_params[k_GfxNumGBufferAttachments] =
     {
      init_clr_att(width, height, BIFROST_IMAGE_FORMAT_R16G16B16A16_UNORM, bfTrue, bfFalse),
      init_clr_att(width, height, BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM, bfTrue, bfFalse),
     };

    for (int i = 0; i < k_GfxNumGBufferAttachments; ++i)
    {
      color_attachments[i] = gfx::createAttachment(device, texture_create_params[i], k_SamplerNearestClampToEdge);
    }

    const bfTextureCreateParams create_depth_tex = init_depth_att(width, height, BIFROST_IMAGE_FORMAT_D24_UNORM_S8_UINT, bfTrue, bfFalse);

    depth_attachment = bfGfxDevice_newTexture(device, &create_depth_tex);
    bfTexture_loadData(depth_attachment, nullptr, 0);
    bfTexture_setSampler(depth_attachment, &k_SamplerNearestClampToEdge);

    for (int i = 0; i < k_GfxNumGBufferAttachments; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        clear_values[i].color.float32[j] = 0.0f;
      }

      clear_values[i].color.float32[3] = 1.0f;
    }

    // A Brighter ambient background color for editor "Scene View"
    clear_values[1].color.float32[0] = 1.0f;
    clear_values[1].color.float32[1] = 1.0f;
    clear_values[1].color.float32[2] = 1.0f;

    clear_values[k_GfxNumGBufferAttachments].depthStencil.depth   = 1.0f;
    clear_values[k_GfxNumGBufferAttachments].depthStencil.stencil = 0;
  }

  void GBuffer::setupAttachments(bfRenderpassInfo& renderpass_info, uint16_t subpass_index)
  {
    bfAttachmentInfo attachments_info[k_GfxNumGBufferAttachments + 1];  // Last one is depth.

    for (int i = 0; i < k_GfxNumGBufferAttachments; ++i)
    {
      attachments_info[i].texture      = color_attachments[i];
      attachments_info[i].final_layout = BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachments_info[i].may_alias    = bfFalse;
    }

    attachments_info[k_GfxNumGBufferAttachments].texture      = depth_attachment;
    attachments_info[k_GfxNumGBufferAttachments].final_layout = BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments_info[k_GfxNumGBufferAttachments].may_alias    = bfFalse;

    for (const auto& att_info : attachments_info)
    {
      bfRenderpassInfo_addAttachment(&renderpass_info, &att_info);
    }

    for (int i = 0; i < k_GfxNumGBufferAttachments; ++i)
    {
      bfRenderpassInfo_addColorOut(&renderpass_info, subpass_index, i, BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    bfRenderpassInfo_addDepthOut(&renderpass_info, subpass_index, k_GfxNumGBufferAttachments, BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  }

  void GBuffer::deinit(bfGfxDeviceHandle device)
  {
    for (auto& color_attachment : color_attachments)
    {
      bfGfxDevice_release(device, color_attachment);
    }

    bfGfxDevice_release(device, depth_attachment);
  }

  void SSAOBuffer::init(bfGfxDeviceHandle device, int width, int height)
  {
    // Create Color Attachments
    for (auto& color_attachment : color_attachments)
    {
      color_attachment = gfx::createAttachment(
       device,
       bfTextureCreateParams_initColorAttachment(
        width,
        height,
        BIFROST_IMAGE_FORMAT_R8_UNORM,
        bfTrue,
        bfFalse),
       k_SamplerNearestClampToEdge);
    }

    // TODO(Shareef): Should probably add a Random Module to Bifrost.
    std::default_random_engine            rand_engine{unsigned(std::chrono::system_clock::now().time_since_epoch().count())};
    std::uniform_real_distribution<float> rand_distribution{0.0f, 1.0f};

    // Kernel Sample data init
    {
      SSAOKernelUnifromData kernel = {};
      int                   index  = 0;

      for (Vector3f& sample : kernel.u_Kernel)
      {
        const float scale = float(index) / float(k_GfxSSAOKernelSize);

        sample =
         {
          rand_distribution(rand_engine) * 2.0f - 1.0f,  // [-1.0f, +1.0f]
          rand_distribution(rand_engine) * 2.0f - 1.0f,  // [-1.0f, +1.0f]
          rand_distribution(rand_engine),                // [+0.0f, +1.0f]
          1.0f,
         };

        Vec3f_normalize(&sample);

        sample *= math::lerp3(0.1f, scale * scale, 1.0f);  // Moves the sample closer to the origin.

        ++index;
      }

      kernel.u_SampleRadius = 0.5f;
      kernel.u_SampleBias   = 0.025f;

      // TODO: Since (maybe we would want to change SSAOKernelUnifromData::u_SampleRadius) this never changes then this should use staging buffer instead.
      const auto        limits = bfGfxDevice_limits(device);
      const std::size_t size   = bfAlignUpSize(sizeof(SSAOKernelUnifromData), limits.uniform_buffer_offset_alignment);

      const bfBufferCreateParams create_camera_buffer =
       {
        {
         size,
         BIFROST_BPF_HOST_MAPPABLE,
        },
        BIFROST_BUF_UNIFORM_BUFFER,
       };

      kernel_uniform = bfGfxDevice_newBuffer(device, &create_camera_buffer);

      void* uniform_buffer_ptr = bfBuffer_map(kernel_uniform, 0, BIFROST_BUFFER_WHOLE_SIZE);
      std::memcpy(uniform_buffer_ptr, &kernel, sizeof(kernel));
      bfBuffer_unMap(kernel_uniform);
    }

    // Noise Texture Init
    {
      float             noise_texture_data[k_GfxSSAONoiseTextureNumElements * 4];
      const std::size_t noise_texture_data_size = bfCArraySize(noise_texture_data);
      std::size_t       i                       = 0;

      while (i < noise_texture_data_size)
      {
        noise_texture_data[i++] = rand_distribution(rand_engine) * 2.0f - 1.0f;  // [-1.0f, +1.0f]
        noise_texture_data[i++] = rand_distribution(rand_engine) * 2.0f - 1.0f;  // [-1.0f, +1.0f]
        noise_texture_data[i++] = 0.0f;
        noise_texture_data[i++] = 0.0f;
      }

      auto noise_tex_params = bfTextureCreateParams_init2D(BIFROST_IMAGE_FORMAT_R32G32B32A32_SFLOAT, k_GfxSSAONoiseTextureDim, k_GfxSSAONoiseTextureDim);

      noise_tex_params.generate_mipmaps = false;
      noise_tex_params.flags |= BIFROST_TEX_IS_LINEAR;

      noise = gfx::createTexture(
       device,
       noise_tex_params,
       k_SamplerNearestRepeat,
       reinterpret_cast<const char*>(noise_texture_data),
       sizeof(noise_texture_data));
    }

    {
      for (BifrostClearValue& clear_value : clear_values)
      {
        clear_value.color.float32[0] = 0.0f;
        clear_value.color.float32[1] = 0.0f;
        clear_value.color.float32[2] = 0.0f;
        clear_value.color.float32[3] = 0.0f;
      }
    }
  }

  void SSAOBuffer::setupAttachments(bfRenderpassInfo& renderpass_info, uint16_t ao_subpass_index, int color_attachment_idx)
  {
    bfAttachmentInfo attachment;
    attachment.texture      = color_attachments[color_attachment_idx];
    attachment.final_layout = BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachment.may_alias    = bfFalse;

    bfRenderpassInfo_addAttachment(&renderpass_info, &attachment);
    bfRenderpassInfo_addColorOut(&renderpass_info, ao_subpass_index, 0, BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  }

  void SSAOBuffer::deinit(bfGfxDeviceHandle device)
  {
    bfGfxDevice_release(device, noise);
    bfGfxDevice_release(device, kernel_uniform);

    for (auto& color_attachment : color_attachments)
    {
      bfGfxDevice_release(device, color_attachment);
      color_attachment = nullptr;
    }
  }

  void BaseMultiBuffer::create(bfGfxDeviceHandle device, bfBufferUsageBits usage, const bfGfxFrameInfo& info, size_t element_size, size_t element_alignment)
  {
    element_aligned_size = bfAlignUpSize(element_size, element_alignment);
    total_size           = element_aligned_size * info.num_frame_indices;

    const bfBufferCreateParams create_buffer =
     {
      {
       total_size,
       BIFROST_BPF_HOST_MAPPABLE,
      },
      usage,
     };

    handle = bfGfxDevice_newBuffer(device, &create_buffer);
  }

  void BaseMultiBuffer::destroy(bfGfxDeviceHandle device) const
  {
    bfGfxDevice_release(device, handle);
  }

  void Renderable::create(bfGfxDeviceHandle device, const bfGfxFrameInfo& info)
  {
    const auto limits = bfGfxDevice_limits(device);

    transform_uniform.create(device, BIFROST_BUF_UNIFORM_BUFFER, info, limits.uniform_buffer_offset_alignment);
  }

  void Renderable::destroy(bfGfxDeviceHandle device) const
  {
    transform_uniform.destroy(device);
  }

  void CameraGPUData::init(bfGfxDeviceHandle device, bfGfxFrameInfo frame_info, int initial_width, int initial_height)
  {
    const auto limits           = bfGfxDevice_limits(device);
    const auto create_composite = bfTextureCreateParams_initColorAttachment(
     initial_width,
     initial_height,
     BIFROST_IMAGE_FORMAT_R16G16B16A16_SFLOAT,  // TODO:BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM BIFROST_IMAGE_FORMAT_R32G32B32A32_SFLOAT
     bfTrue,
     bfFalse);

    geometry_buffer.init(device, initial_width, initial_height);
    ssao_buffer.init(device, initial_width, initial_height);
    composite_buffer = gfx::createAttachment(device, create_composite, k_SamplerNearestRepeat);
    camera_uniform_buffer.create(device, BIFROST_BUF_UNIFORM_BUFFER | BIFROST_BUF_PERSISTENTLY_MAPPED_BUFFER, frame_info, limits.uniform_buffer_offset_alignment);
  }

  void CameraGPUData::updateBuffers(BifrostCamera& camera, const bfGfxFrameInfo& frame_info, float global_time, const Vector3f& ambient)
  {
    CameraUniformData* const buffer_data = camera_uniform_buffer.currentElement(frame_info);

    Mat4x4 view_proj;
    Mat4x4_mult(&camera.proj_cache, &camera.view_cache, &view_proj);

    buffer_data->u_CameraProjection        = camera.proj_cache;
    buffer_data->u_CameraInvViewProjection = camera.inv_view_proj_cache;
    buffer_data->u_CameraViewProjection    = view_proj;
    buffer_data->u_CameraView              = camera.view_cache;
    buffer_data->u_CameraForwardAndTime    = camera.forward;
    buffer_data->u_CameraForwardAndTime.w  = global_time;
    buffer_data->u_CameraPosition          = camera.position;
    buffer_data->u_CameraAmbient           = ambient;

    camera_uniform_buffer.flushCurrent(frame_info);

    Mat4x4_mult(&camera.proj_cache, &camera.view_cache, &view_projection_cache);
  }

  void CameraGPUData::bindDescriptorSet(bfGfxCommandListHandle command_list, const bfGfxFrameInfo& frame_info)
  {
    const bfBufferSize offset = camera_uniform_buffer.offset(frame_info);
    const bfBufferSize size   = camera_uniform_buffer.elementSize();

    // Update Bindings
    {
      // TODO(SR): Optimize into an immutable DescriptorSet!
      bfDescriptorSetInfo desc_set_camera = bfDescriptorSetInfo_make();

      bfDescriptorSetInfo_addUniform(&desc_set_camera, 0, 0, &offset, &size, &camera_uniform_buffer.handle(), 1);

      bfGfxCmdList_bindDescriptorSet(command_list, k_GfxCameraSetIndex, &desc_set_camera);
    }
  }

  void CameraGPUData::resize(bfGfxDeviceHandle device, int width, int height)
  {
    bfGfxDevice_flush(device);

    ssao_buffer.deinit(device);
    geometry_buffer.deinit(device);
    bfGfxDevice_release(device, composite_buffer);

    geometry_buffer.init(device, width, height);
    ssao_buffer.init(device, width, height);

    const auto create_composite = bfTextureCreateParams_initColorAttachment(
     width,
     height,
     BIFROST_IMAGE_FORMAT_R16G16B16A16_SFLOAT,  // TODO:BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM BIFROST_IMAGE_FORMAT_R32G32B32A32_SFLOAT
     bfTrue,
     bfFalse);

    composite_buffer = gfx::createAttachment(device, create_composite, k_SamplerNearestRepeat);
  }

  void CameraGPUData::deinit(bfGfxDeviceHandle device)
  {
    camera_uniform_buffer.destroy(device);
    ssao_buffer.deinit(device);
    geometry_buffer.deinit(device);
    bfGfxDevice_release(device, composite_buffer);
  }

  StandardRenderer::StandardRenderer(IMemoryManager& memory) :
    m_GLSLCompiler{memory},
    m_GfxBackend{nullptr},
    m_GfxDevice{nullptr},
    m_FrameInfo{},
    m_StandardVertexLayout{nullptr},
    m_EmptyVertexLayout{nullptr},
    m_MainCmdList{nullptr},
    m_MainSurface{nullptr},
    m_GBufferShader{nullptr},
    m_SSAOBufferShader{nullptr},
    m_SSAOBlurShader{nullptr},
    m_AmbientLighting{nullptr},
    m_LightShaders{nullptr},
    m_RenderablePool{memory},
    m_RenderableMapping{},
    m_AutoRelease{memory},
    m_WhiteTexture{nullptr},
    m_DirectionalLightBuffer{},
    m_PunctualLightBuffers{},
    m_GlobalTime{0.0f},
    m_MainWindow{nullptr}
  {
  }

  void StandardRenderer::init(const bfGfxContextCreateParams& gfx_create_params, BifrostWindow* main_window)
  {
    m_GfxBackend               = bfGfxContext_new(&gfx_create_params);
    m_GfxDevice                = bfGfxContext_device(m_GfxBackend);
    main_window->renderer_data = bfGfxContext_createWindow(m_GfxBackend, main_window);
    m_MainWindow               = (bfWindowSurfaceHandle)main_window->renderer_data;
    m_FrameInfo                = bfGfxContext_getFrameInfo(m_GfxBackend, m_MainWindow);

    m_StandardVertexLayout = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(m_StandardVertexLayout, 0, sizeof(StandardVertex));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(StandardVertex, pos));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(StandardVertex, normal));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(StandardVertex, color));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_FLOAT32_2, offsetof(StandardVertex, uv));

    m_EmptyVertexLayout = bfVertexLayout_new();

    initShaders();

    {
      const auto limits = bfGfxDevice_limits(m_GfxDevice);

      m_DirectionalLightBuffer.create(m_GfxDevice, BIFROST_BUF_UNIFORM_BUFFER | BIFROST_BUF_PERSISTENTLY_MAPPED_BUFFER, m_FrameInfo, limits.uniform_buffer_offset_alignment);

      for (auto& buffer : m_PunctualLightBuffers)
      {
        buffer.create(m_GfxDevice, BIFROST_BUF_UNIFORM_BUFFER | BIFROST_BUF_PERSISTENTLY_MAPPED_BUFFER, m_FrameInfo, limits.uniform_buffer_offset_alignment);
      }
    }

    m_WhiteTexture = gfx::createTexture(m_GfxDevice, bfTextureCreateParams_init2D(BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM, 1, 1), k_SamplerNearestClampToEdge, &k_ColorWhite4u, sizeof(k_ColorWhite4u));

    m_AutoRelease.push(m_WhiteTexture);
  }

  bool StandardRenderer::frameBegin()
  {
    if (bfGfxContext_beginFrame(m_GfxBackend, m_MainWindow))
    {
      m_MainCmdList = bfGfxContext_requestCommandList(m_GfxBackend, m_MainWindow, 0);
      m_FrameInfo   = bfGfxContext_getFrameInfo(m_GfxBackend, m_MainWindow);

      if (m_MainCmdList)
      {
        m_MainSurface = bfGfxDevice_requestSurface(m_MainWindow);

        DirectionalLightUniformData* dir_light_buffer   = m_DirectionalLightBuffer.currentElement(m_FrameInfo);
        PunctualLightUniformData*    point_light_buffer = m_PunctualLightBuffers[0].currentElement(m_FrameInfo);
        PunctualLightUniformData*    spot_light_buffer  = m_PunctualLightBuffers[1].currentElement(m_FrameInfo);

        dir_light_buffer->u_NumLights   = 0;
        point_light_buffer->u_NumLights = 0;
        spot_light_buffer->u_NumLights  = 0;

        return bfGfxCmdList_begin(m_MainCmdList);
      }
    }

    return false;
  }

  void StandardRenderer::bindMaterial(bfGfxCommandListHandle command_list, const Material& material)
  {
    const auto defaultTexture = [this](const AssetTextureHandle& handle) -> bfTextureHandle {
      return handle ? handle->handle() : m_WhiteTexture;
    };

    bfTextureHandle albedo            = defaultTexture(material.albedoTexture());
    bfTextureHandle normal            = defaultTexture(material.normalTexture());
    bfTextureHandle metallic          = defaultTexture(material.metallicTexture());
    bfTextureHandle roughness         = defaultTexture(material.roughnessTexture());
    bfTextureHandle ambient_occlusion = defaultTexture(material.ambientOcclusionTexture());

    // Update Bindings
    {
      bfDescriptorSetInfo desc_set_material = bfDescriptorSetInfo_make();

      bfDescriptorSetInfo_addTexture(&desc_set_material, 0, 0, &albedo, 1);
      bfDescriptorSetInfo_addTexture(&desc_set_material, 1, 0, &normal, 1);
      bfDescriptorSetInfo_addTexture(&desc_set_material, 2, 0, &metallic, 1);
      bfDescriptorSetInfo_addTexture(&desc_set_material, 3, 0, &roughness, 1);
      bfDescriptorSetInfo_addTexture(&desc_set_material, 4, 0, &ambient_occlusion, 1);

      bfGfxCmdList_bindDescriptorSet(command_list, k_GfxMaterialSetIndex, &desc_set_material);
    }
  }

  void StandardRenderer::bindObject(bfGfxCommandListHandle command_list, const CameraGPUData& camera, Entity& entity)
  {
    const auto key = CameraObjectPair{&camera, &entity};

    auto it = m_RenderableMapping.find(key);

    Renderable* renderable;

    if (it == m_RenderableMapping.end())
    {
      renderable = &m_RenderablePool.emplaceFront();
      renderable->create(m_GfxDevice, m_FrameInfo);
      m_RenderableMapping.emplace(key, renderable);
    }
    else
    {
      renderable = it->value();
    }

    const bfBufferSize offset = renderable->transform_uniform.offset(m_FrameInfo);
    const bfBufferSize size   = sizeof(ObjectUniformData);

    // Upload Data
    {
      ObjectUniformData* const obj_data = (ObjectUniformData*)bfBuffer_map(renderable->transform_uniform.handle(), offset, size);

      Mat4x4& model = entity.transform().world_transform;

      Mat4x4 model_view_proj;
      Mat4x4_mult(&camera.view_projection_cache, &model, &model_view_proj);

      obj_data->u_ModelViewProjection = model_view_proj;
      obj_data->u_Model               = model;
      obj_data->u_NormalModel         = entity.transform().normal_transform;

      renderable->transform_uniform.flushCurrent(m_FrameInfo, size);
      bfBuffer_unMap(renderable->transform_uniform.handle());
    }

    // Update Bindings
    {
      // TODO(SR): Optimize into an immutable DescriptorSet!
      bfDescriptorSetInfo desc_set_object = bfDescriptorSetInfo_make();

      bfDescriptorSetInfo_addUniform(&desc_set_object, 0, 0, &offset, &size, &renderable->transform_uniform.handle(), 1);

      bfGfxCmdList_bindDescriptorSet(command_list, k_GfxObjectSetIndex, &desc_set_object);
    }
  }

  void StandardRenderer::addLight(Light& light)
  {
    LightGPUData* gpu_light = nullptr;

    switch (light.type())
    {
      case LightType::DIRECTIONAL:
      {
        DirectionalLightUniformData* dir_light_buffer = m_DirectionalLightBuffer.currentElement(m_FrameInfo);

        if (dir_light_buffer->u_NumLights < int(bfCArraySize(dir_light_buffer->u_Lights)))
        {
          gpu_light = dir_light_buffer->u_Lights + dir_light_buffer->u_NumLights;

          ++dir_light_buffer->u_NumLights;
        }
        break;
      }
      case LightType::POINT:
      case LightType::SPOT:
      {
        PunctualLightUniformData* light_buffer = m_PunctualLightBuffers[light.type() == LightType::SPOT].currentElement(m_FrameInfo);

        if (light_buffer->u_NumLights < int(bfCArraySize(light_buffer->u_Lights)))
        {
          gpu_light = light_buffer->u_Lights + light_buffer->u_NumLights;
          ++light_buffer->u_NumLights;
        }
        break;
      }
    }

    if (gpu_light)
    {
      Light::LightGPUDataCache& gpu_cache = light.m_GPUCache;

      if (light.type() != LightType::DIRECTIONAL && gpu_cache.is_dirty)
      {
        const float inv_radius = 1.0f / std::max(light.radius(), k_Epsilon);

        gpu_cache.inv_light_radius_pow2 = inv_radius * inv_radius;

        if (light.type() == LightType::SPOT)
        {
          const float cos_inner = std::cos(light.innerAngleRad());
          const float cos_outer = std::cos(light.outerAngleRad());

          gpu_cache.spot_scale  = 1.0f / std::max(cos_inner - cos_outer, k_Epsilon);
          gpu_cache.spot_offset = -cos_outer * light.m_GPUCache.spot_scale;
        }

        gpu_cache.is_dirty = false;
      }

      // TODO: Direction should adopt transform.
      gpu_light->color                           = light.colorIntensity();
      gpu_light->direction_and_inv_radius_pow2   = light.direction();
      gpu_light->direction_and_inv_radius_pow2.w = gpu_cache.inv_light_radius_pow2;
      gpu_light->position_and_spot_scale         = light.owner().transform().world_position;
      gpu_light->position_and_spot_scale.w       = gpu_cache.spot_scale;
      gpu_light->spot_offset                     = gpu_cache.spot_offset;
    }
  }

  void StandardRenderer::beginGBufferPass(CameraGPUData& camera) const
  {
    static constexpr std::uint16_t k_LoadFlags         = 0x0;
    static constexpr std::uint16_t k_ClearFlags        = bfBit(0) | bfBit(1) | bfBit(2);
    static constexpr std::uint16_t k_StoreFlags        = bfBit(0) | bfBit(1) | bfBit(2);
    static constexpr std::uint16_t k_StencilClearFlags = bfBit(k_GfxNumGBufferAttachments);
    static constexpr std::uint16_t k_StencilStoreFlags = bfBit(k_GfxNumGBufferAttachments);

    const bfSubpassDependency color_write_dep =
     {
      {0, BIFROST_SUBPASS_EXTERNAL},
      {BIFROST_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, BIFROST_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
      {BIFROST_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, BIFROST_ACCESS_SHADER_READ_BIT},
      true,
     };

    auto& gbuffer = camera.geometry_buffer;

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, k_LoadFlags);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, k_LoadFlags);
    bfRenderpassInfo_setClearOps(&renderpass_info, k_ClearFlags);
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, k_StencilClearFlags);
    bfRenderpassInfo_setStoreOps(&renderpass_info, k_StoreFlags);
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, k_StencilStoreFlags);
    gbuffer.setupAttachments(renderpass_info, 0u);
    bfRenderpassInfo_addDependencies(&renderpass_info, &color_write_dep, 1);

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info);
    bfGfxCmdList_setClearValues(m_MainCmdList, gbuffer.clear_values);
    bfGfxCmdList_setAttachments(m_MainCmdList, gbuffer.attachments());
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);

    bfGfxCmdList_beginRenderpass(m_MainCmdList);

    bfGfxCmdList_setDepthTesting(m_MainCmdList, bfTrue);
    bfGfxCmdList_setDepthWrite(m_MainCmdList, bfTrue);
    bfGfxCmdList_setDepthTestOp(m_MainCmdList, BIFROST_COMPARE_OP_LESS_OR_EQUAL);
    bfGfxCmdList_setCullFace(m_MainCmdList, BIFROST_CULL_FACE_BACK);

    for (int i = 0; i < k_GfxNumGBufferAttachments; ++i)
    {
      bfGfxCmdList_setBlendSrc(m_MainCmdList, i, BIFROST_BLEND_FACTOR_NONE);
      bfGfxCmdList_setBlendDst(m_MainCmdList, i, BIFROST_BLEND_FACTOR_NONE);
      bfGfxCmdList_setBlendSrcAlpha(m_MainCmdList, i, BIFROST_BLEND_FACTOR_NONE);
      bfGfxCmdList_setBlendDstAlpha(m_MainCmdList, i, BIFROST_BLEND_FACTOR_NONE);
    }

    bfGfxCmdList_bindProgram(m_MainCmdList, m_GBufferShader);
    bfGfxCmdList_bindVertexDesc(m_MainCmdList, m_StandardVertexLayout);
  }

  void StandardRenderer::beginSSAOPass(CameraGPUData& camera) const
  {
    static constexpr std::uint16_t k_LoadFlags         = 0x0;
    static constexpr std::uint16_t k_ClearFlags        = bfBit(0) | bfBit(1) | bfBit(2);
    static constexpr std::uint16_t k_StoreFlags        = bfBit(0) | bfBit(1) | bfBit(2);
    static constexpr std::uint16_t k_StencilClearFlags = 0x0;
    static constexpr std::uint16_t k_StencilStoreFlags = 0x0;

    bfGfxCmdList_setCullFace(m_MainCmdList, BIFROST_CULL_FACE_FRONT);

    {
      const bfPipelineBarrier barriers[] =
       {
        bfPipelineBarrier_memory(BIFROST_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, BIFROST_ACCESS_SHADER_READ_BIT),
       };
      const std::uint32_t num_barriers = std::uint32_t(bfCArraySize(barriers));

      bfGfxCmdList_pipelineBarriers(
       m_MainCmdList,
       BIFROST_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
       BIFROST_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       barriers,
       num_barriers,
       bfTrue);
    }

    auto renderpass_info0 = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info0, k_LoadFlags);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info0, k_LoadFlags);
    bfRenderpassInfo_setClearOps(&renderpass_info0, k_ClearFlags);
    bfRenderpassInfo_setStencilClearOps(&renderpass_info0, k_StencilClearFlags);
    bfRenderpassInfo_setStoreOps(&renderpass_info0, k_StoreFlags);
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info0, k_StencilStoreFlags);

    auto  renderpass_info1 = renderpass_info0;
    auto& ssoa_buffer      = camera.ssao_buffer;
    auto& gbuffer          = camera.geometry_buffer;

    ssoa_buffer.setupAttachments(renderpass_info0, 0u, 0u);
    ssoa_buffer.setupAttachments(renderpass_info1, 0u, 1u);

    bfGfxCmdList_bindVertexDesc(m_MainCmdList, m_EmptyVertexLayout);
    bfGfxCmdList_setDepthTesting(m_MainCmdList, bfFalse);
    bfGfxCmdList_setDepthWrite(m_MainCmdList, bfFalse);

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info0);
    bfGfxCmdList_setClearValues(m_MainCmdList, ssoa_buffer.clear_values);
    bfGfxCmdList_setAttachments(m_MainCmdList, ssoa_buffer.color_attachments);
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);

    bfGfxCmdList_beginRenderpass(m_MainCmdList);

    bfGfxCmdList_bindProgram(m_MainCmdList, m_SSAOBufferShader);

    camera.bindDescriptorSet(m_MainCmdList, m_FrameInfo);

    {
      bfDescriptorSetInfo desc_set_textures = bfDescriptorSetInfo_make();
      bfBufferSize        offset            = 0;
      bfBufferSize        size              = bfBuffer_size(ssoa_buffer.kernel_uniform);

      bfDescriptorSetInfo_addTexture(&desc_set_textures, 0, 0, &gbuffer.depth_attachment, 1);
      bfDescriptorSetInfo_addTexture(&desc_set_textures, 1, 0, &gbuffer.color_attachments[0], 1);
      bfDescriptorSetInfo_addTexture(&desc_set_textures, 2, 0, &ssoa_buffer.noise, 1);
      bfDescriptorSetInfo_addUniform(&desc_set_textures, 3, 0, &offset, &size, &ssoa_buffer.kernel_uniform, 1);

      bfGfxCmdList_bindDescriptorSet(m_MainCmdList, k_GfxMaterialSetIndex, &desc_set_textures);
    }

    bfGfxCmdList_draw(m_MainCmdList, 0, 3);

    endPass(m_MainCmdList);

    {
      const bfPipelineBarrier barriers[] =
       {
        bfPipelineBarrier_image(
         BIFROST_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
         BIFROST_ACCESS_SHADER_READ_BIT,
         ssoa_buffer.color_attachments[0],
         BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
       };
      const std::uint32_t num_barriers = std::uint32_t(bfCArraySize(barriers));

      bfGfxCmdList_pipelineBarriers(
       m_MainCmdList,
       BIFROST_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
       BIFROST_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
       barriers,
       num_barriers,
       bfFalse);
    }

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info1);
    bfGfxCmdList_setClearValues(m_MainCmdList, ssoa_buffer.clear_values + 1u);
    bfGfxCmdList_setAttachments(m_MainCmdList, ssoa_buffer.color_attachments + 1u);
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);
    bfGfxCmdList_beginRenderpass(m_MainCmdList);

    bfGfxCmdList_bindProgram(m_MainCmdList, m_SSAOBlurShader);

    camera.bindDescriptorSet(m_MainCmdList, m_FrameInfo);

    {
      bfDescriptorSetInfo desc_set_textures = bfDescriptorSetInfo_make();

      bfDescriptorSetInfo_addTexture(&desc_set_textures, 0, 0, &ssoa_buffer.color_attachments[0], 1);

      bfGfxCmdList_bindDescriptorSet(m_MainCmdList, k_GfxMaterialSetIndex, &desc_set_textures);
    }

    bfGfxCmdList_draw(m_MainCmdList, 0, 3);
  }

  void StandardRenderer::beginLightingPass(CameraGPUData& camera)
  {
    const bfPipelineBarrier barriers[] =
     {
      bfPipelineBarrier_memory(BIFROST_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, BIFROST_ACCESS_SHADER_READ_BIT),
     };
    const std::uint32_t num_barriers = std::uint32_t(bfCArraySize(barriers));

    bfGfxCmdList_pipelineBarriers(
     m_MainCmdList,
     BIFROST_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
     BIFROST_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
     barriers,
     num_barriers,
     bfTrue);

    bfAttachmentInfo deferred_composite;
    deferred_composite.texture      = camera.composite_buffer;
    deferred_composite.final_layout = BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    deferred_composite.may_alias    = bfFalse;

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setClearOps(&renderpass_info, bfBit(0));
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStoreOps(&renderpass_info, bfBit(0));
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, 0x0);
    bfRenderpassInfo_addAttachment(&renderpass_info, &deferred_composite);
    bfRenderpassInfo_addColorOut(&renderpass_info, 0, 0, BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    BifrostClearValue clear_colors[1];
    clear_colors[0].color.float32[0] = 0.2f;
    clear_colors[0].color.float32[1] = 0.2f;
    clear_colors[0].color.float32[2] = 0.2f;
    clear_colors[0].color.float32[3] = 1.0f;

    bfTextureHandle attachments[] = {deferred_composite.texture};

    bfGfxCmdList_bindVertexDesc(m_MainCmdList, m_EmptyVertexLayout);
    bfGfxCmdList_setDepthTesting(m_MainCmdList, bfFalse);
    bfGfxCmdList_setDepthWrite(m_MainCmdList, bfFalse);

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info);
    bfGfxCmdList_setClearValues(m_MainCmdList, clear_colors);
    bfGfxCmdList_setAttachments(m_MainCmdList, attachments);
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);
    bfGfxCmdList_beginRenderpass(m_MainCmdList);

    auto& ssoa_buffer = camera.ssao_buffer;
    auto& gbuffer     = camera.geometry_buffer;

    const auto baseLightingBegin = [this, &camera](auto& shader) {
      bfGfxCmdList_bindProgram(m_MainCmdList, shader);
      camera.bindDescriptorSet(m_MainCmdList, m_FrameInfo);
    };

    const auto baseLightingEnd = [this, &gbuffer, &ssoa_buffer]() {
      bfDescriptorSetInfo desc_set_textures = bfDescriptorSetInfo_make();

      bfDescriptorSetInfo_addTexture(&desc_set_textures, 0, 0, &gbuffer.color_attachments[0], 1);
      bfDescriptorSetInfo_addTexture(&desc_set_textures, 1, 0, &gbuffer.color_attachments[1], 1);
      bfDescriptorSetInfo_addTexture(&desc_set_textures, 2, 0, &ssoa_buffer.color_attachments[1], 1);
      bfDescriptorSetInfo_addTexture(&desc_set_textures, 3, 0, &gbuffer.depth_attachment, 1);

      bfGfxCmdList_bindDescriptorSet(m_MainCmdList, k_GfxMaterialSetIndex, &desc_set_textures);

      bfGfxCmdList_draw(m_MainCmdList, 0, 3);
    };

    const auto lightingDraw = [this, &baseLightingBegin, &baseLightingEnd](auto& shader, auto& buffer) -> void {
      baseLightingBegin(shader);

      {
        buffer.flushCurrent(m_FrameInfo);

        bfDescriptorSetInfo desc_set_buffer = bfDescriptorSetInfo_make();

        bfBufferSize offset = buffer.offset(m_FrameInfo);
        bfBufferSize size   = buffer.elementSize();

        bfDescriptorSetInfo_addUniform(&desc_set_buffer, 0, 0, &offset, &size, &buffer.handle(), 1);

        bfGfxCmdList_bindDescriptorSet(m_MainCmdList, k_GfxLightSetIndex, &desc_set_buffer);
      }

      baseLightingEnd();
    };

    baseLightingBegin(m_AmbientLighting);
    baseLightingEnd();

    bfGfxCmdList_setBlendSrc(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_ONE);
    bfGfxCmdList_setBlendDst(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_ONE);
    bfGfxCmdList_setBlendSrcAlpha(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_ONE);
    bfGfxCmdList_setBlendDstAlpha(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_ZERO);

    lightingDraw(m_LightShaders[LightShaders::DIR], m_DirectionalLightBuffer);
    lightingDraw(m_LightShaders[LightShaders::POINT], m_PunctualLightBuffers[0]);
    lightingDraw(m_LightShaders[LightShaders::SPOT], m_PunctualLightBuffers[1]);

    bfGfxCmdList_setBlendSrc(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_SRC_ALPHA);
    bfGfxCmdList_setBlendDst(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    bfGfxCmdList_setBlendSrcAlpha(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_ONE);
    bfGfxCmdList_setBlendDstAlpha(m_MainCmdList, 0, BIFROST_BLEND_FACTOR_ZERO);

    // TODO: Post process pass.
  }

  void StandardRenderer::beginScreenPass(bfGfxCommandListHandle command_list) const
  {
    const bfTextureHandle surface_tex = bfGfxDevice_requestSurface(bfGfxCmdList_window(command_list));

    bfAttachmentInfo main_surface;
    main_surface.texture      = surface_tex;
    main_surface.final_layout = BIFROST_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    main_surface.may_alias    = bfFalse;

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setClearOps(&renderpass_info, bfBit(0));
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStoreOps(&renderpass_info, bfBit(0));
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, 0x0);
    bfRenderpassInfo_addAttachment(&renderpass_info, &main_surface);
    bfRenderpassInfo_addColorOut(&renderpass_info, 0, 0, BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    BifrostClearValue clear_colors[1];
    clear_colors[0].color.float32[0] = 0.6f;
    clear_colors[0].color.float32[1] = 0.6f;
    clear_colors[0].color.float32[2] = 0.75f;
    clear_colors[0].color.float32[3] = 1.0f;

    bfTextureHandle attachments[] = {surface_tex};

    bfGfxCmdList_setDepthTesting(command_list, bfFalse);
    bfGfxCmdList_setDepthWrite(command_list, bfFalse);

    bfGfxCmdList_setRenderpassInfo(command_list, &renderpass_info);
    bfGfxCmdList_setClearValues(command_list, clear_colors);
    bfGfxCmdList_setAttachments(command_list, attachments);
    bfGfxCmdList_setRenderAreaRel(command_list, 0.0f, 0.0f, 1.0f, 1.0f);
    bfGfxCmdList_beginRenderpass(command_list);

    bfGfxCmdList_bindVertexDesc(command_list, m_StandardVertexLayout);
  }

  void StandardRenderer::endPass(bfGfxCommandListHandle command_list) const
  {
    bfGfxCmdList_endRenderpass(command_list);
  }

  void StandardRenderer::frameEnd() const
  {
    bfGfxCmdList_end(m_MainCmdList);
    bfGfxCmdList_submit(m_MainCmdList);
    bfGfxContext_endFrame(m_GfxBackend);
  }

  void StandardRenderer::deinit()
  {
    for (Renderable& renderable : m_RenderablePool)
    {
      renderable.destroy(m_GfxDevice);
    }
    m_RenderablePool.clear();

    for (auto resource : m_AutoRelease)
    {
      bfGfxDevice_release(m_GfxDevice, resource);
    }
    m_AutoRelease.clear();

    deinitShaders();

    m_DirectionalLightBuffer.destroy(m_GfxDevice);

    for (auto& buffer : m_PunctualLightBuffers)
    {
      buffer.destroy(m_GfxDevice);
    }

    bfVertexLayout_delete(m_EmptyVertexLayout);
    bfVertexLayout_delete(m_StandardVertexLayout);
    bfGfxContext_delete(m_GfxBackend);

    m_GfxDevice  = nullptr;
    m_GfxBackend = nullptr;
  }

  namespace bindings
  {
    void addObject(bfShaderProgramHandle shader, BifrostShaderStageBits stages)
    {
      bfShaderProgram_addUniformBuffer(shader, "u_Set3", k_GfxObjectSetIndex, 0, 1, stages);
    }

    void addMaterial(bfShaderProgramHandle shader, BifrostShaderStageBits stages)
    {
      bfShaderProgram_addImageSampler(shader, "u_AlbedoTexture", k_GfxMaterialSetIndex, 0, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_NormalTexture", k_GfxMaterialSetIndex, 1, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_MetallicTexture", k_GfxMaterialSetIndex, 2, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_RoughnessTexture", k_GfxMaterialSetIndex, 3, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_AmbientOcclusionTexture", k_GfxMaterialSetIndex, 4, 1, stages);
    }

    void addCamera(bfShaderProgramHandle shader, BifrostShaderStageBits stages)
    {
      bfShaderProgram_addUniformBuffer(shader, "u_Set0", k_GfxCameraSetIndex, 0, 1, stages);
    }

    void addSSAOInputs(bfShaderProgramHandle shader, BifrostShaderStageBits stages)
    {
      bfShaderProgram_addImageSampler(shader, "u_DepthTexture", k_GfxMaterialSetIndex, 0, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_NormalTexture", k_GfxMaterialSetIndex, 1, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_NoiseTexture", k_GfxMaterialSetIndex, 2, 1, stages);
      bfShaderProgram_addUniformBuffer(shader, "u_Set2", k_GfxMaterialSetIndex, 3, 1, stages);
    }

    void addSSAOBlurInputs(bfShaderProgramHandle shader, BifrostShaderStageBits stages)
    {
      bfShaderProgram_addImageSampler(shader, "u_SSAOTexture", k_GfxMaterialSetIndex, 0, 1, stages);
    }

    void addLightingInputs(bfShaderProgramHandle shader, BifrostShaderStageBits stages)
    {
      bfShaderProgram_addImageSampler(shader, "u_GBufferRT0", k_GfxMaterialSetIndex, 0, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_GBufferRT1", k_GfxMaterialSetIndex, 1, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_SSAOBlurredBuffer", k_GfxMaterialSetIndex, 2, 1, stages);
      bfShaderProgram_addImageSampler(shader, "u_DepthTexture", k_GfxMaterialSetIndex, 3, 1, stages);
    }

    void addLightBuffer(bfShaderProgramHandle shader, BifrostShaderStageBits stages)
    {
      bfShaderProgram_addUniformBuffer(shader, "u_Set1", k_GfxLightSetIndex, 0, 1, stages);
    }
  }  // namespace bindings

  void StandardRenderer::initShaders()
  {
    const auto gbuffer_vert_module       = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/gbuffer.vert.glsl");
    const auto gbuffer_frag_module       = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/gbuffer.frag.glsl");
    const auto fullscreen_vert_module    = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/fullscreen_quad.vert.glsl");
    const auto ssao_frag_module          = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/ssao.frag.glsl");
    const auto ssao_blur_frag_module     = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/ssao_blur.frag.glsl");
    const auto ambient_light_frag_module = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/ambient_lighting.frag.glsl");
    const auto dir_light_frag_module     = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/directional_lighting.frag.glsl");
    const auto point_light_frag_module   = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/point_lighting.frag.glsl");
    const auto spot_light_frag_module    = m_GLSLCompiler.createModule(m_GfxDevice, "assets/shaders/standard/spot_lighting.frag.glsl");

    m_GBufferShader                     = gfx::createShaderProgram(m_GfxDevice, 4, gbuffer_vert_module, gbuffer_frag_module, "GBuffer Shader");
    m_SSAOBufferShader                  = gfx::createShaderProgram(m_GfxDevice, 3, fullscreen_vert_module, ssao_frag_module, "SSAO Buffer");
    m_SSAOBlurShader                    = gfx::createShaderProgram(m_GfxDevice, 3, fullscreen_vert_module, ssao_blur_frag_module, "SSAO Blur Buffer");
    m_AmbientLighting                   = gfx::createShaderProgram(m_GfxDevice, 3, fullscreen_vert_module, ambient_light_frag_module, "A Light");
    m_LightShaders[LightShaders::DIR]   = gfx::createShaderProgram(m_GfxDevice, 3, fullscreen_vert_module, dir_light_frag_module, "D Light Shader");
    m_LightShaders[LightShaders::POINT] = gfx::createShaderProgram(m_GfxDevice, 3, fullscreen_vert_module, point_light_frag_module, "P Light Shader");
    m_LightShaders[LightShaders::SPOT]  = gfx::createShaderProgram(m_GfxDevice, 3, fullscreen_vert_module, spot_light_frag_module, "S Light Shader");

    bindings::addObject(m_GBufferShader, BIFROST_SHADER_STAGE_VERTEX);
    bindings::addMaterial(m_GBufferShader, BIFROST_SHADER_STAGE_FRAGMENT);

    bindings::addCamera(m_SSAOBufferShader, BIFROST_SHADER_STAGE_VERTEX | BIFROST_SHADER_STAGE_FRAGMENT);
    bindings::addSSAOInputs(m_SSAOBufferShader, BIFROST_SHADER_STAGE_FRAGMENT);

    bindings::addCamera(m_SSAOBlurShader, BIFROST_SHADER_STAGE_VERTEX);
    bindings::addSSAOBlurInputs(m_SSAOBlurShader, BIFROST_SHADER_STAGE_FRAGMENT);

    bindings::addCamera(m_AmbientLighting, BIFROST_SHADER_STAGE_VERTEX | BIFROST_SHADER_STAGE_FRAGMENT);
    bindings::addLightingInputs(m_AmbientLighting, BIFROST_SHADER_STAGE_FRAGMENT);
    bindings::addLightBuffer(m_AmbientLighting, BIFROST_SHADER_STAGE_FRAGMENT);

    for (const auto& light_shader : m_LightShaders)
    {
      bindings::addCamera(light_shader, BIFROST_SHADER_STAGE_VERTEX | BIFROST_SHADER_STAGE_FRAGMENT);
      bindings::addLightingInputs(light_shader, BIFROST_SHADER_STAGE_FRAGMENT);
      bindings::addLightBuffer(light_shader, BIFROST_SHADER_STAGE_FRAGMENT);
    }

    bfShaderProgram_compile(m_GBufferShader);
    bfShaderProgram_compile(m_SSAOBufferShader);
    bfShaderProgram_compile(m_SSAOBlurShader);
    bfShaderProgram_compile(m_AmbientLighting);
    bfShaderProgram_compile(m_LightShaders[LightShaders::DIR]);
    bfShaderProgram_compile(m_LightShaders[LightShaders::POINT]);
    bfShaderProgram_compile(m_LightShaders[LightShaders::SPOT]);

    m_AutoRelease.push(gbuffer_vert_module);
    m_AutoRelease.push(gbuffer_frag_module);
    m_AutoRelease.push(fullscreen_vert_module);
    m_AutoRelease.push(ssao_frag_module);
    m_AutoRelease.push(ssao_blur_frag_module);
    m_AutoRelease.push(ambient_light_frag_module);
    m_AutoRelease.push(dir_light_frag_module);
    m_AutoRelease.push(point_light_frag_module);
    m_AutoRelease.push(spot_light_frag_module);
    m_AutoRelease.push(m_GBufferShader);
    m_AutoRelease.push(m_SSAOBufferShader);
    m_AutoRelease.push(m_SSAOBlurShader);
    m_AutoRelease.push(m_AmbientLighting);
    m_AutoRelease.push(m_LightShaders[LightShaders::DIR]);
    m_AutoRelease.push(m_LightShaders[LightShaders::POINT]);
    m_AutoRelease.push(m_LightShaders[LightShaders::SPOT]);
  }

  void StandardRenderer::deinitShaders()
  {
    // Shaders are using 'm_AutoRelease'...
  }

  bool AssetTextureInfo::load(Engine& engine)
  {
    const bfGfxDeviceHandle device = bfGfxContext_device(engine.renderer().context());

    const bfTextureCreateParams create_params = bfTextureCreateParams_init2D(
     BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM,
     BIFROST_TEXTURE_UNKNOWN_SIZE,
     BIFROST_TEXTURE_UNKNOWN_SIZE);

    const String full_path = engine.assets().fullPath(*this);
    Texture&     texture   = m_Payload.set<Texture>(device);

    texture.m_Handle = bfGfxDevice_newTexture(device, &create_params);

    bfTexture_loadFile(texture.m_Handle, full_path.c_str());
    bfTexture_setSampler(texture.m_Handle, &k_SamplerNearestRepeat);

    return true;
  }

  namespace gfx
  {
    bfTextureHandle createAttachment(bfGfxDeviceHandle device, const bfTextureCreateParams& create_params, const bfTextureSamplerProperties& sampler)
    {
      const bfTextureHandle color_att = bfGfxDevice_newTexture(device, &create_params);

      bfTexture_loadData(color_att, nullptr, 0);
      bfTexture_setSampler(color_att, &sampler);

      return color_att;
    }

    bfTextureHandle createTexture(bfGfxDeviceHandle device, const bfTextureCreateParams& create_params, const bfTextureSamplerProperties& sampler, const void* data, std::size_t data_size)
    {
      const bfTextureHandle texture = bfGfxDevice_newTexture(device, &create_params);

      bfTexture_loadData(texture, reinterpret_cast<const char*>(data), data_size);
      bfTexture_setSampler(texture, &sampler);

      return texture;
    }

    bfShaderProgramHandle createShaderProgram(bfGfxDeviceHandle device, std::uint32_t num_desc_sets, bfShaderModuleHandle vertex_module, bfShaderModuleHandle fragment_module, const char* debug_name)
    {
      bfShaderProgramCreateParams create_shader{debug_name, num_desc_sets};

      const bfShaderProgramHandle shader = bfGfxDevice_newShaderProgram(device, &create_shader);

      bfShaderProgram_addModule(shader, vertex_module);
      bfShaderProgram_addModule(shader, fragment_module);

      return shader;
    }
  }  // namespace gfx
}  // namespace bifrost
