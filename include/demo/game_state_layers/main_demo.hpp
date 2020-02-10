#ifndef MAIN_DEMO_HPP
#define MAIN_DEMO_HPP

#include "bifrost/bifrost.hpp"
#include "bifrost/core/bifrost_igame_state_layer.hpp"
#include "imgui/imgui.h"

class ImGUIOverlay final : public bifrost::IGameStateLayer
{
 private:
  const char*      m_Name;
  bifrost::Entity* m_SelectedEntity;

 public:
  ImGUIOverlay(const char* name = "ImGUI Overlay") :
    m_Name{name},
    m_SelectedEntity{nullptr}
  {
  }

  const char* name() override
  {
    return m_Name;
  }

 protected:
  void onUpdate(BifrostEngine& engine, float delta_time) override;

  void inspect(const char* label, bifrost::BaseObjectT* object) const;
  bool inspect(const char* label, bifrost::Any& object, bifrost::meta::BaseClassMetaInfo* class_info) const;

 private:
  template<typename F>
  static bool window(const char* name, F&& content, bool* is_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None)
  {
    const bool ret = ImGui::Begin(name, is_open, flags);

    if (ret)
    {
      content();
    }

    ImGui::End();

    return ret;
  }
};

class MainDemoLayer final : public bifrost::IGameStateLayer
{
 public:
  const char* name() override
  {
    return "Main Demo";
  }

 protected:
  void onCreate(BifrostEngine& engine) override
  {
    bfLogPrint("MainDemoLayer::onCreate");
  }

  void onLoad(BifrostEngine& engine) override;

  void onUnload(BifrostEngine& engine) override
  {
    bfLogPrint("MainDemoLayer::onUnload");
  }

  void onDestroy(BifrostEngine& engine) override
  {
    bfLogPrint("MainDemoLayer::onDestroy");
  }

 public:
  ~MainDemoLayer() override = default;
};

#endif /* MAIN_DEMO_HPP */
