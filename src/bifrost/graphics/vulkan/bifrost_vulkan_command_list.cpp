#include "bifrost_vulkan_logical_device.h"

#include "bifrost_vulkan_conversions.h" /* bfVKConvert* */
#include "bifrost_vulkan_hash.hpp"      /* hash */
#include <cassert>                      /* assert */

#define CUSTOM_ALLOC nullptr

bfBool32 bfGfxCmdList_begin(bfGfxCommandListHandle self)
{
  VkCommandBufferBeginInfo begin_info;
  begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext            = NULL;
  begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  begin_info.pInheritanceInfo = NULL;

  const VkResult error = vkBeginCommandBuffer(self->handle, &begin_info);
  return error == VK_SUCCESS;
}

void bfGfxCmdList_setRenderpass(bfGfxCommandListHandle self, bfRenderpassHandle renderpass)
{
  self->renderpass = renderpass;
}

void bfGfxCmdList_setRenderpassInfo(bfGfxCommandListHandle self, const bfRenderpassInfo* renderpass_info)
{
  const uint64_t hash_code = bifrost::vk::hash(0x0, renderpass_info);

  bfRenderpassHandle rp = self->parent->cache_renderpass.find(hash_code);

  if (!rp)
  {
    rp = bfGfxDevice_newRenderpass(self->parent, renderpass_info);
    self->parent->cache_renderpass.insert(hash_code, rp);
  }

  bfGfxCmdList_setRenderpass(self, rp);
}

void bfGfxCmdList_setClearValues(bfGfxCommandListHandle self, const BifrostClearValue* clear_values)
{
  const std::size_t num_clear_colors = self->renderpass->info.num_attachments;

  for (std::size_t i = 0; i < num_clear_colors; ++i)
  {
    const BifrostClearValue* const bf_color = clear_values + i;
    VkClearValue* const            color    = self->clear_colors + i;

    *color = bfVKConvertClearColor(bf_color);
  }
}

void bfGfxCmdList_setAttachments(bfGfxCommandListHandle self, bfTextureHandle* attachments)
{
  const uint32_t num_attachments = self->renderpass->info.num_attachments;
  const uint64_t hash_code       = bifrost::vk::hash(0x0, attachments, num_attachments);

  bfFramebufferHandle fb = self->parent->cache_framebuffer.find(hash_code);

  if (!fb)
  {
    VkImageView image_views[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];

    fb = new bfFramebuffer();

    for (uint32_t i = 0; i < num_attachments; ++i)
    {
      fb->attachments[i] = attachments[i];
      image_views[i]     = attachments[i]->tex_view;
    }

    VkFramebufferCreateInfo frame_buffer_create_params;
    frame_buffer_create_params.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frame_buffer_create_params.pNext           = nullptr;
    frame_buffer_create_params.flags           = 0;
    frame_buffer_create_params.renderPass      = self->renderpass->handle;
    frame_buffer_create_params.attachmentCount = num_attachments;
    frame_buffer_create_params.pAttachments    = image_views;
    frame_buffer_create_params.width           = static_cast<uint32_t>(attachments[0]->image_width);
    frame_buffer_create_params.height          = static_cast<uint32_t>(attachments[0]->image_height);
    frame_buffer_create_params.layers          = static_cast<uint32_t>(attachments[0]->image_depth);

    const VkResult err = vkCreateFramebuffer(self->parent->handle, &frame_buffer_create_params, nullptr, &fb->handle);
    assert(err == VK_SUCCESS);

    self->parent->cache_framebuffer.insert(hash_code, fb);
  }

  self->framebuffer = fb;
}

void bfGfxCmdList_setRenderAreaAbs(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
  self->render_area.offset.x      = x;
  self->render_area.offset.y      = y;
  self->render_area.extent.width  = width;
  self->render_area.extent.height = height;
}

