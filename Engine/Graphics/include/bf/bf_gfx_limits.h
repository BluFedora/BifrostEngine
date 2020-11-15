/******************************************************************************/
/*!
 * @file   bf_gfx_limits.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Various limits used within the renderer to simplify implemetation details.
 *
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BF_GFX_LIMITS_H
#define BF_GFX_LIMITS_H

#if __cplusplus
extern "C" {
#endif

/*!
 * @brief
 *  Hardware Limitations Overview\n\n
 *
 *  Portable Vulkan:      4 DescriptorSets\n
 *  Mobile:               4 DescriptorSets\n
 *  Older Graphics cards: 4 DescriptorSets\n
 *  Newer Graphics cards: 8 DescriptorSets\n\n
 *
 *  Suggested Layout:\n
 *    [0] Camera / Scene\n
 *    [1] General Shader Params\n
 *    [2] Materials\n
 *    [3] Object Transforms\n
 */
enum
{
  k_bfGfxMaxSubpasses                    = 4,
  k_bfGfxMaxAttachments                  = 8,
  k_bfGfxMaxRenderpassDependencies       = k_bfGfxMaxSubpasses,
  k_bfGfxDescriptorSets                  = 4,
  k_bfGfxShaderEntryPointNameLength      = 32,
  k_bfGfxShaderProgramNameLength         = 32,
  k_bfGfxMaxLayoutBindings               = 16,
  k_bfGfxDesfcriptorSetMaxLayoutBindings = 16,
  k_bfGfxMaxBufferBindings               = 16,
  k_bfGfxMaxDescriptorSetWrites          = 32,
  k_bfGfxMaxFrameGPUAhead                = 4,
  k_bfGfxMaxPipelineBarrierWrites        = 16,
};

/*
  [x]   static const unsigned VULKAN_NUM_BINDINGS        = 16;
  [x]   static const unsigned VULKAN_NUM_VERTEX_ATTRIBS  = 16;
  [ ]   static const unsigned VULKAN_NUM_VERTEX_BUFFERS  = 4;
  [ ]   static const unsigned VULKAN_PUSH_CONSTANT_SIZE  = 128;
  [ ]   static const unsigned VULKAN_UBO_SIZE            = 16 * 1024;
  [ ]   static const unsigned VULKAN_NUM_SPEC_CONSTANTS  = 8;
  [ ]   static const unsigned VULKAN_NUM_DYNAMIC_UNIFORM_BUFFEFERS  = 8;
 */

#if __cplusplus
}
#endif

#endif /* BF_GFX_LIMITS_H */
