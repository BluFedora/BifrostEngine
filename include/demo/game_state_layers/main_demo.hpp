#ifndef MAIN_DEMO_HPP
#define MAIN_DEMO_HPP

#include "bifrost/core/bifrost_igame_state_layer.hpp"
#include "bifrost/event/bifrost_window_event.hpp"

class MainDemoLayer final : public bifrost::IGameStateLayer
{
 private:
  bool m_IsKeyDown[bifrost::k_KeyCodeMax + 1];
  bool m_IsShiftDown;

 public:
  MainDemoLayer() :
    IGameStateLayer(),
    m_IsKeyDown{false},
    m_IsShiftDown{false}
  {
  }

  ~MainDemoLayer() override = default;

  const char* name() override { return "Main Demo"; }

  void onEvent(BifrostEngine& engine, bifrost::Event& event) override;
  void onUpdate(BifrostEngine& engine, float delta_time) override;
};

#endif /* MAIN_DEMO_HPP */
