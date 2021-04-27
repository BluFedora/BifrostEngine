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
        const UpdateTime time{dt, engine.fixedDt()};

        for (BehaviorOnUpdate& update : engine.behaviorEvt().m_OnUpdate)
        {
          update(time);
        }
      }
    }
  }
}  // namespace bf