void bfGfxCmdList_setRenderAreaRel(bfGfxCommandListHandle self, float x, float y, float width, float height)
{
  const int32_t fb_width  = self->framebuffer->attachments[0]->image_width;
  const int32_t fb_height = self->framebuffer->attachments[0]->image_height;

  bfGfxCmdList_setRenderAreaAbs(
   self,
   int32_t(fb_width * x),
   int32_t(fb_height * y),
   uint32_t(fb_width * width),
   uint32_t(fb_height * height));
}

void bfGfxCmdList_beginRenderpass(bfGfxCommandListHandle self)
{
  VkRenderPassBeginInfo begin_render_pass_info;
  begin_render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin_render_pass_info.pNext           = nullptr;
  begin_render_pass_info.renderPass      = self->renderpass->handle;
  begin_render_pass_info.framebuffer     = self->framebuffer->handle;
  begin_render_pass_info.renderArea      = self->render_area;
  begin_render_pass_info.clearValueCount = self->renderpass->info.num_attachments;
  begin_render_pass_info.pClearValues    = self->clear_colors;

  vkCmdBeginRenderPass(self->handle, &begin_render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  self->pipeline_state.subpass_index = 0;
}

void bfGfxCmdList_nextSubpass(bfGfxCommandListHandle self)
{
  vkCmdNextSubpass(self->handle, VK_SUBPASS_CONTENTS_INLINE);
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
}

void bfGfxCmdList_setCullFace(bfGfxCommandListHandle self, BifrostCullFaceFlags cull_face)
{
  state(self).cull_face = cull_face;
}

void bfGfxCmdList_setDepthTesting(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_depth_test = value;
}

void bfGfxCmdList_setDepthWrite(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).depth_write = value;
}

void bfGfxCmdList_setDepthTestOp(bfGfxCommandListHandle self, BifrostCompareOp op)
{
  state(self).depth_test_op = op;
}

void bfGfxCmdList_setStencilTesting(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_stencil_test = value;
}

void bfGfxCmdList_setPrimitiveRestart(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).primitive_restart = value;
}

void bfGfxCmdList_setRasterizerDiscard(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).rasterizer_discard = value;
}

void bfGfxCmdList_setDepthBias(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).depth_bias = value;
}

void bfGfxCmdList_setSampleShading(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).sample_shading = value;
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
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_src = factor;
}

void bfGfxCmdList_setBlendDst(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_dst = factor;
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
void bfGfxCmdList_setStencilCompareOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op)
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
}

void bfGfxCmdList_setDynamicStates(bfGfxCommandListHandle self, uint16_t dynamic_states)
{
  auto& s = self->pipeline_state.state;

  s.dynamic_viewport           = dynamic_states & BIFROST_PIPELINE_DYNAMIC_VIEWPORT;
  s.dynamic_scissor            = dynamic_states & BIFROST_PIPELINE_DYNAMIC_SCISSOR;
  s.dynamic_line_width         = dynamic_states & BIFROST_PIPELINE_DYNAMIC_LINE_WIDTH;
  s.dynamic_depth_bias         = dynamic_states & BIFROST_PIPELINE_DYNAMIC_DEPTH_BIAS;
  s.dynamic_blend_constants    = dynamic_states & BIFROST_PIPELINE_DYNAMIC_BLEND_CONSTANTS;
  s.dynamic_depth_bounds       = dynamic_states & BIFROST_PIPELINE_DYNAMIC_DEPTH_BOUNDS;
  s.dynamic_stencil_cmp_mask   = dynamic_states & BIFROST_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK;
  s.dynamic_stencil_write_mask = dynamic_states & BIFROST_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK;
  s.dynamic_stencil_reference  = dynamic_states & BIFROST_PIPELINE_DYNAMIC_STENCIL_REFERENCE;
}

void bfGfxCmdList_setViewport(bfGfxCommandListHandle self, float x, float y, float width, float height, const float depth[2])
{
  auto& vp     = self->pipeline_state.viewport;
  vp.x         = x;
  vp.y         = y;
  vp.width     = width;
  vp.height    = height;
  vp.min_depth = depth[0];
  vp.max_depth = depth[1];
}

