#ifndef BF_ANIMATION_SYSTEM
#define BF_ANIMATION_SYSTEM

#include "bf/ecs/bifrost_iecs_system.hpp"
#include "bf/graphics/bifrost_standard_renderer.hpp"

#include "bf/Animation2D.h"

namespace bf
{
  struct ObjectBoneData
  {
    Mat4x4 bones[k_GfxMaxTotalBones];
  };

  class AnimationSystem final : public IECSSystem
  {
   private:
    bfAnim2DCtx*                                    m_Anim2DCtx;
    List<Renderable<ObjectBoneData>>                m_RenderablePool;  // TODO(SR): Per make this Scene.
    HashTable<Entity*, Renderable<ObjectBoneData>*> m_Renderables;     // TODO(SR): Per make this Scene.

   public:
    AnimationSystem(IMemoryManager& memory) :
      m_Anim2DCtx{nullptr},
      m_RenderablePool{memory},
      m_Renderables{}
    {
    }

    bfAnim2DCtx*                anim2DCtx() const { return m_Anim2DCtx; }
    Renderable<ObjectBoneData>& getRenderable(StandardRenderer& renderer, Entity& entity);

    void onInit(Engine& engine) override;
    void onFrameUpdate(Engine& engine, float dt) override;
    void onDeinit(Engine& engine) override;
  };
}  // namespace bf

#endif /* BF_ANIMATION_SYSTEM */
