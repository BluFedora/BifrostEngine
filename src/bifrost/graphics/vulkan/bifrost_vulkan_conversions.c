#include "bifrost_vulkan_conversions.h"

#include <assert.h> /* assert */

VkFormat bfVkConvertFormat(BifrostImageFormat format)
{
  return (VkFormat)format;
}

VkImageLayout bfVkConvertImgLayout(BifrostImageLayout layout)
{
  return (VkImageLayout)layout;
}

VkSampleCountFlags bfVkConvertSampleFlags(uint32_t flags)
{
  return (VkSampleCountFlags)flags;
}

VkSampleCountFlagBits bfVkConvertSampleCount(BifrostSampleFlags bit)
{
  switch (bit)
  {
    case BIFROST_SAMPLE_1: return VK_SAMPLE_COUNT_1_BIT;
    case BIFROST_SAMPLE_2: return VK_SAMPLE_COUNT_2_BIT;
    case BIFROST_SAMPLE_4: return VK_SAMPLE_COUNT_4_BIT;
    case BIFROST_SAMPLE_8: return VK_SAMPLE_COUNT_8_BIT;
    case BIFROST_SAMPLE_16: return VK_SAMPLE_COUNT_16_BIT;
    case BIFROST_SAMPLE_32: return VK_SAMPLE_COUNT_32_BIT;
    case BIFROST_SAMPLE_64: return VK_SAMPLE_COUNT_64_BIT;
  }

  assert(0);
  return VK_SAMPLE_COUNT_1_BIT;
}

VkClearValue bfVKConvertClearColor(const BifrostClearValue* color)
{
  VkClearValue ret;

  ret.color.uint32[0] = color->color.uint32[0];
  ret.color.uint32[1] = color->color.uint32[1];
  ret.color.uint32[2] = color->color.uint32[2];
  ret.color.uint32[3] = color->color.uint32[3];

  return ret;
}

VkIndexType bfVkConvertIndexType(BifrostIndexType idx_type)
{
  if (idx_type == BIFROST_INDEX_TYPE_UINT16)
  {
    return VK_INDEX_TYPE_UINT16;
  }

  return VK_INDEX_TYPE_UINT32;
}

VkShaderStageFlagBits bfVkConvertShaderType(BifrostShaderType type)
{
  switch (type)
  {
    case BIFROST_SHADER_TYPE_VERTEX: return VK_SHADER_STAGE_VERTEX_BIT;
    case BIFROST_SHADER_TYPE_TESSELLATION_CONTROL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case BIFROST_SHADER_TYPE_TESSELLATION_EVALUATION: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case BIFROST_SHADER_TYPE_GEOMETRY: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case BIFROST_SHADER_TYPE_FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case BIFROST_SHADER_TYPE_COMPUTE:
      return VK_SHADER_STAGE_COMPUTE_BIT;
      // case BIFROST_SHADER_TYPE_MAX: assert(0);
  }

  assert(0);
  return VK_SHADER_STAGE_ALL;
}

VkPrimitiveTopology bfVkConvertTopology(BifrostDrawMode draw_mode)
{
  switch (draw_mode)
  {
    case BIFROST_DRAW_MODE_POINT_LIST: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case BIFROST_DRAW_MODE_LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case BIFROST_DRAW_MODE_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case BIFROST_DRAW_MODE_TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case BIFROST_DRAW_MODE_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case BIFROST_DRAW_MODE_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
  }

  assert(0);
  return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}

VkViewport bfVkConvertViewport(const BifrostViewport* viewport)
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

VkRect2D bfVkConvertScissorRect(const BifrostScissorRect* viewport)
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
    case BIFROST_POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
    case BIFROST_POLYGON_MODE_LINE: return VK_POLYGON_MODE_LINE;
    case BIFROST_POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT;
  }

  assert(0);
  return VK_POLYGON_MODE_FILL;
}
