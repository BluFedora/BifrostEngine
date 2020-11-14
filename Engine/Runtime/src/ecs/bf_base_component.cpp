/******************************************************************************/
/*!
 * @file   bf_base_component.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   The base class for all core engine components.
 *   Look into "bf_component_list.hpp" for registering
 *   components.
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

  void SpriteRenderer::onEnable(Engine& engine)
  {
    bfLogPrint("void bf::SpriteRenderer::onEnable(Engine& engine)");
  }

  void SpriteAnimator::onEnable(Engine& engine)
  {
    if (bfAnim2DSprite_isInvalidHandle(&m_SpriteHandle))
    {
      m_SpriteHandle = bfAnim2DScene_addSprite(scene().anim2DScene());
    }

    bfLogPrint("void bf::SpriteAnimator::onEnable(Engine& engine)");
  }

  void SpriteAnimator::onDisable(Engine& engine)
  {
    bfLogPrint("void bf::SpriteAnimator::onDisable(Engine& engine)");

    if (!bfAnim2DSprite_isInvalidHandle(&m_SpriteHandle))
    {
      bfAnim2DScene_destroySprite(scene().anim2DScene(), &m_SpriteHandle);
    }
  }
}  // namespace bf
