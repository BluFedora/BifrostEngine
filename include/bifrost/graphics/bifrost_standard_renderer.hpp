/*!
 * @file   bifrost_standard_renderer.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   This is the reference renderer that all more specific
 *   renderers should look to for how the layout of
 *   graphics resources are expected to be.
 *
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_STANDARD_RENDERER_HPP
#define BIFROST_STANDARD_RENDERER_HPP

#include "bifrost/bifrost_math.h"
#include "bifrost/data_structures/bifrost_array.hpp"
#include "bifrost_gfx_api.h"

namespace bifrost
{
  struct StandardVertex final
  {
    Vec3f     pos;
    Vec3f     normal;
    bfColor4u color;
    Vec2f     uv;
  };

  struct Vertex2DPosUV final
  {
    Vec2f pos;
    Vec2f uv;
  };

  // -------------------------------------------------------- //
  //                                                          //
  // Standard Shader Layout:                                  //
  //                                                          //
  // descriptor_set0                                          //
  // {                                                        //
  //   (binding = 0) mat4x4 u_ViewProjection;                 //
  //   (binding = 1) vec3   u_CameraPosition;                 //
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
  // }                                                        //
  //                                                          //
  // -------------------------------------------------------- //
  //                                                          //
  // Standard GBuffer Layout:                                 //
  //                                                          //
  // GBuffer0 [position.x, position.y, position.z, roughness] //
  // GBuffer1 [normal.x,   normal.y,   normal.z,   ao       ] //
  // GBuffer2 [albedo.r,   albedo.g,   albedo.b,   metallic ] //
  //                                                          //
  // -------------------------------------------------------- //

  static constexpr int k_GfxNumGBufferAttachments = 3;

  struct GBuffer final
  {
    // NOTE(Shareef):
    //   Don't mess with the layout of this struct unless you change the way
    //   [GBuffer::attachments] works or all uses of it.
    bfTextureHandle   color_attachments[k_GfxNumGBufferAttachments];
    bfTextureHandle   depth_attachment;
    BifrostClearValue clear_values[k_GfxNumGBufferAttachments + 1];

    void init(bfGfxDeviceHandle device, int width, int height);
    void setupAttachments(bfRenderpassInfo& renderpass_info, uint16_t subpass_index = 0);
    void deinit(bfGfxDeviceHandle device);

    bfTextureHandle* attachments() { return color_attachments; }
  };

  class StandardRenderer final
  {
   private:
    bfGfxContextHandle      m_GfxBackend;
    bfGfxDeviceHandle       m_GfxDevice;
    bfVertexLayoutSetHandle m_StandardVertexLayout;
    bfGfxCommandListHandle  m_MainCmdList;
    bfTextureHandle         m_MainSurface;
    GBuffer                 m_GBuffer;
    bfTextureHandle         m_DeferredComposite;
    bfShaderProgramHandle   m_GBufferShader;
    Array<bfGfxBaseHandle>  m_AutoRelease;

   public:
    StandardRenderer(IMemoryManager& memory);

    bfGfxContextHandle      context() const { return m_GfxBackend; }
    bfGfxDeviceHandle       device() const { return m_GfxDevice; }
    bfVertexLayoutSetHandle standardVertexLayout() const { return m_StandardVertexLayout; }
    bfGfxCommandListHandle  mainCommandList() const { return m_MainCmdList; }
    bfTextureHandle         surface() const { return m_MainSurface; }
    const GBuffer&          gBuffer() const { return m_GBuffer; }

    void               init(const bfGfxContextCreateParams& gfx_create_params);
    [[nodiscard]] bool frameBegin();
    void               beginGBufferPass();
    void               beginForwardSubpass();
    void               beginScreenPass() const;
    void               endPass() const;
    void               frameEnd() const;
    void               deinit();

   private:
    void initShaders();
    void deinitShaders();
  };
}  // namespace bifrost

#endif /* BIFROST_STANDARD_RENDERER_HPP */
