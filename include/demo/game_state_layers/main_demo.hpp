#ifndef MAIN_DEMO_HPP
#define MAIN_DEMO_HPP

#include "bifrost/core/bifrost_igame_state_layer.hpp"

class MainDemoLayer final : public bifrost::IGameStateLayer
{
 public:
  const char* name() override
  {
    return "Main Demo";
  }

 public:
  ~MainDemoLayer() override = default;
};

#endif /* MAIN_DEMO_HPP */
