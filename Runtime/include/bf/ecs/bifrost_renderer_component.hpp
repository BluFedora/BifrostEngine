/******************************************************************************/
/*!
 * @file   bf_renderer_component.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Contains the definitions of components that are drawn on screen.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BF_RENDERER_COMPONENT_HPP
#define BF_RENDERER_COMPONENT_HPP

#include "bf/asset_io/bf_gfx_assets.hpp"
#include "bf/asset_io/bf_spritesheet_asset.hpp"  // bfSpritesheet, bfAnim2DSpriteHandle
#include "bf/ecs/bf_base_component.hpp"          /* BaseComponent       */

namespace bf
{
  class MeshRenderer : public Component<MeshRenderer>
  {
    BF_META_FRIEND;

   public:
    ARC<MaterialAsset> m_Material;  // TODO(SR): Needs to be an array.
    ARC<ModelAsset>    m_Model;
    BVHNodeOffset      m_BHVNode;

   public:
    explicit MeshRenderer(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_Model{nullptr},
      m_BHVNode{k_BVHNodeInvalidOffset}
    {
    }

    ARC<MaterialAsset>& material() { return m_Material; }
    ARC<ModelAsset>&    model() { return m_Model; }
  };

  namespace ComponentTraits
  {
    template<>
    void onEnable<MeshRenderer>(MeshRenderer& comp, Engine& engine);
    template<>
    void onDisable<MeshRenderer>(MeshRenderer& comp, Engine& engine);
  }  // namespace ComponentTraits

  BIFROST_META_REGISTER(bf::MeshRenderer)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<MeshRenderer>("MeshRenderer"),                    //
       field<IARCHandle>("m_Material", &MeshRenderer::m_Material),  //
       field<IARCHandle>("m_Model", &MeshRenderer::m_Model)         //
      )
    BIFROST_META_END()
  }

  class SkinnedMeshRenderer : public Component<MeshRenderer>
  {
    BF_META_FRIEND;

   public:
    ARC<MaterialAsset> m_Material;  // TODO(SR): Needs to be an array.
    ARC<ModelAsset>    m_Model;
    ARC<Anim3DAsset>   m_Animation;
    AnimationTimeType  m_CurrentTime;
    BVHNodeOffset      m_BHVNode;

   public:
    explicit SkinnedMeshRenderer(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_Model{nullptr},
      m_Animation{nullptr},
      m_CurrentTime{AnimationTimeType(0)},
      m_BHVNode{k_BVHNodeInvalidOffset}
    {
    }

    ARC<MaterialAsset>& material() { return m_Material; }
    ARC<ModelAsset>&    model() { return m_Model; }
  };

  namespace ComponentTraits
  {
    template<>
    void onEnable<SkinnedMeshRenderer>(SkinnedMeshRenderer& comp, Engine& engine);
    template<>
    void onDisable<SkinnedMeshRenderer>(SkinnedMeshRenderer& comp, Engine& engine);
  }  // namespace ComponentTraits

  BIFROST_META_REGISTER(bf::SkinnedMeshRenderer)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<MeshRenderer>("SkinnedMeshRenderer"),                      //
       field<IARCHandle>("m_Material", &SkinnedMeshRenderer::m_Material),    //
       field<IARCHandle>("m_Animation", &SkinnedMeshRenderer::m_Animation),  //
       field<IARCHandle>("m_Model", &SkinnedMeshRenderer::m_Model)           //
      )
    BIFROST_META_END()
  }

  using SpriteRendererFlags = std::uint8_t;

  class SpriteRenderer : public Component<SpriteRenderer>
  {
    BF_META_FRIEND;

   public:
    static constexpr SpriteRendererFlags FLAG_DEFAULT = 0x0;
    static constexpr SpriteRendererFlags FLAG_FLIP_X  = bfBit(0);
    static constexpr SpriteRendererFlags FLAG_FLIP_Y  = bfBit(1);

   public:
    ARC<MaterialAsset>  m_Material;
    Vector2f            m_Size;
    Rect2f              m_UVRect;
    bfColor4u           m_Color;
    SpriteRendererFlags m_Flags;
    BVHNodeOffset       m_BHVNode;

   public:
    explicit SpriteRenderer(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_Size{1.0f, 1.0f},
      m_UVRect{0.0f, 0.0f, 1.0f, 1.0f},
      m_Color{255, 255, 255, 255},
      m_Flags{FLAG_DEFAULT},
      m_BHVNode{k_BVHNodeInvalidOffset}
    {
    }

    ARC<MaterialAsset>&  material() { return m_Material; }
    Vector2f&            size() { return m_Size; }
    Rect2f&              uvRect() { return m_UVRect; }
    bfColor4u&           color() { return m_Color; }
    SpriteRendererFlags& flags() { return m_Flags; }
  };

  namespace ComponentTraits
  {
    template<>
    void onEnable<SpriteRenderer>(SpriteRenderer& comp, Engine& engine);
    template<>
    void onDisable<SpriteRenderer>(SpriteRenderer& comp, Engine& engine);
  }  // namespace ComponentTraits

  BIFROST_META_REGISTER(bf::SpriteRenderer)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<SpriteRenderer>("SpriteRenderer"),                  //
       field<IARCHandle>("m_Material", &SpriteRenderer::m_Material),  //
       field("m_Size", &SpriteRenderer::m_Size),                      //
       field("m_UVRect", &SpriteRenderer::m_UVRect),                  //
       field("m_Color", &SpriteRenderer::m_Color),                    //
       field("m_Flags", &SpriteRenderer::m_Flags)                     //
      )
    BIFROST_META_END()
  }

  class SpriteAnimator : public Component<SpriteAnimator>
  {
    BF_META_FRIEND;
    friend class AnimationSystem;

   public:
    ARC<SpritesheetAsset> m_Spritesheet;
    bfAnim2DSpriteHandle  m_SpriteHandle;

   public:
    explicit SpriteAnimator(Entity& owner);

    ARC<SpritesheetAsset> spritesheet() const { return m_Spritesheet; }
    bfAnim2DSpriteHandle  animatedSprite() const { return m_SpriteHandle; }
  };

  namespace ComponentTraits
  {
    template<>
    void onCreate<SpriteAnimator>(SpriteAnimator& comp, Engine& engine);
    template<>
    void onDestroy<SpriteAnimator>(SpriteAnimator& comp, Engine& engine);
  }  // namespace ComponentTraits

  BIFROST_META_REGISTER(bf::SpriteAnimator)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<SpriteAnimator>("SpriteAnimator"),                       //
       field<IARCHandle>("m_Spritesheet", &SpriteAnimator::m_Spritesheet)  //
      )
    BIFROST_META_END()
  }
  //*
  using ParticleEmitterFlags = std::uint8_t;

  class ParticleEmitter : public Component<ParticleEmitter>
  {
    BF_META_FRIEND;

   public:
    static constexpr ParticleEmitterFlags FLAG_IS_PLAYING = bfBit(0);
    static constexpr ParticleEmitterFlags FLAG_DEFAULT    = FLAG_IS_PLAYING;

   private:
    ARC<MaterialAsset>   m_Material;
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
  //*/
}  // namespace bf

#endif /* BF_RENDERER_COMPONENT_HPP */
