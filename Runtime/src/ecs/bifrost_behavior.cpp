/*!
 * @file   bifrost_behavior.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   The base class for all gameplay code for extending the engine.
 *
 * @version 0.0.1
 * @date    2020-06-13
 *
 * @copyright Copyright (c) 2020
 */
#include "bifrost/ecs/bifrost_behavior.hpp"

#include "bifrost/asset_io/bifrost_scene.hpp"  // Scene
#include "bifrost/ecs/bifrost_entity.hpp"      // Entity

namespace bf
{
  BaseBehavior::BaseBehavior(PrivateCtorTag) :
    IBehavior(),
    BaseComponent(),
    bfNonCopyMoveable<BaseBehavior>{},
    m_EventFlags{ON_NOTHING}
  {
  }

  void BaseBehavior::serialize(ISerializer& serializer)
  {
    (void)serializer;
  }

  void BaseBehavior::setActive(bool is_active)
  {
    if (isEventFlagSet(IS_ACTIVE) != is_active)
    {
      m_EventFlags ^= IS_ACTIVE;

      auto& behavior_list = owner().scene().m_ActiveBehaviors;

      if (is_active)
      {
        behavior_list.push(this);
        clearEventFlags(ON_ENABLE_CALLED);
      }
      else
      {
        const auto idx = behavior_list.find(this);

        behavior_list.swapAndPopAt(idx);

        onDisable();
      }
    }
  }
}  // namespace bifrost

using namespace bf;

void game::ExampleBehavior::onEnable()
{
  setEventFlags(ON_UPDATE);
}

void game::ExampleBehavior::onUpdate(float dt)
{
  auto& transform = owner().transform();

  const Vector3f scale = {(std::sin(time) + 1.0f) * (3.0f * 0.5f)};

  bfTransform_setScale(&transform, &scale);

  time += dt;
}
