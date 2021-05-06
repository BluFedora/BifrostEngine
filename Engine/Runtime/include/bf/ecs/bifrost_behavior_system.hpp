/******************************************************************************/
/*!
 * @file   bifrost_behavior_system.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *
 * @version 0.0.1
 * @date    2020-06-14
 *
 * @copyright Copyright (c) 2020-2021
 */
/******************************************************************************/
#ifndef BF_BEHAVIOR_SYSTEM_HPP
#define BF_BEHAVIOR_SYSTEM_HPP

#include "bifrost_iecs_system.hpp"

#include "bf/DenseMap.hpp"          // DenseMap
#include "bf/bf_function_view.hpp"  // FunctionView

namespace bf
{
  using BehaviorOnUpdate   = FunctionView<void(UpdateTime)>;
  using BehaviorOnUpdateID = DenseMapHandle<BehaviorOnUpdate, 8, 24>;

  class BaseBehavior;

  class BehaviorEvents
  {
    friend class BehaviorSystem;

   private:
    DenseMap<BehaviorOnUpdateID> m_OnUpdate;

   public:
    BehaviorEvents(IMemoryManager& memory) :
      m_OnUpdate{memory}
    {
    }

    void onUpdate(BaseBehavior* behavior);

    void remove(BehaviorOnUpdateID id)
    {
      m_OnUpdate.remove(id);
    }
  };

  class BehaviorSystem final : public IECSSystem
  {
   public:
    void onFrameUpdate(Engine& engine, float dt) override;
  };
}  // namespace bf

#endif /* BF_BEHAVIOR_SYSTEM_HPP */
