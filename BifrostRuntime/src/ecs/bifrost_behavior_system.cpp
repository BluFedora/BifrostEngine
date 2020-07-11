#include "bifrost/ecs/bifrost_behavior_system.hpp"

#include "bifrost/core/bifrost_engine.hpp"
#include "bifrost/ecs/bifrost_behavior.hpp"

namespace bifrost
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