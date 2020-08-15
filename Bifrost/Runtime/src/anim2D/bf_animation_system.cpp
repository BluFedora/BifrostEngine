#include "bf/anim2D/bf_animation_system.hpp"

#include "bifrost/core/bifrost_engine.hpp"

namespace bf
{
  void AnimationSystem::onInit(Engine& engine)
  {
    const bfAnimation2DCreateParams create_anim_ctx = {nullptr, nullptr, nullptr};

    m_Anim2DCtx = bfAnimation2D_new(&create_anim_ctx);
  }

  void AnimationSystem::onFrameUpdate(Engine& engine, float dt)
  {
    const auto scene = engine.currentScene();

    if (scene)
    {
      
    }
  }

  void AnimationSystem::onDeinit(Engine& engine)
  {
    bfAnimation2D_delete(m_Anim2DCtx);
  }
}
