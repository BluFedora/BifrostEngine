#include "bf_vulkan_conversions.h"

#include <assert.h> /* assert */

VkFormat bfVkConvertFormat(bfGfxImageFormat format)
{
  return (VkFormat)format;
}

VkImageLayout bfVkConvertImgLayout(bfGfxImageLayout layout)
{
  return (VkImageLayout)layout;
}

VkSampleCountFlags bfVkConvertSampleFlags(uint32_t flags)
{
  return (VkSampleCountFlags)flags;
}

VkSampleCountFlagBits bfVkConvertSampleCount(bfGfxSampleFlags bit)
{
  switch (bit)
  {
    case BF_SAMPLE_1: return VK_SAMPLE_COUNT_1_BIT;
    case BF_SAMPLE_2: return VK_SAMPLE_COUNT_2_BIT;
    case BF_SAMPLE_4: return VK_SAMPLE_COUNT_4_BIT;
    case BF_SAMPLE_8: return VK_SAMPLE_COUNT_8_BIT;
    case BF_SAMPLE_16: return VK_SAMPLE_COUNT_16_BIT;
    case BF_SAMPLE_32: return VK_SAMPLE_COUNT_32_BIT;
    case BF_SAMPLE_64: return VK_SAMPLE_COUNT_64_BIT;
  }

  assert(0);
  return VK_SAMPLE_COUNT_1_BIT;
}

VkClearValue bfVKConvertClearColor(const bfClearValue* color)
{
  VkClearValue ret;

  ret.color.uint32[0] = color->color.uint32[0];
  ret.color.uint32[1] = color->color.uint32[1];
  ret.color.uint32[2] = color->color.uint32[2];
  ret.color.uint32[3] = color->color.uint32[3];

  return ret;
}

VkIndexType bfVkConvertIndexType(bfGfxIndexType idx_type)
{
  return idx_type == BF_INDEX_TYPE_UINT16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
}

VkShaderStageFlagBits bfVkConvertShaderType(bfShaderType type)
{
  switch (type)
  {
    case BF_SHADER_TYPE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
    case BF_SHADER_TYPE_TESSELLATION_CONTROL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case BF_SHADER_TYPE_TESSELLATION_EVALUATION: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case BF_SHADER_TYPE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case BF_SHADER_TYPE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case BF_SHADER_TYPE_COMPUTE:
      return VK_SHADER_STAGE_COMPUTE_BIT;
      // case BF_SHADER_TYPE_MAX: assert(0);
  }

  assert(0);
  return VK_SHADER_STAGE_ALL;
}