void bfGfxCmdList_setScissorRect(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
  auto& s = self->pipeline_state.scissor_rect;

  s.x      = x;
  s.y      = y;
  s.width  = width;
  s.height = height;
}

void bfGfxCmdList_setBlendConstants(bfGfxCommandListHandle self, const float constants[4])
{
  memcpy(self->pipeline_state.blend_constants, constants, sizeof(self->pipeline_state.blend_constants));
}

void bfGfxCmdList_setLineWidth(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.line_width = value;
}

void bfGfxCmdList_setDepthBiasConstantFactor(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_constant_factor = value;
}

void bfGfxCmdList_setDepthBiasClamp(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_clamp = value;
}

void bfGfxCmdList_setDepthBiasSlopeFactor(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_slope_factor = value;
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
  assert(num_buffers < 16);

  VkBuffer binded_buffers[16];

  for (uint32_t i = 0; i < num_buffers; ++i)
  {
    binded_buffers[i] = buffers[i]->handle;
  }

  vkCmdBindVertexBuffers(
   self->handle,
   binding,
   num_buffers,
   binded_buffers,
   offsets);
}

void bfGfxCmdList_bindIndexBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, uint64_t offset, BifrostIndexType idx_type)
{
  vkCmdBindIndexBuffer(self->handle, buffer->handle, offset, bfVkConvertIndexType(idx_type));
}

void bfGfxCmdList_bindProgram(bfGfxCommandListHandle self, bfShaderProgramHandle shader)
{
  self->pipeline_state.program = shader;
}

void bfGfxCmdList_bindDescriptorSets(bfGfxCommandListHandle self, uint32_t binding, bfDescriptorSetHandle* desc_sets, uint32_t num_desc_sets)
{
  assert(num_desc_sets <= BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS);
  VkDescriptorSet sets[BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS];

  const VkPipelineBindPoint bind_point = self->renderpass ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;

  assert(bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS && "Compute not fully supported yet.");

  for (uint32_t i = 0; i < num_desc_sets; ++i)
  {
    sets[i] = desc_sets[i]->handle;
  }

  vkCmdBindDescriptorSets(
   self->handle,
   bind_point,
   self->pipeline_state.program->layout,
   binding,
   num_desc_sets,
   sets,
   0,
   nullptr);
}

