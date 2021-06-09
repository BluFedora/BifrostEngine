#include "bf/ecs/bifrost_behavior_system.hpp"

#include "bf/core/bifrost_engine.hpp"
#include "bf/ecs/bifrost_behavior.hpp"

namespace bf
{
  void BehaviorEvents::onUpdate(BaseBehavior* behavior)
  {
    if (!(behavior->m_OnUpdateID.isValid() && m_OnUpdate.has(behavior->m_OnUpdateID)))
    {
      behavior->m_OnUpdateID = m_OnUpdate.add(BehaviorOnUpdate::make<IBehavior, &IBehavior::onUpdate>(behavior));
    }
  }

  void BehaviorSystem::onFrameUpdate(Engine& engine, float dt)
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
}  // namespace bf