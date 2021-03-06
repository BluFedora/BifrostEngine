#include "bf/editor/bifrost_editor_game.hpp"

#include "bf/asset_io/bifrost_json_serializer.hpp"
#include "bf/editor/bifrost_editor_overlay.hpp"

#include <imgui/imgui.h>

#include <imgui/imgui_internal.h>

namespace bf::editor
{
  GameView::GameView() :
    m_Editor{nullptr},
    m_Camera{nullptr},
    m_SerializedScene{}
  {
  }

  GameView::~GameView()
  {
    auto&      engine = m_Editor->engine();
    const auto scene  = engine.currentScene();

    if (scene)
    {
      stopSimulation(engine, scene);
    }

    if (m_Camera)
    {
      m_Editor->engine().returnCamera(m_Camera);
    }
  }

  void GameView::onPreDrawGUI(EditorOverlay& editor)
  {
    if (m_Camera)
    {
      std::uint8_t view_flags = 0x0;

      if (isVisible())
      {
        view_flags |= RenderView::DO_DRAW;
      }

      m_Camera->flags = view_flags;
    }
  }

  void GameView::onDrawGUI(EditorOverlay& editor)
  {
    auto& engine       = editor.engine();
    auto& open_project = editor.currentlyOpenProject();

    const auto scene = engine.currentScene();

    m_Editor = &editor;

    if (open_project)
    {
      if (!m_Camera)
      {
        m_Camera = engine.borrowCamera({1280, 720});

        if (!m_Camera)
        {
          return;
        }
      }

      LinearAllocatorScope mem_scope{engine.tempMemory()};

      if (ImGui::BeginMenuBar())
      {
        if (scene)
        {
          static const char* k_EngineStateToString[] =
           {
            "*Playing*",
            "<Editor>",
            "<Stopped>",
           };

          char* buffer = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "Status (%s)", k_EngineStateToString[int(engine.state())]);

          if (ImGui::Selectable(buffer, engine.state() != EngineState::EDITOR_PLAYING, ImGuiSelectableFlags_None, ImVec2(ImGui::CalcTextSize(buffer).x, 0.0f)))
          {
            toggleEngineState(engine, scene);
            editor.selection().clear();
          }
        }

        ImGui::EndMenuBar();
      }

      const bfTextureHandle color_buffer        = m_Camera->gpu_camera.composite_buffer;
      const std::uint32_t   color_buffer_width  = bfTexture_width(color_buffer);
      const std::uint32_t   color_buffer_height = bfTexture_height(color_buffer);
      const ImVec2          content_area        = ImGui::GetContentRegionAvail();
      const Rect2i          draw_region         = rect::aspectRatioDrawRegion(color_buffer_width, color_buffer_height, uint32_t(content_area.x), uint32_t(content_area.y));
      auto* const           window_draw         = ImGui::GetWindowDrawList();
      const ImVec2          window_pos          = ImGui::GetWindowPos();
      const ImVec2          cursor_offset       = ImGui::GetCursorPos();
      const ImVec2          full_offset         = window_pos + cursor_offset;
      const ImVec2          position_min        = ImVec2{float(draw_region.left()), float(draw_region.top())} + full_offset;
      const ImVec2          position_max        = ImVec2{float(draw_region.right()), float(draw_region.bottom())} + full_offset;

      if (scene)
      {
        m_Camera->cpu_camera = scene->camera();
      }

      window_draw->AddImage(
       color_buffer,
       position_min,
       position_max,
       ImVec2(0.0f, 0.0f),
       ImVec2(1.0f, 1.0f),
       0xFFFFFFFF);
    }
  }

  void GameView::toggleEngineState(Engine& engine, const ARC<SceneAsset>& scene)
  {
    if (engine.state() == EngineState::EDITOR_PLAYING)
    {
      startSimulation(engine, scene);
    }
    else
    {
      stopSimulation(engine, scene);
    }
  }

  void GameView::startSimulation(Engine& engine, const ARC<SceneAsset>& scene)
  {
    if (engine.state() == EngineState::EDITOR_PLAYING)
    {
      JsonSerializerWriter serializer{engine.tempMemory()};

      if (serializer.beginDocument())
      {
        scene->reflect(serializer);
        serializer.endDocument();
      }

      m_SerializedScene = serializer.document();

      engine.setState(EngineState::RUNTIME_PLAYING);
      scene->startup();
    }
  }

  void GameView::stopSimulation(Engine& engine, const ARC<SceneAsset>& scene)
  {
    if (engine.state() != EngineState::EDITOR_PLAYING)
    {
      scene->shutdown();
      engine.setState(EngineState::EDITOR_PLAYING);

      JsonSerializerReader serializer{engine.assets(), engine.tempMemory(), m_SerializedScene};

      if (serializer.beginDocument())
      {
        scene->reflect(serializer);
        serializer.endDocument();
      }
    }
  }
}  // namespace bf::editor
