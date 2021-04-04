/*!
 * @file   bifrost_behavior_system.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *
 * @version 0.0.1
 * @date    2020-06-14
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_BEHAVIOR_SYSTEM_HPP
#define BIFROST_BEHAVIOR_SYSTEM_HPP

#include "bifrost_iecs_system.hpp"

namespace bf
{
  class BehaviorSystem final : public IECSSystem
  {
  public:
    void onFrameUpdate(Engine& engine, float dt) override;
  };
}  // namespace bifrost

#endif /* BIFROST_BEHAVIOR_SYSTEM_HPP */
