/******************************************************************************/
/*!
* @file   bifrost_component_renderer.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Handles the drawing of all rendering components.
*
* @version 0.0.1
* @date    2020-07-10
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BF_COMPONENT_RENDERER_HPP
#define BF_COMPONENT_RENDERER_HPP

#include "bf/asset_io/bf_gfx_assets.hpp"  /* */
#include "bf/ecs/bifrost_iecs_system.hpp" /* IECSSystem                            */
#include "bf/gfx/bf_render_queue.hpp"     /* */
#include "bifrost_standard_renderer.hpp"  /* TransientVertexBuffer, StandardVertex */

#define k_UseIndexBufferForSprites 1

namespace bf
{
  struct Renderable2DPrimitive
  {
    Mat4x4         transform;
    MaterialAsset* material;
    Vector3f       origin;
    Vector2f       size;
    bfColor4u      color;
    Rect2f         uv_rect;
  };

#if k_UseIndexBufferForSprites
  static constexpr std::size_t k_NumVerticesPerSprite = 4;
  static constexpr std::size_t k_NumIndicesPerSprite  = 6;
#else
  static constexpr std::size_t k_NumVerticesPerSprite = 6;
  static constexpr std::size_t k_NumIndicesPerSprite  = 0;
#endif

  static constexpr std::size_t k_MaxSpritesInBatch        = 256;
  static constexpr std::size_t k_MaxVerticesInSpriteBatch = k_MaxSpritesInBatch * k_NumVerticesPerSprite;
  static constexpr std::size_t k_MaxIndicesInSpriteBatch  = (k_MaxVerticesInSpriteBatch / k_NumVerticesPerSprite) * k_NumIndicesPerSprite;

  using SpriteIndexType                             = std::uint16_t;
  static constexpr bfGfxIndexType k_SpriteIndexType = bfIndexTypeFromT<SpriteIndexType>();

  static_assert(k_MaxVerticesInSpriteBatch % k_NumVerticesPerSprite == 0, "The number of vertices in a batch is most optimal as a multiple of 4.");
  static_assert(k_MaxVerticesInSpriteBatch < std::numeric_limits<SpriteIndexType>::max(), "The index type needs top be upgraded if we cannot refer to all vertices in a single GfxLinkedBuffer::Link.");
  static_assert(!k_NumIndicesPerSprite || k_MaxIndicesInSpriteBatch % k_NumIndicesPerSprite == 0, "The number of indices in a batch is most optimal as a multiple of 6.");

  class ComponentRenderer final : public IECSSystem
  {
    using VertexBuffer = GfxLinkedBuffer<StandardVertex, k_MaxVerticesInSpriteBatch, BF_BUFFER_USAGE_VERTEX_BUFFER>;
    using IndexBuffer  = GfxLinkedBuffer<SpriteIndexType, k_MaxVerticesInSpriteBatch, BF_BUFFER_USAGE_INDEX_BUFFER>;

   private:
    bfShaderModuleHandle          m_ShaderModules[2]   = {};       //!< [Sprite-Vertex, Sprite-Fragment]
    bfShaderProgramHandle         m_ShaderProgram      = nullptr;  //!< Sprite Program
    VertexBuffer*                 m_SpriteVertexBuffer = nullptr;
    Array<Renderable2DPrimitive>* m_PerFrameSprites    = nullptr;

#if k_UseIndexBufferForSprites
    IndexBuffer* m_SpriteIndexBuffer = nullptr;
#endif

   public:
    void onInit(Engine& engine) override;
    void onFrameBegin(Engine& engine, float dt) override;
    void onFrameDraw(Engine& engine, RenderView& camera, float alpha) override;
    void onDeinit(Engine& engine) override;

    // Immediate Mode Sprite Submission

    void pushSprite(const Renderable2DPrimitive& sprite) const;

    //

    static void pushModel(
     RenderView&               camera,
     Entity*                   entity,
     const ModelAsset&         model,
     const bfDrawCallPipeline& pipeline,
     StandardRenderer&         engine_renderer,
     RenderQueue&              render_queue,
     float                     distance_from_camera = 1.0f);
  };
}  // namespace bf

#endif /* BF_COMPONENT_RENDERER_HPP */
