#ifndef MAIN_DEMO_HPP
#define MAIN_DEMO_HPP

#include "bifrost/core/bifrost_igame_state_layer.hpp"

class MainDemoLayer final : public bifrost::IGameStateLayer
{
 public:
  MainDemoLayer() :
    IGameStateLayer()
  {
  }

  ~MainDemoLayer() override = default;

  const char* name() override { return "Main Demo"; }

  void onEvent(Engine& engine, bifrost::Event& event) override;
  void onUpdate(Engine& engine, float delta_time) override;
};

#endif /* MAIN_DEMO_HPP */
