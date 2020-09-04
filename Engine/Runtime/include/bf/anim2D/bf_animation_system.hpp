#ifndef BF_ANIMATION_SYSTEM
#define BF_ANIMATION_SYSTEM

#include "bifrost/ecs/bifrost_iecs_system.hpp"

#include "bf/Animation2D.h"

namespace bf
{
  class AnimationSystem final : public IECSSystem
  {
   private:
    bfAnimation2DCtx* m_Anim2DCtx;

   public:
    bfAnimation2DCtx* anim2DCtx() const { return m_Anim2DCtx; }

    void onInit(Engine& engine) override;
    void onFrameUpdate(Engine& engine, float dt) override;
    void onDeinit(Engine& engine) override;
  };
}  // namespace bf

#endif /* BF_ANIMATION_SYSTEM */
