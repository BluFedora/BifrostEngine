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

namespace bifrost
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
}  // namespace bifrost

#include "bifrost/ecs/bifrost_entity.hpp"
#include "bifrost/asset_io/bifrost_scene.hpp"

using namespace bifrost;

void game::ExampleBehavior::onUpdate(Engine& engine, float dt)
{
  auto& transform = owner().transform();

  const Vector3f scale = {(std::sin(time) + 1.0f) * (3.0f * 0.5f)};

  bfTransform_setScale(&transform, &scale);

  time += dt;
}
