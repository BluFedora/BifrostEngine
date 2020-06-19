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
        for (IBehavior* const behavior : scene->behaviors())
        {
          behavior->onUpdate(engine, dt);
        }
      }
    }
  }
}  // namespace bifrost