#ifndef MAIN_DEMO_HPP
#define MAIN_DEMO_HPP

#include "bf/core/bifrost_igame_state_layer.hpp"

class MainDemoLayer final : public bf::IGameStateLayer
{
 public:
  MainDemoLayer();

  ~MainDemoLayer() override = default;

  const char* name() override { return "Main Demo"; }

  void onEvent(Engine& engine, bf::Event& event) override;
  void onUpdate(Engine& engine, float delta_time) override;
};

#endif /* MAIN_DEMO_HPP */
