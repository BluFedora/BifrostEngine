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

#include "bifrost/bifrost.hpp"
#include "bifrost/data_structures/bifrost_string.hpp"

namespace bifrost
{
  static const int k_BaseResolution[] = {1280 / 4, 720 / 4};

  void GBuffer::init(bfGfxDeviceHandle device, int width, int height)
  {
    // Function aliases for readability...
    const auto& init_clr_att   = bfTextureCreateParams_initColorAttachment;
    const auto& init_depth_att = bfTextureCreateParams_initDepthAttachment;
    const auto& init_sampler   = bfTextureSamplerProperties_init;

    const bfTextureSamplerProperties sampler = init_sampler(BIFROST_SFM_NEAREST, BIFROST_SAM_REPEAT);
    const bfTextureCreateParams      texture_create_params[k_GfxNumGBufferAttachments] =
     {
      init_clr_att(width, height, BIFROST_IMAGE_FORMAT_R16G16B16A16_SFLOAT, bfTrue, bfFalse),
      init_clr_att(width, height, BIFROST_IMAGE_FORMAT_R16G16B16A16_SFLOAT, bfTrue, bfFalse),
      init_clr_att(width, height, BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM, bfTrue, bfFalse),
     };

    for (int i = 0; i < k_GfxNumGBufferAttachments; ++i)
    {
      auto& color_att = color_attachments[i];

      color_att = bfGfxDevice_newTexture(device, &texture_create_params[i]);
      bfTexture_loadData(color_att, nullptr, 0);
      bfTexture_setSampler(color_att, &sampler);
    }

    const bfTextureCreateParams create_depth_tex = init_depth_att(width, height, BIFROST_IMAGE_FORMAT_D24_UNORM_S8_UINT, bfFalse, bfFalse);

    depth_attachment = bfGfxDevice_newTexture(device, &create_depth_tex);
    bfTexture_loadData(depth_attachment, nullptr, 0);

    for (int i = 0; i < k_GfxNumGBufferAttachments; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        clear_values[i].color.float32[j] = 0.0f;
      }

      clear_values[i].color.float32[3] = 1.0f;
    }

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
    attachments_info[k_GfxNumGBufferAttachments].final_layout = BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
    for (const auto& color_attachment : color_attachments)
    {
      bfGfxDevice_release(device, color_attachment);
    }

