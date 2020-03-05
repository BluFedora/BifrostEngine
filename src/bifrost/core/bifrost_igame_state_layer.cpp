#include "bifrost/core/bifrost_igame_state_layer.hpp"

namespace bifrost
{
  IGameStateLayer::IGameStateLayer():
    m_Prev{nullptr},
    m_Next{nullptr},
    m_IsOverlay{false}
  {
  }

  void IGameStateLayer::onCreate(BifrostEngine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onLoad(BifrostEngine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onEvent(BifrostEngine& engine, Event& event)
  {
    (void)engine;
    (void)event;
  }

  void IGameStateLayer::onFixedUpdate(BifrostEngine& engine, float delta_time)
  {
    (void)engine;
    (void)delta_time;
  }

  void IGameStateLayer::onUpdate(BifrostEngine& engine, float delta_time)
  {
    (void)engine;
    (void)delta_time;
  }

  void IGameStateLayer::onUnload(BifrostEngine& engine)
  {
    (void)engine;
  }

  void IGameStateLayer::onDestroy(BifrostEngine& engine)
  {
    (void)engine;
  }
}
