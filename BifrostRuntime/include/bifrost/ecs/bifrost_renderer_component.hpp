/*!
 * @file   bifrost_renderer_component.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Contains the components that are drawn on screen.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_RENDERER_COMPONENT_HPP
#define BIFROST_RENDERER_COMPONENT_HPP

#include "bifrost/asset_io/bifrost_material.hpp" /* AssetMaterialHandle */
#include "bifrost_base_component.hpp"            /* BaseComponent       */

#include "bf/Animation2D.h"  // bfAnim2DSpriteHandle

namespace bf
{
  class MeshRenderer : public Component<MeshRenderer>
  {
    BIFROST_META_FRIEND;

   private:
    AssetMaterialHandle m_Material;
    EntityRef           m_EntityRef;  // TEMP CODE
    AssetModelHandle    m_Model;

   public:
    explicit MeshRenderer(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_EntityRef{},
      m_Model{nullptr}
    {
    }

    AssetMaterialHandle& material() { return m_Material; }
    AssetModelHandle&    model() { return m_Model; }
  };

  using SpriteRendererFlags = std::uint8_t;

  class SpriteRenderer : public Component<SpriteRenderer>
  {
    BIFROST_META_FRIEND;

   public:
    static constexpr SpriteRendererFlags FLAG_FLIP_X  = bfBit(0);
    static constexpr SpriteRendererFlags FLAG_FLIP_Y  = bfBit(1);
    static constexpr SpriteRendererFlags FLAG_DEFAULT = 0x0;

   private:
    AssetMaterialHandle m_Material;
    Vector2f            m_Size;
    Rect2f              m_UVRect;
    bfColor4u           m_Color;
    SpriteRendererFlags m_Flags;

   public:
    explicit SpriteRenderer(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_Size{1.0f, 1.0f},
      m_UVRect{0.0f, 0.0f, 1.0f, 1.0f},
      m_Color{255, 255, 255, 255},
      m_Flags{FLAG_DEFAULT}
    {
    }

    AssetMaterialHandle& material() { return m_Material; }
    Vector2f&            size() { return m_Size; }
    Rect2f&              uvRect() { return m_UVRect; }
    bfColor4u&           color() { return m_Color; }
    SpriteRendererFlags& flags() { return m_Flags; }

    void onEnable(Engine& engine);
  };

  class SpriteAnimator : public Component<SpriteAnimator>
  {
   private:
    bfAnim2DSpriteHandle m_SpriteHandle;

   public:
    explicit SpriteAnimator(Entity& owner) :
      Base(owner),
      m_SpriteHandle{bfAnim2DSprite_invalidHandle()}
    {
    }
  };

  using ParticleEmitterFlags = std::uint8_t;

  class ParticleEmitter : public Component<ParticleEmitter>
  {
    BIFROST_META_FRIEND;

   public:
    static constexpr ParticleEmitterFlags FLAG_IS_PLAYING = bfBit(0);
    static constexpr ParticleEmitterFlags FLAG_DEFAULT    = FLAG_IS_PLAYING;

   private:
    AssetMaterialHandle  m_Material;
    Vector2f             m_Size;
    Rect2f               m_UVRect;
    bfColor4f            m_Color;
    std::uint32_t        m_MaxParticles;
    ParticleEmitterFlags m_Flags;

   public:
    explicit ParticleEmitter(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_Size{1.0f, 1.0f},
      m_UVRect{0.0f, 0.0f, 1.0f, 1.0f},
      m_Color{1.0f, 1.0f, 1.0f, 1.0f},
      m_MaxParticles{100},
      m_Flags{FLAG_DEFAULT}
    {
    }
  };
}  // namespace bf

BIFROST_META_REGISTER(bf::MeshRenderer){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<MeshRenderer>("MeshRenderer"),                         //
   field<BaseAssetHandle>("m_Material", &MeshRenderer::m_Material),  //
   field("m_EntityRef", &MeshRenderer::m_EntityRef),                 //
   field<BaseAssetHandle>("m_Model", &MeshRenderer::m_Model)         //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::SpriteRenderer)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<SpriteRenderer>("SpriteRenderer"),                       //
     field<BaseAssetHandle>("m_Material", &SpriteRenderer::m_Material),  //
     field("m_Size", &SpriteRenderer::m_Size),                           //
     field("m_UVRect", &SpriteRenderer::m_UVRect),                       //
     field("m_Color", &SpriteRenderer::m_Color),                         //
     field("m_Flags", &SpriteRenderer::m_Flags)                          //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_RENDERER_COMPONENT_HPP */
