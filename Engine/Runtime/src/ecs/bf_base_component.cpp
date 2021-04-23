/******************************************************************************/
/*!
 * @file   bf_base_component.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   The base class for all core engine components.
 *   Look into "bf_component_list.hpp" for registering components.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#include "bf/ecs/bf_base_component.hpp"

#include "bf/asset_io/bifrost_scene.hpp" /* Scene  */
#include "bf/ecs/bf_entity.hpp"          /* Entity */

namespace bf
{
  BaseComponent::BaseComponent(Entity& owner) :
    m_Owner{&owner}
  {
  }

  BaseComponent::BaseComponent() :
    m_Owner{nullptr}
  {
  }

  Entity& BaseComponent::owner() const { return *m_Owner; }
  Scene&  BaseComponent::scene() const { return owner().scene(); }
  Engine& BaseComponent::engine() const { return scene().engine(); }

  template<>
  void ComponentTraits::onEnable<MeshRenderer>(MeshRenderer& comp, Engine& engine)
  {
    comp.m_BHVNode = comp.scene().bvh().insert(&comp.owner(), comp.owner().transform());
  }

  template<>
  void ComponentTraits::onDisable<MeshRenderer>(MeshRenderer& comp, Engine& engine)
  {
    comp.scene().bvh().remove(comp.m_BHVNode);
  }

  template<>
  void ComponentTraits::onEnable<SkinnedMeshRenderer>(SkinnedMeshRenderer& comp, Engine& engine)
  {
    comp.m_BHVNode = comp.scene().bvh().insert(&comp.owner(), comp.owner().transform());
  }

  template<>
  void ComponentTraits::onDisable<SkinnedMeshRenderer>(SkinnedMeshRenderer& comp, Engine& engine)
  {
    comp.scene().bvh().remove(comp.m_BHVNode);
  }

  template<>
  void ComponentTraits::onEnable<SpriteRenderer>(SpriteRenderer& comp, Engine& engine)
  {
    comp.m_BHVNode = comp.scene().bvh().insert(&comp.owner(), comp.owner().transform());
  }

  template<>
  void ComponentTraits::onDisable<SpriteRenderer>(SpriteRenderer& comp, Engine& engine)
  {
    comp.scene().bvh().remove(comp.m_BHVNode);
  }

  SpriteAnimator::SpriteAnimator(Entity& owner) :
    Base(owner),
    m_Spritesheet{nullptr},
    m_Anim2DUpdateInfo{}
  {
    m_Anim2DUpdateInfo.playback_speed      = 1.0f;
    m_Anim2DUpdateInfo.time_left_for_frame = 0.0f;
    m_Anim2DUpdateInfo.animation           = k_bfAnim2DInvalidID;
    m_Anim2DUpdateInfo.spritesheet_idx     = k_bfAnim2DInvalidID;
    m_Anim2DUpdateInfo.current_frame       = 0;
    m_Anim2DUpdateInfo.is_looping          = true;
  }
}  // namespace bf
