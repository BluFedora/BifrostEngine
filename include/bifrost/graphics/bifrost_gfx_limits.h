#ifndef BIFROST_GFX_LIMITS_H
#define BIFROST_GFX_LIMITS_H

#if __cplusplus
extern "C" {
#endif
#define BIFROST_GFX_RENDERPASS_MAX_SUBPASSES 16
#define BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS 8
#define BIFROST_GFX_RENDERPASS_MAX_DEPENDENCIES BIFROST_GFX_RENDERPASS_MAX_SUBPASSES

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
#define BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS 4
#define BIFROST_GFX_SHADER_ENTRY_POINT_NAME_LENGTH 32
#define BIFROST_GFX_SHADER_PROGRAM_NAME_LENGTH 32
#define BIFROST_GFX_VERTEX_LAYOUT_MAX_BINDINGS 16
#define BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS 16
#define BIFROST_GFX_BUFFERS_MAX_BINDING 16
#define BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES 32
#define BIFROST_GFX_GPU_MAX_FRAMES_AHEAD 4
#define BIFROST_GFX_PIPELINE_BARRIER_MAX_WRITES 16

  /*
    [x] static const unsigned VULKAN_NUM_BINDINGS        = 16;
    [x] static const unsigned VULKAN_NUM_VERTEX_ATTRIBS  = 16;
    [ ] static const unsigned VULKAN_NUM_VERTEX_BUFFERS  = 4;
    [ ] static const unsigned VULKAN_PUSH_CONSTANT_SIZE  = 128;
    [ ] static const unsigned VULKAN_UBO_SIZE            = 16 * 1024;
    [ ] static const unsigned VULKAN_NUM_SPEC_CONSTANTS  = 8;
    [ ] static const unsigned VULKAN_NUM_DYNAMIC_UNIFORM_BUFFEFERS  = 8;
   */
#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_LIMITS_H */
