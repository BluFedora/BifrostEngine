/******************************************************************************/
/*!
 * @file   bf_gfx_limits.h
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Various limits used within the renderer to simplify implementation details.
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
 *  Hardware Limitations Overview          \n\n
 *
 *  Portable Vulkan:      4 DescriptorSets \n
 *  Mobile:               4 DescriptorSets \n
 *  Older Graphics cards: 4 DescriptorSets \n
 *  Newer Graphics cards: 8 DescriptorSets \n\n
 *
 *  Suggested Layout:           \n
 *    [0] Camera / Scene        \n
 *    [1] General Shader Params \n
 *    [2] Materials             \n
 *    [3] Object Transforms     \n
 */
enum /* bfGraphicsConstants */
{
  k_bfGfxMaxSubpasses                    = 4,
  k_bfGfxMaxAttachments                  = 8,
  k_bfGfxMaxRenderpassDependencies       = k_bfGfxMaxSubpasses,
  k_bfGfxDescriptorSets                  = 4,
  k_bfGfxShaderEntryPointNameLength      = 32,
  k_bfGfxShaderProgramNameLength         = 32,
  k_bfGfxMaxLayoutBindings               = 16,
  k_bfGfxDesfcriptorSetMaxLayoutBindings = 6,
  k_bfGfxMaxBufferBindings               = 16,
  k_bfGfxMaxDescriptorSetWrites          = 32,
  k_bfGfxMaxFrameGPUAhead                = 4,
  k_bfGfxMaxPipelineBarrierWrites        = 16,
  k_bfGfxMaxFramesDelay                  = 3, /*!< The max number of frames the CPU and GPU can be out of sync by. */
};

/*
  static const unsigned VULKAN_NUM_VERTEX_BUFFERS  = 4;
  static const unsigned VULKAN_PUSH_CONSTANT_SIZE  = 128;
  static const unsigned VULKAN_UBO_SIZE            = 16 * 1024;
  static const unsigned VULKAN_NUM_SPEC_CONSTANTS  = 8;
  static const unsigned VULKAN_NUM_DYNAMIC_UNIFORM_BUFFEFERS  = 8;
 */
#if __cplusplus
}
#endif

#endif /* BF_GFX_LIMITS_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