static void flushPipeline(bfGfxCommandListHandle self)
{
  assert(!"Not implemented");

  const uint64_t hash_code = bifrost::vk::hash(0x0, &self->pipeline_state);

  bfPipelineHandle pl = self->parent->cache_pipeline.find(hash_code);

  if (!pl)
  {
    // TODO(Shareef): Create Pipeline.

    pl = new bfPipeline();

    auto& state   = self->pipeline_state;
    auto& program = state.program;

    VkPipelineShaderStageCreateInfo shader_stages[BIFROST_SHADER_TYPE_MAX];

    for (size_t i = 0; i < program->modules.size; ++i)
    {
      bfShaderModuleHandle shader_module = program->modules.elements[i];
      auto&                shader_stage  = shader_stages[i];
      shader_stage.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shader_stage.pNext                 = nullptr;
      shader_stage.flags                 = 0x0;
      shader_stage.stage                 = bfVkConvertShaderType(shader_module->type);
      shader_stage.module                = shader_module->handle;
      shader_stage.pName                 = shader_module->entry_point;
      shader_stage.pSpecializationInfo   = nullptr;  // https://www.jeremyong.com/c++/vulkan/graphics/rendering/2018/04/20/putting-cpp-to-work-making-abstractions-for-vulkan-specialization-info/
    }

    VkPipelineVertexInputStateCreateInfo vertex_input;
    vertex_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.pNext                           = nullptr;
    vertex_input.flags                           = 0x0;
    vertex_input.vertexBindingDescriptionCount   = state.vertex_set_layout->num_buffer_bindings;
    vertex_input.pVertexBindingDescriptions      = state.vertex_set_layout->buffer_bindings;
    vertex_input.vertexAttributeDescriptionCount = state.vertex_set_layout->num_attrib_bindings;
    vertex_input.pVertexAttributeDescriptions    = state.vertex_set_layout->attrib_bindings;

    // TODO
    //
    // VkPipelineInputAssemblyStateCreateFlags flags;
    // VkPrimitiveTopology                     topology;
    // VkBool32                                primitiveRestartEnable;
    //
    VkPipelineInputAssemblyStateCreateInfo input_asm;
    input_asm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm.pNext = nullptr;

    // TODO
    //
    // VkStructureType                        sType;
    // const void*                            pNext;
    // VkPipelineTessellationStateCreateFlags flags;
    // uint32_t                               patchControlPoints;
    //
    VkPipelineTessellationStateCreateInfo tess;
    tess.pNext = nullptr;

    // https://erkaman.github.io/posts/tess_opt.html
    // https://computergraphics.stackexchange.com/questions/7955/why-are-tessellation-shaders-disliked

    // TODO
    //
    // VkPipelineViewportStateCreateFlags flags;
    // uint32_t                           viewportCount;
    // const VkViewport*                  pViewports;
    // uint32_t                           scissorCount;
    // const VkRect2D*                    pScissors;
    //
    VkPipelineViewportStateCreateInfo viewport;
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.pNext = nullptr;


    // TODO
    //
    // VkPipelineRasterizationStateCreateFlags flags;
    // VkBool32                                depthClampEnable;
    // VkBool32                                rasterizerDiscardEnable;
    // VkPolygonMode                           polygonMode;
    // VkCullModeFlags                         cullMode;
    // VkFrontFace                             frontFace;
    // VkBool32                                depthBiasEnable;
    // float                                   depthBiasConstantFactor;
    // float                                   depthBiasClamp;
    // float                                   depthBiasSlopeFactor;
    // float                                   lineWidth;
    //
    VkPipelineRasterizationStateCreateInfo rasterization;
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.pNext = nullptr;


    // TODO
    //
    // VkPipelineMultisampleStateCreateFlags flags;
    // VkSampleCountFlagBits                 rasterizationSamples;
    // VkBool32                              sampleShadingEnable;
    // float                                 minSampleShading;
    // const VkSampleMask*                   pSampleMask;
    // VkBool32                              alphaToCoverageEnable;
    // VkBool32                              alphaToOneEnable;
    //
    VkPipelineMultisampleStateCreateInfo multisample;
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.pNext = nullptr;


    // TODO
    //
    // VkPipelineDepthStencilStateCreateFlags flags;
    // VkBool32                               depthTestEnable;
    // VkBool32                               depthWriteEnable;
    // VkCompareOp                            depthCompareOp;
    // VkBool32                               depthBoundsTestEnable;
    // VkBool32                               stencilTestEnable;
    // VkStencilOpState                       front;
    // VkStencilOpState                       back;
    // float                                  minDepthBounds;
    // float                                  maxDepthBounds;
    //
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    depth_stencil.pNext = nullptr;


    // TODO
    //
    // VkStructureType                            sType;
    // VkPipelineColorBlendStateCreateFlags       flags;
    // VkBool32                                   logicOpEnable;
    // VkLogicOp                                  logicOp;
    // uint32_t                                   attachmentCount;
    // const VkPipelineColorBlendAttachmentState* pAttachments;
    // float                                      blendConstants[4];
    //
    VkPipelineColorBlendStateCreateInfo   color_blend;
    color_blend.pNext = nullptr;


    // TODO
    //
    // VkPipelineDynamicStateCreateFlags flags;
    // uint32_t                          dynamicStateCount;
    // const VkDynamicState*             pDynamicStates;
    //
    VkPipelineDynamicStateCreateInfo      dynamic_state;
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = nullptr;


    // TODO(SR): Look into pipeline derivatives?
    //   @PipelineDerivative

    VkGraphicsPipelineCreateInfo pl_create_info;
    pl_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pl_create_info.pNext               = nullptr;
    pl_create_info.flags               = 0x0;  // @PipelineDerivative
    pl_create_info.stageCount          = program->modules.size;
    pl_create_info.pStages             = shader_stages;
    pl_create_info.pVertexInputState   = &vertex_input;
    pl_create_info.pInputAssemblyState = &input_asm;
    pl_create_info.pTessellationState  = &tess;
    pl_create_info.pViewportState      = &viewport;
    pl_create_info.pRasterizationState = &rasterization;
    pl_create_info.pMultisampleState   = &multisample;
    pl_create_info.pDepthStencilState  = &depth_stencil;
    pl_create_info.pColorBlendState    = &color_blend;
    pl_create_info.pDynamicState       = &dynamic_state;
    pl_create_info.layout              = program->layout;
    pl_create_info.renderPass          = self->renderpass->handle;
    pl_create_info.subpass             = state.subpass_index;
    pl_create_info.basePipelineHandle  = VK_NULL_HANDLE;  // @PipelineDerivative
    pl_create_info.basePipelineIndex   = -1;              // @PipelineDerivative

    VkResult err = vkCreateGraphicsPipelines(
     self->parent->handle,
     VK_NULL_HANDLE,
     1,
     &pl_create_info,
     CUSTOM_ALLOC,
     &pl->handle);
    assert(err == VK_SUCCESS);
  }

  if (pl != self->pipeline)
  {
    vkCmdBindPipeline(self->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline->handle);
    self->pipeline = pl;
  }
}

