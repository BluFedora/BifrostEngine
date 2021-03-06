Framebuffer Creation Info:

[https://vulkan-tutorial.com/Multisampling]

Multisampling is only useful for Framebuffers.
Using it on a normal texture has no benefit.
Multisampling requires a mip_level of 1.

Framebuffer:
  VkImageView[] views;
  uint32_t      width;
  uint32_t      height;
  uint32_t      layers;

VkImageView:
  BifrostImageFormat format;    // Should just always match the VkImage::format
  uint32_t           mip_levels;
  uint32_t           num_layers;
  uint32_t           base_mip_level;
  uint32_t           base_array_layer;
  VkImage            image;
  @Optional ComponentMapping mapping;
  Aspect
    VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
    VK_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
    VK_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,

VkImage:
  width,
  height,
  depth,
  VkImageTiling tiling; (should always be optimal, Linear is useless)
  usage:
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004,
    VK_IMAGE_USAGE_STORAGE_BIT = 0x00000008,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT = 0x00000040,
    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT = 0x00000080,
  Image Type,
  mipLevels,
  arrayLayers,
  BifrostImageFormat format;
  samples;

To make it so framebuffers can use textures then I would need to Separate Sampler from the 
definition of a Texture. Technically in the API calling setSampler is optional soooo.

This can make automagic framebuffer creation easy since the Swapchain can be exposed as a 
TextureHandle but internally just not free in the traditional way.

A Cube Map just has 6 "arrayLayers" w/ VK_IMAGE_TYPE_2D.

Types Of Textures:
Texture2D
TextureArray
TextureCube

OpenGL:
glDrawBuffers is part of the Actual Frame buffers state.

Rendering Features:
  Directional Shadow Mapping needs to be able to make a Framebuffer with just a Depth buffer.

--- On Demand Resources (+ Hashed Contents) ---

> Framebuffer
  Array<TextureHandles> textures;

> Renderpass
  // dependencies do not need to be hashed since if you have the same arrangement
  // of subpasses (namely) 'bfAttachmentRefCache'
  // Maybe I am wrong but there should never be a case where you have the same 
  // arrangement of subpasses (aka same ins vs outs) but not need the same dependencies. 

  uint16_t                load_ops;         
  uint16_t                stencil_load_ops; 
  uint16_t                clear_ops;        
  uint16_t                stencil_clear_ops;
  uint16_t                store_ops;        
  uint16_t                stencil_store_ops;
  uint16_t                num_attachments;
  Array<bfAttachmentInfo> attachments;
  bfAttachmentInfo        depth_attachment;
  uint32_t                num_subpasses;
  Array<bfSubpassCache>   subpasses;
  
  bfAttachmentInfo {
    bfTextureHandle    texture;
    BifrostImageLayout final_layout;
    bool32             may_alias;
  }

  bfSubpassCache {
    uint32_t                    num_out_attachment_refs;
    Array<bfAttachmentRefCache> out_attachment_refs;
    uint32_t                    num_in_attachment_refs;
    Array<bfAttachmentRefCache> in_attachment_refs;
    bfAttachmentRefCache        depth_att;
  }

  bfAttachmentRefCache {
    uint32_t           attachment_index;
    BifrostImageLayout layout;
  }

> Pipeline
  bfPipelineStaticState static_state; [Hashed as 4 uint64_t]
  uint16_t              dynamic_state; // (Viewport / Scissor Rect / Blend Constants)
  BifrostViewport       viewport;
  BifrostScissorRect    scissor_rect;
  float                 blend_constants[4];
  float                 line_width;
  struct
  {
    float bias_constant_factor;
    float bias_clamp;
    float bias_slope_factor;

  } depth;

  float    min_sample_shading;
  uint32_t sample_count_flags;  // VK_SAMPLE_COUNT_1_BIT 
  uint32_t subpass_index;
  bfShaderProgram program;
  bfVertexLayoutSetHandle vertex_layout;
  bfRenderpassHandle renderpass;


  Desc Descriptor and Crap

  Per Shader:
	A VkDescriptorSetLayoutBinding is a single array of a certain resource.  (determines binding)
	A VkDescriptorSetLayout is a list of 'VkDescriptorSetLayoutBinding'.     (defines a set)
	A VkPipelineLayout has list of 'VkDescriptorSetLayout' + push constants. (determines set)