    bfGfxDevice_release(device, depth_attachment);
  }

  StandardRenderer::StandardRenderer(IMemoryManager& memory) :
    m_GfxBackend{nullptr},
    m_GfxDevice{nullptr},
    m_StandardVertexLayout{nullptr},
    m_MainCmdList{nullptr},
    m_MainSurface{nullptr},
    m_GBuffer{},
    m_DeferredComposite{nullptr},
    m_GBufferShader{nullptr},
    m_AutoRelease{memory}
  {
  }

  void StandardRenderer::init(const bfGfxContextCreateParams& gfx_create_params)
  {
    m_GfxBackend = bfGfxContext_new(&gfx_create_params);
    m_GfxDevice  = bfGfxContext_device(m_GfxBackend);

    m_StandardVertexLayout = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(m_StandardVertexLayout, 0, sizeof(StandardVertex));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(StandardVertex, pos));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(StandardVertex, normal));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(StandardVertex, color));
    bfVertexLayout_addVertexLayout(m_StandardVertexLayout, 0, BIFROST_VFA_FLOAT32_2, offsetof(StandardVertex, uv));

    m_GBuffer.init(m_GfxDevice, k_BaseResolution[0], k_BaseResolution[1]);

    const auto create_composite = bfTextureCreateParams_initColorAttachment(
     k_BaseResolution[0],
     k_BaseResolution[1],
     BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM,
     bfTrue,
     bfFalse);

    m_DeferredComposite = bfGfxDevice_newTexture(m_GfxDevice, &create_composite);

    initShaders();

    m_AutoRelease.push(m_DeferredComposite);
  }

  bool StandardRenderer::frameBegin()
  {
    if (bfGfxContext_beginFrame(m_GfxBackend, -1))
    {
      const bfGfxCommandListCreateParams thread_command_list{0, -1};

      m_MainCmdList = bfGfxContext_requestCommandList(m_GfxBackend, &thread_command_list);

      if (m_MainCmdList)
      {
        m_MainSurface = bfGfxDevice_requestSurface(m_MainCmdList);

        return bfGfxCmdList_begin(m_MainCmdList);
      }
    }

    return false;
  }

  void StandardRenderer::beginGBufferPass()
  {
    const bfSubpassDependency color_write_dep =
     {
      {0, BIFROST_SUBPASS_EXTERNAL},
      {BIFROST_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, BIFROST_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
      {BIFROST_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, BIFROST_ACCESS_SHADER_READ_BIT},
      false,
     };

    static constexpr std::uint16_t k_ClearFlags = bfBit(0) | bfBit(1) | bfBit(2) | bfBit(3);
    static constexpr std::uint16_t k_StoreFlags = bfBit(0) | bfBit(1) | bfBit(2) | bfBit(3);

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setClearOps(&renderpass_info, k_ClearFlags);
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStoreOps(&renderpass_info, k_StoreFlags);
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, 0x0);
    m_GBuffer.setupAttachments(renderpass_info, 0u);
    bfRenderpassInfo_addDependencies(&renderpass_info, &color_write_dep, 1);

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info);
    bfGfxCmdList_setClearValues(m_MainCmdList, m_GBuffer.clear_values);
    bfGfxCmdList_setAttachments(m_MainCmdList, m_GBuffer.attachments());
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);

    bfGfxCmdList_beginRenderpass(m_MainCmdList);

    bfGfxCmdList_setDepthTesting(m_MainCmdList, bfTrue);
    bfGfxCmdList_setDepthWrite(m_MainCmdList, bfTrue);
    bfGfxCmdList_setDepthTestOp(m_MainCmdList, BIFROST_COMPARE_OP_LESS_OR_EQUAL);

    bfGfxCmdList_bindProgram(m_MainCmdList, m_GBufferShader);
    bfGfxCmdList_bindVertexDesc(m_MainCmdList, m_StandardVertexLayout);
  }

  void StandardRenderer::beginForwardSubpass()
  {
    



  }

  void StandardRenderer::beginScreenPass() const
  {
    bfAttachmentInfo main_surface;
    main_surface.texture      = m_MainSurface;
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

    bfTextureHandle attachments[] = {m_MainSurface};

    bfGfxCmdList_setDepthTesting(m_MainCmdList, bfFalse);
    bfGfxCmdList_setDepthWrite(m_MainCmdList, bfFalse);

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info);
    bfGfxCmdList_setClearValues(m_MainCmdList, clear_colors);
    bfGfxCmdList_setAttachments(m_MainCmdList, attachments);
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);
    bfGfxCmdList_beginRenderpass(m_MainCmdList);

    bfGfxCmdList_bindVertexDesc(m_MainCmdList, m_StandardVertexLayout);
  }

  void StandardRenderer::endPass() const
  {
    bfGfxCmdList_endRenderpass(m_MainCmdList);
  }

  void StandardRenderer::frameEnd() const
  {
    bfGfxCmdList_end(m_MainCmdList);
    bfGfxCmdList_submit(m_MainCmdList);
    bfGfxContext_endFrame(m_GfxBackend);
  }

  void StandardRenderer::deinit()
  {
    bfGfxDevice_flush(m_GfxDevice);

    for (auto resource : m_AutoRelease)
    {
      bfGfxDevice_release(m_GfxDevice, resource);
    }

    deinitShaders();
    m_GBuffer.deinit(m_GfxDevice);

    bfVertexLayout_delete(m_StandardVertexLayout);
    bfGfxContext_delete(m_GfxBackend);

    m_GfxDevice  = nullptr;
    m_GfxBackend = nullptr;
  }

  static bfShaderModuleHandle loadShaderModule(bfGfxDeviceHandle device, const char* file_name)
  {
    static constexpr char k_VertexShaderExt[]       = ".vert.spv";
    static constexpr int  k_VertexShaderExtLength   = sizeof(k_VertexShaderExt) - 1;
    static constexpr char k_FragmentShaderExt[]     = ".frag.spv";
    static constexpr int  k_FragmentShaderExtLength = sizeof(k_FragmentShaderExt) - 1;

    const StringRange path = file_name;

    BifrostShaderType type;

    if (file::pathEndsIn(path.begin(), k_VertexShaderExt, k_VertexShaderExtLength, (int)path.length()))
    {
      type = BIFROST_SHADER_TYPE_VERTEX;
    }
    else if (file::pathEndsIn(path.begin(), k_FragmentShaderExt, k_FragmentShaderExtLength, (int)path.length()))
    {
      type = BIFROST_SHADER_TYPE_FRAGMENT;
    }
    else
    {
      throw "Bad File Ext";
    }

    const bfShaderModuleHandle module = bfGfxDevice_newShaderModule(device, type);

    if (!bfShaderModule_loadFile(module, file_name))
    {
      throw "Bad File Name";
    }

    return module;
  }

  void StandardRenderer::initShaders()
  {
    const auto gbuffer_vert_module = loadShaderModule(m_GfxDevice, "assets/shaders/standard/gbuffer.vert.spv");
    const auto gbuffer_frag_module = loadShaderModule(m_GfxDevice, "assets/shaders/standard/gbuffer.frag.spv");

    bfShaderProgramCreateParams create_shader;
    create_shader.debug_name    = "GBuffer Shader";
    create_shader.num_desc_sets = 4;

    m_GBufferShader = bfGfxDevice_newShaderProgram(m_GfxDevice, &create_shader);

    bfShaderProgram_addModule(m_GBufferShader, gbuffer_vert_module);
    bfShaderProgram_addModule(m_GBufferShader, gbuffer_frag_module);

    bfShaderProgram_addUniformBuffer(m_GBufferShader, "u_Set0", 0, 0, 1, BIFROST_SHADER_STAGE_VERTEX);
    bfShaderProgram_addUniformBuffer(m_GBufferShader, "u_Set3", 3, 0, 1, BIFROST_SHADER_STAGE_VERTEX);
    bfShaderProgram_addUniformBuffer(m_GBufferShader, "u_Set1", 1, 0, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addImageSampler(m_GBufferShader, "u_AlbedoTexture", 2, 0, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addImageSampler(m_GBufferShader, "u_NormalTexture", 2, 1, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addImageSampler(m_GBufferShader, "u_MetallicTexture", 2, 2, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addImageSampler(m_GBufferShader, "u_RoughnessTexture", 2, 3, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addImageSampler(m_GBufferShader, "u_AmbientOcclusionTexture", 2, 4, 1, BIFROST_SHADER_STAGE_FRAGMENT);

    bfShaderProgram_compile(m_GBufferShader);

    m_AutoRelease.push(gbuffer_vert_module);
    m_AutoRelease.push(gbuffer_frag_module);
    m_AutoRelease.push(m_GBufferShader);
  }

  void StandardRenderer::deinitShaders()
  {
    // Shaders are using 'm_AutoRelease'...
  }
}  // namespace bifrost