VkShaderStageFlagBits bfVkConvertShaderStage(BifrostShaderStageBits flags)
{
  VkShaderStageFlagBits result = 0x0;

  if (flags & BF_SHADER_STAGE_VERTEX)
  {
    result |= VK_SHADER_STAGE_VERTEX_BIT;
  }

  if (flags & BF_SHADER_STAGE_TESSELLATION_CONTROL)
  {
    result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
  }

  if (flags & BF_SHADER_STAGE_TESSELLATION_EVALUATION)
  {
    result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
  }

  if (flags & BF_SHADER_STAGE_GEOMETRY)
  {
    result |= VK_SHADER_STAGE_GEOMETRY_BIT;
  }

  if (flags & BF_SHADER_STAGE_FRAGMENT)
  {
    result |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  if (flags & BF_SHADER_STAGE_COMPUTE)
  {
    result |= VK_SHADER_STAGE_COMPUTE_BIT;
  }

  return result;
}

VkPrimitiveTopology bfVkConvertTopology(bfDrawMode draw_mode)
{
  switch (draw_mode)
  {
    case BF_DRAW_MODE_POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case BF_DRAW_MODE_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case BF_DRAW_MODE_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case BF_DRAW_MODE_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case BF_DRAW_MODE_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case BF_DRAW_MODE_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
  }

  assert(0);
  return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}

VkViewport bfVkConvertViewport(const bfViewport* viewport)
{
  VkViewport ret;

  ret.x        = viewport->x;
  ret.y        = viewport->y;
  ret.width    = viewport->width;
  ret.height   = viewport->height;
  ret.minDepth = viewport->min_depth;
  ret.maxDepth = viewport->max_depth;

  return ret;
}

VkRect2D bfVkConvertScissorRect(const bfScissorRect* viewport)
{
  VkRect2D ret;

  ret.offset.x      = viewport->x;
  ret.offset.y      = viewport->y;
  ret.extent.width  = viewport->width;
  ret.extent.height = viewport->height;

  return ret;
}

VkPolygonMode bfVkConvertPolygonMode(BifrostPolygonFillMode polygon_mode)
{
  switch (polygon_mode)
  {
    case BF_POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
    case BF_POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
    case BF_POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT;
  }

  assert(0);
  return VK_POLYGON_MODE_FILL;
}

VkCullModeFlags bfVkConvertCullModeFlags(uint32_t cull_face_flags)
{
  VkCullModeFlags flags = VK_CULL_MODE_NONE;

  if (cull_face_flags & BF_CULL_FACE_FRONT)
  {
    flags |= VK_CULL_MODE_FRONT_BIT;
  }

  if (cull_face_flags & BF_CULL_FACE_BACK)
  {
    flags |= VK_CULL_MODE_BACK_BIT;
  }

  return flags;
}

VkFrontFace bfVkConvertFrontFace(bfFrontFace front_face)
{
  switch (front_face)
  {
    case BF_FRONT_FACE_CCW: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    case BF_FRONT_FACE_CW: return VK_FRONT_FACE_CLOCKWISE;
  }

  assert(0);
  return VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

VkFormat bfVkConvertVertexFormatAttrib(bfGfxVertexFormatAttribute vertex_format_attrib)
{
  switch (vertex_format_attrib)
  {
    case BF_VFA_FLOAT32_4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case BF_VFA_FLOAT32_3: return VK_FORMAT_R32G32B32_SFLOAT;
    case BF_VFA_FLOAT32_2: return VK_FORMAT_R32G32_SFLOAT;
    case BF_VFA_FLOAT32_1: return VK_FORMAT_R32_SFLOAT;
    case BF_VFA_UINT32_4: return VK_FORMAT_R32G32B32A32_UINT;
    case BF_VFA_UINT32_3: return VK_FORMAT_R32G32B32_UINT;
    case BF_VFA_UINT32_2: return VK_FORMAT_R32G32_UINT;
    case BF_VFA_UINT32_1: return VK_FORMAT_R32_UINT;
    case BF_VFA_SINT32_4: return VK_FORMAT_R32G32B32A32_SINT;
    case BF_VFA_SINT32_3: return VK_FORMAT_R32G32B32_SINT;
    case BF_VFA_SINT32_2: return VK_FORMAT_R32G32_SINT;
    case BF_VFA_SINT32_1: return VK_FORMAT_R32_SINT;
    case BF_VFA_USHORT16_4: return VK_FORMAT_R16G16B16A16_UINT;
    case BF_VFA_USHORT16_3: return VK_FORMAT_R16G16B16_UINT;
    case BF_VFA_USHORT16_2: return VK_FORMAT_R16G16_UINT;
    case BF_VFA_USHORT16_1: return VK_FORMAT_R16_UINT;
    case BF_VFA_SSHORT16_4: return VK_FORMAT_R16G16B16A16_SINT;
    case BF_VFA_SSHORT16_3: return VK_FORMAT_R16G16B16_SINT;
    case BF_VFA_SSHORT16_2: return VK_FORMAT_R16G16_SINT;
    case BF_VFA_SSHORT16_1: return VK_FORMAT_R16_SINT;
    case BF_VFA_UCHAR8_4: return VK_FORMAT_R8G8B8A8_UINT;
    case BF_VFA_UCHAR8_3: return VK_FORMAT_R8G8B8_UINT;
    case BF_VFA_UCHAR8_2: return VK_FORMAT_R8G8_UINT;
    case BF_VFA_UCHAR8_1: return VK_FORMAT_R8_UINT;
    case BF_VFA_SCHAR8_4: return VK_FORMAT_R8G8B8A8_SINT;
    case BF_VFA_SCHAR8_3: return VK_FORMAT_R8G8B8_SINT;
    case BF_VFA_SCHAR8_2: return VK_FORMAT_R8G8_SINT;
    case BF_VFA_SCHAR8_1: return VK_FORMAT_R8_SINT;
    case BF_VFA_UCHAR8_4_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
      bfInvalidDefaultCase();
      return VK_FORMAT_R32G32B32A32_SFLOAT;
  }
}

VkFlags bfVkConvertBufferUsageFlags(uint16_t flags)
{
  const VkFlags result = flags & ~BF_BUFFER_USAGE_PERSISTENTLY_MAPPED_BUFFER;

  return result;
}

VkFlags bfVkConvertBufferPropertyFlags(uint16_t flags)
{
  VkFlags result = 0x0;

  if (flags & BF_BUFFER_PROP_DEVICE_LOCAL) result |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (flags & BF_BUFFER_PROP_HOST_MAPPABLE) result |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  if (flags & BF_BUFFER_PROP_HOST_CACHE_MANAGED) result |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (flags & BF_BUFFER_PROP_HOST_CACHED) result |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
  if (flags & BF_BUFFER_PROP_DEVICE_LAZY_ALLOC) result |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
  if (flags & BF_BUFFER_PROP_PROTECTED) result |= VK_MEMORY_PROPERTY_PROTECTED_BIT;

  return result;
}

VkImageType bfVkConvertTextureType(bfTextureType type)
{
  switch (type)
  {
    case BF_TEX_TYPE_1D: return VK_IMAGE_TYPE_1D;
    case BF_TEX_TYPE_2D: return VK_IMAGE_TYPE_2D;
    case BF_TEX_TYPE_3D: return VK_IMAGE_TYPE_3D;
  }

  assert(0);
  return VK_IMAGE_TYPE_1D;
}

VkFilter bfVkConvertSamplerFilterMode(bfTexSamplerFilterMode mode)
{
  switch (mode)
  {
    case BF_SFM_NEAREST: return VK_FILTER_NEAREST;
    case BF_SFM_LINEAR: return VK_FILTER_LINEAR;
  }

  assert(0);
  return VK_FILTER_LINEAR;
}

VkSamplerAddressMode bfVkConvertSamplerAddressMode(bfTexSamplerAddressMode mode)
{
  switch (mode)
  {
    case BF_SAM_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case BF_SAM_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case BF_SAM_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case BF_SAM_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case BF_SAM_MIRROR_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
  }

  assert(0);
  return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkCompareOp bfVkConvertCompareOp(bfCompareOp op)
{
  switch (op)
  {
    case BF_COMPARE_OP_NEVER: return VK_COMPARE_OP_NEVER;
    case BF_COMPARE_OP_LESS_THAN: return VK_COMPARE_OP_LESS;
    case BF_COMPARE_OP_EQUAL: return VK_COMPARE_OP_EQUAL;
    case BF_COMPARE_OP_LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case BF_COMPARE_OP_GREATER: return VK_COMPARE_OP_GREATER;
    case BF_COMPARE_OP_NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
    case BF_COMPARE_OP_GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case BF_COMPARE_OP_ALWAYS: return VK_COMPARE_OP_ALWAYS;
  }

  assert(0);
  return VK_COMPARE_OP_NEVER;
}

VkStencilOp bfVkConvertStencilOp(bfStencilOp op)
{
#define CONVERT(X) \
  case BF_STENCIL_OP_##X: return VK_STENCIL_OP_##X

  switch (op)
  {
    CONVERT(KEEP);
    CONVERT(ZERO);
    CONVERT(REPLACE);
    CONVERT(INCREMENT_AND_CLAMP);
    CONVERT(DECREMENT_AND_CLAMP);
    CONVERT(INVERT);
    CONVERT(INCREMENT_AND_WRAP);
    CONVERT(DECREMENT_AND_WRAP);
  }

#undef CONVERT

  assert(0);
  return VK_STENCIL_OP_KEEP;
}

VkLogicOp bfVkConvertLogicOp(bfLogicOp op)
{
#define CONVERT(X)           \
  case BF_LOGIC_OP_##X: \
    return VK_LOGIC_OP_##X

#define CONVERT2(Y, X) \
  case BF_LOGIC_OP_##Y: return VK_LOGIC_OP_##X

  switch (op)
  {
    CONVERT(CLEAR);
    CONVERT(AND);
    CONVERT2(AND_REV, AND_REVERSE);
    CONVERT(COPY);
    CONVERT2(AND_INV, AND_INVERTED);
    CONVERT2(NONE, NO_OP);
    CONVERT(XOR);
    CONVERT(OR);
    CONVERT(NOR);
    CONVERT(EQUIVALENT);
    CONVERT2(INV, INVERT);
    CONVERT2(OR_REV, OR_REVERSE);
    CONVERT2(COPY_INV, COPY_INVERTED);
    CONVERT2(OR_INV, OR_INVERTED);
    CONVERT(NAND);
    CONVERT(SET);
  }

#undef CONVERT
#undef CONVERT2

  assert(0);
  return VK_LOGIC_OP_CLEAR;
}

VkBlendFactor bfVkConvertBlendFactor(bfBlendFactor factor)
{
#define CONVERT(X)               \
  case BF_BLEND_FACTOR_##X: \
    return VK_BLEND_FACTOR_##X

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
    CONVERT(SRC1_COLOR);
    CONVERT(ONE_MINUS_SRC1_COLOR);
    CONVERT(SRC1_ALPHA);
    CONVERT(ONE_MINUS_SRC1_ALPHA);
    case BF_BLEND_FACTOR_NONE: break;
  }

#undef CONVERT

  assert(0);
  return VK_BLEND_FACTOR_ZERO;
}

VkBlendOp bfVkConvertBlendOp(bfBlendOp factor)
{
  switch (factor)
  {
    case BF_BLEND_OP_ADD: return VK_BLEND_OP_ADD;
    case BF_BLEND_OP_SUB: return VK_BLEND_OP_SUBTRACT;
    case BF_BLEND_OP_REV_SUB: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BF_BLEND_OP_MIN: return VK_BLEND_OP_MIN;
    case BF_BLEND_OP_MAX: return VK_BLEND_OP_MAX;
  }

  assert(0);
  return VK_BLEND_OP_ADD;
}

static void ifAndSet(uint32_t* result, uint32_t flags_in, uint32_t bifrost_flag, uint32_t vulkan_flag)
{
  if (flags_in & bifrost_flag)
  {
    *result |= vulkan_flag;
  }
}

VkFlags bfVkConvertColorMask(uint16_t flags)
{
  VkFlags result = 0x0;

  ifAndSet(&result, flags, BF_COLOR_MASK_R, VK_COLOR_COMPONENT_R_BIT);
  ifAndSet(&result, flags, BF_COLOR_MASK_G, VK_COLOR_COMPONENT_G_BIT);
  ifAndSet(&result, flags, BF_COLOR_MASK_B, VK_COLOR_COMPONENT_B_BIT);
  ifAndSet(&result, flags, BF_COLOR_MASK_A, VK_COLOR_COMPONENT_A_BIT);

  return result;
}

VkPipelineStageFlags bfVkConvertPipelineStageFlags(bfGfxPipelineStageBits flags)
{
  VkPipelineStageFlags result = 0x0;

  ifAndSet(&result, flags, BF_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
  ifAndSet(&result, flags, BF_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  return result;
}

VkAccessFlags bfVkConvertAccessFlags(bfGfxAccessFlagsBits flags)
{
  VkAccessFlags result = 0x0;

  ifAndSet(&result, flags, BF_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_INDEX_READ_BIT, VK_ACCESS_INDEX_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_UNIFORM_READ_BIT, VK_ACCESS_UNIFORM_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_INPUT_ATTACHMENT_READ_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT);
  ifAndSet(&result, flags, BF_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
  ifAndSet(&result, flags, BF_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
  ifAndSet(&result, flags, BF_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
  ifAndSet(&result, flags, BF_ACCESS_HOST_READ_BIT, VK_ACCESS_HOST_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_HOST_WRITE_BIT, VK_ACCESS_HOST_WRITE_BIT);
  ifAndSet(&result, flags, BF_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_READ_BIT);
  ifAndSet(&result, flags, BF_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT);

  return result;
}

uint32_t bfConvertQueueIndex(const uint32_t queue_list[BF_GFX_QUEUE_MAX], bfGfxQueueType type)
{
  switch (type)
  {
    case BF_GFX_QUEUE_GRAPHICS:
    case BF_GFX_QUEUE_COMPUTE:
    case BF_GFX_QUEUE_TRANSFER:
    case BF_GFX_QUEUE_PRESENT:
      return queue_list[type];
    case BF_GFX_QUEUE_IGNORE:
      return VK_QUEUE_FAMILY_IGNORED;
    case BF_GFX_QUEUE_MAX:
    default:
      assert(!"Invalid queue type");
      break;
  }

  return -1;
}

// Internal API

VkImageView bfCreateImageView(
 VkDevice           device,
 VkImage            image,
 VkImageViewType    view_type,
 VkFormat           format,
 VkImageAspectFlags aspect_flags,
 uint32_t           base_mip_level,
 uint32_t           base_array_layer,
 uint32_t           mip_levels,
 uint32_t           layer_count)
{
  VkImageViewCreateInfo viewInfo           = {0};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = view_type;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspect_flags;
  viewInfo.subresourceRange.baseMipLevel   = base_mip_level;
  viewInfo.subresourceRange.baseArrayLayer = base_array_layer;
  viewInfo.subresourceRange.levelCount     = mip_levels;
  viewInfo.subresourceRange.layerCount     = layer_count;

  VkImageView img_view;

  const VkResult error = vkCreateImageView(device, &viewInfo, NULL, &img_view);
  assert(error == VK_SUCCESS);

  return img_view;
}

VkImageView bfCreateImageView2D(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels)
{
  return bfCreateImageView(
   device,
   image,
   VK_IMAGE_VIEW_TYPE_2D,
   format,
   aspect_flags,
   0,
   0,
   mip_levels,
   1);
}
