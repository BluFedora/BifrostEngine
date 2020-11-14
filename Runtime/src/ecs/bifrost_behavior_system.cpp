#include "bf/ecs/bifrost_behavior_system.hpp"

#include "bf/core/bifrost_engine.hpp"
#include "bf/ecs/bifrost_behavior.hpp"

namespace bf
{
  void BehaviorSystem::onFrameUpdate(Engine& engine, float dt)
  {
    const auto scene = engine.currentScene();

    if (scene)
    {
      // TODO(SR): Add editor mode update.
      if (engine.state() != EngineState::EDITOR_PLAYING)
      {
        for (BaseBehavior* const behavior : scene->behaviors())
        {
          if (!behavior->isEventFlagSet(IBehavior::ON_ENABLE_CALLED))
          {
            behavior->onEnable();
            behavior->setEventFlags(IBehavior::ON_ENABLE_CALLED);
          }
        }

        for (BaseBehavior* const behavior : scene->behaviors())
        {
          if (behavior->isEventFlagSet(IBehavior::ON_UPDATE))
          {
            behavior->onUpdate(dt);
          }
        }
      }
    }
  }
}  // namespace bifrost