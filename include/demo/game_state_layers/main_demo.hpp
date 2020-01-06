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
  void onCreate(BifrostEngine& engine) override
  {
  }

  void onLoad(BifrostEngine& engine) override
  {
    ImGuiStyle& style = ImGui::GetStyle();

    bfLogPrint("Setting up ImGUI Styles.");

    style.FrameRounding    = 2.0f;
    style.FramePadding     = ImVec2(4.0f, 2.0f);
    style.FrameBorderSize  = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowPadding    = ImVec2(5.0f, 5.0f);
    style.WindowRounding   = 3.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ChildRounding    = 2.0f;
    style.GrabMinSize      = 6.0f;
    style.GrabRounding     = 2.0f;

    bfLogPrint("Setting up ImGUI Colors.");
    ImVec4* colors                     = style.Colors;
    colors[ImGuiCol_Text]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_WindowBg]          = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBg]           = ImVec4(0.06f, 0.06f, 0.07f, 0.54f);
    colors[ImGuiCol_TitleBgActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Border]            = ImVec4(0.09f, 0.05f, 0.11f, 0.38f);
    colors[ImGuiCol_TitleBg]           = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.00f, 0.00f, 0.00f, 0.66f);
    colors[ImGuiCol_CheckMark]         = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ResizeGrip]        = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.28f, 0.33f, 0.81f);
    colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.16f, 0.15f, 0.20f, 0.95f);
    colors[ImGuiCol_Tab]               = ImVec4(0.12f, 0.09f, 0.16f, 0.86f);
    colors[ImGuiCol_TabActive]         = ImVec4(0.41f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_Header]            = ImVec4(0.08f, 0.08f, 0.09f, 0.31f);
    colors[ImGuiCol_HeaderHovered]     = ImVec4(0.26f, 0.26f, 0.27f, 0.80f);
    colors[ImGuiCol_HeaderActive]      = ImVec4(0.14f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]    = ImVec4(0.63f, 0.65f, 0.68f, 0.44f);
    colors[ImGuiCol_FrameBgActive]     = ImVec4(0.37f, 0.38f, 0.40f, 0.89f);
    colors[ImGuiCol_SliderGrab]        = ImVec4(0.75f, 0.75f, 0.77f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_Button]            = ImVec4(0.51f, 0.53f, 0.71f, 0.40f);
    colors[ImGuiCol_ButtonHovered]     = ImVec4(0.45f, 0.43f, 0.52f, 0.86f);
    colors[ImGuiCol_ButtonActive]      = ImVec4(0.26f, 0.24f, 0.30f, 0.82f);
  }

  void onEvent(BifrostEngine& engine, bifrost::Event& event) override
  {
  }

  void onUpdate(BifrostEngine& engine, float delta_time) override;

  void onUnload(BifrostEngine& engine) override
  {
  }

  void onDestroy(BifrostEngine& engine) override
  {
  }

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

  void onEvent(BifrostEngine& engine, bifrost::Event& event) override
  {
  }

  void onUpdate(BifrostEngine& engine, float delta_time) override
  {
    // bfLogPrint("MainDemoLayer::Update");
  }

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