void bfGfxCmdList_draw(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices)
{
  bfGfxCmdList_drawInstanced(self, first_vertex, num_vertices, 0, 1);
}

void bfGfxCmdList_drawInstanced(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices, uint32_t first_instance, uint32_t num_instances)
{
  flushPipeline(self);
  vkCmdDraw(self->handle, num_vertices, num_instances, first_vertex, first_instance);
}

void bfGfxCmdList_drawIndexed(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset)
{
  bfGfxCmdList_drawIndexedInstanced(self, num_indices, index_offset, vertex_offset, 0, 1);
}

void bfGfxCmdList_drawIndexedInstanced(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset, uint32_t first_instance, uint32_t num_instances)
{
  flushPipeline(self);
  vkCmdDrawIndexed(self->handle, num_indices, num_instances, index_offset, vertex_offset, first_instance);
}

void bfGfxCmdList_executeSubCommands(bfGfxCommandListHandle self, bfGfxCommandListHandle* commands, uint32_t num_commands)
{
  assert(!"Not implemented");
}

void bfGfxCmdList_endRenderpass(bfGfxCommandListHandle self)
{
  vkCmdEndRenderPass(self->handle);
}

void bfGfxCmdList_end(bfGfxCommandListHandle self)
{
  VkResult err = vkEndCommandBuffer(self->handle);

  if (err)
  {
    assert(false);
  }
}

void bfGfxCmdList_updateBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size, const void* data)
{
  vkCmdUpdateBuffer(self->handle, buffer->handle, offset, size, data);
}

// TODO(SR): Make a new file
#include "bifrost/utility/bifrost_hash.hpp"

namespace bifrost::vk
{
  static void hash(std::uint64_t& self, const BifrostViewport& vp)
  {
    self = hash::addF32(self, vp.x);
    self = hash::addF32(self, vp.y);
    self = hash::addF32(self, vp.width);
    self = hash::addF32(self, vp.height);
    self = hash::addF32(self, vp.min_depth);
    self = hash::addF32(self, vp.max_depth);
  }

  static void hash(std::uint64_t& self, const BifrostScissorRect& scissor)
  {
    self = hash::addS32(self, scissor.x);
    self = hash::addS32(self, scissor.y);
    self = hash::addU32(self, scissor.width);
    self = hash::addU32(self, scissor.height);
  }

  static void hash(std::uint64_t& self, const BifrostPipelineDepthInfo& depth)
  {
    self = hash::addF32(self, depth.bias_constant_factor);
    self = hash::addF32(self, depth.bias_clamp);
    self = hash::addF32(self, depth.bias_slope_factor);
  }

