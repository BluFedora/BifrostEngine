#include "bf/core/bifrost_igame_state_layer.hpp"

namespace bf
{
  IGameStateLayer::IGameStateLayer():
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

  void IGameStateLayer::onDraw2D(Engine& engine, Gfx2DPainter& painter)
  {
    (void)engine;
    (void)painter;
  }

  void IGameStateLayer::onUpdate(Engine& engine, float delta_time)
  {
    (void)engine;
    (void)delta_time;
  }

  void IGameStateLayer::onUnload(Engine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onDestroy(Engine& engine)
  {
    (void)engine;
  }
}
