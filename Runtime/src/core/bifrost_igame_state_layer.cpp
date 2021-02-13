#include "bf/core/bifrost_igame_state_layer.hpp"

namespace bf
{
  IGameStateLayer::IGameStateLayer() :
    m_Prev{nullptr},
    m_Next{nullptr},
    m_IsOverlay{false}
  {
  }

  void IGameStateLayer::onCreate(Engine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onLoad(Engine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onEvent(Engine& engine, bfEvent& event)
  {
    (void)engine;
    (void)event;
  }

  void IGameStateLayer::onFixedUpdate(Engine& engine, float delta_time)
  {
    (void)engine;
    (void)delta_time;
  }

  void IGameStateLayer::onUpdate(Engine& engine, float delta_time)
  {
    (void)engine;
    (void)delta_time;
  }

  void IGameStateLayer::onDraw(Engine& engine, RenderView& camera, float alpha)
  {
    (void)engine;
    (void)camera;
    (void)alpha;
  }

  void IGameStateLayer::onRenderBackbuffer(Engine& engine, float alpha)
  {
  }

  void IGameStateLayer::onUnload(Engine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onDestroy(Engine& engine)
  {
    (void)engine;
  }
}  // namespace bf