  static void hash(std::uint64_t& self, const bfFramebufferBlending& fb_blending)
  {
    self = hash::addU32(self, *(uint32_t*)&fb_blending);
  }

  std::uint64_t hash(std::uint64_t self, const bfPipelineCache* pipeline)
  {
    const auto* state = reinterpret_cast<const std::uint64_t*>(&pipeline->state);

    const auto num_attachments = pipeline->renderpass->info.subpasses[pipeline->subpass_index].num_out_attachment_refs;

    self = hash::addU64(self, state[0]);
    self = hash::addU64(self, state[1]);
    hash(self, pipeline->viewport);
    hash(self, pipeline->scissor_rect);

    for (float blend_constant : pipeline->blend_constants)
    {
      self = hash::addF32(self, blend_constant);
    }

    self = hash::addF32(self, pipeline->line_width);
    hash(self, pipeline->depth);
    self = hash::addF32(self, pipeline->min_sample_shading);
    self = hash::addU64(self, pipeline->sample_mask);
    self = hash::addU32(self, pipeline->subpass_index);
    self = hash::addU32(self, num_attachments);

    for (std::uint32_t i = 0; i < num_attachments; ++i)
    {
      hash(self, pipeline->blending[i]);
    }

    self = hash::addPointer(self, pipeline->program);
    self = hash::addPointer(self, pipeline->renderpass);
    self = hash::addPointer(self, pipeline->vertex_set_layout);

    return self;
  }

  std::uint64_t hash(std::uint64_t self, bfTextureHandle* attachments, std::size_t num_attachments)
  {
    for (std::size_t i = 0; i < num_attachments; ++i)
    {
      self = hash::addPointer(self, attachments[i]);
    }

    return self;
  }

  std::uint64_t hash(std::uint64_t self, const bfRenderpassInfo* renderpass_info)
  {
    self = hash::addU32(self, renderpass_info->load_ops);
    self = hash::addU32(self, renderpass_info->stencil_load_ops);
    self = hash::addU32(self, renderpass_info->clear_ops);
    self = hash::addU32(self, renderpass_info->stencil_clear_ops);
    self = hash::addU32(self, renderpass_info->store_ops);
    self = hash::addU32(self, renderpass_info->stencil_store_ops);
    self = hash::addU32(self, renderpass_info->num_subpasses);

    for (uint16_t i = 0; i < renderpass_info->num_subpasses; ++i)
    {
      self = hash(self, renderpass_info->subpasses + i);
    }

    self = hash::addU32(self, renderpass_info->num_attachments);

    for (uint32_t i = 0; i < renderpass_info->num_attachments; ++i)
    {
      self = hash(self, renderpass_info->attachments + i);
    }

    return self;
  }

  std::uint64_t hash(std::uint64_t self, const bfAttachmentInfo* attachment_info)
  {
    self = hash::addPointer(self, attachment_info->texture);
    self = hash::addU32(self, attachment_info->final_layout);
    self = hash::addU32(self, attachment_info->may_alias);

    return self;
  }

  std::uint64_t hash(std::uint64_t self, const bfSubpassCache* subpass_info)
  {
    self = hash::addU32(self, subpass_info->num_out_attachment_refs);

    for (uint32_t i = 0; i < subpass_info->num_out_attachment_refs; ++i)
    {
      self = hash(self, subpass_info->out_attachment_refs + i);
    }

    self = hash::addU32(self, subpass_info->num_in_attachment_refs);

    for (uint32_t i = 0; i < subpass_info->num_in_attachment_refs; ++i)
    {
      self = hash(self, subpass_info->in_attachment_refs + i);
    }

    self = hash(self, &subpass_info->depth_attachment);

    return self;
  }

  std::uint64_t hash(std::uint64_t self, const bfAttachmentRefCache* attachment_ref_info)
  {
    self = hash::addU32(self, attachment_ref_info->attachment_index);
    self = hash::addU32(self, attachment_ref_info->layout);

    return self;
  }
}  // namespace bifrost::vk
