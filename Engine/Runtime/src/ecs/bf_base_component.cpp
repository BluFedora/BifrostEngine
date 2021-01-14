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
#include "bf/debug/bifrost_dbg_logger.h"
#include "bf/ecs/bifrost_entity.hpp" /* Entity */

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

  Entity& BaseComponent::owner() const
  {
    return *m_Owner;
  }

  Scene& BaseComponent::scene() const
  {
    return owner().scene();
  }

  Engine& BaseComponent::engine() const
  {
    return scene().engine();
  }

  template <> void ComponentTraits::onEnable<SpriteRenderer>(SpriteRenderer& comp, Engine& engine)
  {
    bfLogPrint("void bf::SpriteRenderer::onEnable(Engine& engine)");

  }

  SpriteAnimator::SpriteAnimator(Entity& owner) :
    Base(owner),
    m_Spritesheet{nullptr},
    m_SpriteHandle{bfAnim2DScene_addSprite(scene().anim2DScene())}
  {
  }

  template<>
  void ComponentTraits::onEnable<SpriteAnimator>(SpriteAnimator& comp, Engine& engine)
  {
    bfLogPrint("void bf::SpriteAnimator::onEnable(Engine& engine)");
  }

  template<>
  void ComponentTraits::onDisable<SpriteAnimator>(SpriteAnimator& comp, Engine& engine)
  {
    bfLogPrint("void bf::SpriteAnimator::onDisable(Engine& engine)");
  }

  template<>
  void ComponentTraits::onDestroy<SpriteAnimator>(SpriteAnimator& comp, Engine& engine)
  {
    if (!bfAnim2DSprite_isInvalidHandle(comp.m_SpriteHandle))
    {
      bfAnim2DScene_destroySprite(comp.scene().anim2DScene(), &comp.m_SpriteHandle);
    }
  }
}  // namespace bf
