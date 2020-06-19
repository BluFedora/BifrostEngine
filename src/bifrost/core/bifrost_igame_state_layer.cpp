#include "bifrost/core/bifrost_igame_state_layer.hpp"

namespace bifrost
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

  void IGameStateLayer::onEvent(Engine& engine, Event& event)
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

  void IGameStateLayer::onUnload(Engine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onDestroy(Engine& engine)
  {
    (void)engine;
  }
}
