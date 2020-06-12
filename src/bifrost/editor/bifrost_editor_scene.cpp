#include "bifrost/editor/bifrost_editor_scene.hpp"

namespace bifrost::editor
{
  static const float k_SceneViewPadding = 1.0f;
  static const float k_InvalidMousePos  = -1.0f;

  SceneView::SceneView() :
    m_SceneViewViewport{},
    m_IsSceneViewHovered{false},
    m_Camera{nullptr},
    m_OldMousePos{k_InvalidMousePos, k_InvalidMousePos},
    m_MousePos{k_InvalidMousePos, k_InvalidMousePos},
    m_IsDraggingMouse{false},
    m_MouseLookSpeed{0.01f},
    m_Editor{nullptr}
  {
  }

  SceneView::~SceneView()
  {
    if (m_Camera)
    {
      m_Editor->engine().returnCamera(m_Camera);
    }
  }

  void SceneView::onPreDrawGUI(EditorOverlay& editor)
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_SceneViewPadding, k_SceneViewPadding));
  }

  void SceneView::onDrawGUI(EditorOverlay& editor)
  {
    static float Rounding = 5.0f;

    auto& engine       = editor.engine();
    auto& open_project = editor.currentlyOpenProject();

    if (!m_Camera)
    {
      m_Editor = &editor;
      m_Camera = engine.borrowCamera({1, 1});
    }

    if (!m_Camera)
    {
      ImGui::PopStyleVar(3);
      return;
    }

    if (open_project)
    {
      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("Camera"))
        {
          ImGui::DragFloat("Rounding", &Rounding, 1.0f, k_Epsilon, 100.0f);

          ImGui::DragFloat3("Ambient Color", &engine.renderer().AmbientColor.x);

          if (ImGui::DragFloat3("Position", &m_Camera->cpu_camera.position.x))
          {
            Camera_setViewModified(&m_Camera->cpu_camera);
          }

          ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
      }

      const auto   color_buffer        = m_Camera->gpu_camera.composite_buffer;
      const auto   color_buffer_width  = bfTexture_width(color_buffer);
      const auto   color_buffer_height = bfTexture_height(color_buffer);
      const auto   content_area        = ImGui::GetContentRegionAvail();
      const auto   draw_region         = rect::aspectRatioDrawRegion(color_buffer_width, color_buffer_height, uint32_t(content_area.x), uint32_t(content_area.y));
      auto* const  window_draw         = ImGui::GetWindowDrawList();
      const ImVec2 window_pos          = ImGui::GetWindowPos();
      const ImVec2 cursor_offset       = ImGui::GetCursorPos();
      const ImVec2 full_offset         = window_pos + cursor_offset;
      const ImVec2 position_min        = ImVec2{float(draw_region.left()), float(draw_region.top())} + full_offset;
      const ImVec2 position_max        = ImVec2{float(draw_region.right()), float(draw_region.bottom())} + full_offset;

      m_IsSceneViewHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

      m_SceneViewViewport.setLeft(int(position_min.x));
      m_SceneViewViewport.setTop(int(position_min.y));
      m_SceneViewViewport.setRight(int(position_max.x));
      m_SceneViewViewport.setBottom(int(position_max.y));

      if (m_Camera->old_width != content_area.x || m_Camera->old_height != content_area.y)
      {
        auto& window = engine.window();

        window.pushEvent(
         EventType::ON_WINDOW_RESIZE,
         WindowEvent(std::max(int(content_area.x), 1), std::max(int(content_area.y), 1), WindowEvent::FLAGS_DEFAULT),
         Event::FLAGS_IS_FALSIFIED);

        engine.resizeCamera(m_Camera, std::max(int(content_area.x), 1), std::max(int(content_area.y), 1));
      }

      window_draw->AddImageRounded(
       color_buffer,
       position_min,
       position_max,
       ImVec2(0.0f, 0.0f),
       ImVec2(1.0f, 1.0f),
       0xFFFFFFFF,
       Rounding,
       ImDrawCornerFlags_All);
    }
    else
    {
      static const char* s_StrNoProjectOpen = "No Project Open";
      static const char* s_StrNewProject    = "  New Project  ";
      static const char* s_StrOpenProject   = "  Open Project ";

      const auto text_size  = ImGui::CalcTextSize(s_StrNoProjectOpen);
      const auto mid_screen = (ImGui::GetWindowSize() - text_size) * 0.5f;

      ImGui::SetCursorPos(
       (ImGui::GetWindowSize() - text_size) * 0.5f);

      ImGui::Text("%s", s_StrNoProjectOpen);

      ImGui::SetCursorPosX(mid_screen.x);

      editor.buttonAction({&editor}, "File.New.Project", s_StrNewProject);

      ImGui::SetCursorPosX(mid_screen.x);

      editor.buttonAction({&editor}, "File.Open.Project", s_StrOpenProject);
    }
  }

  void SceneView::onPostDrawGUI(EditorOverlay& editor)
  {
    ImGui::PopStyleVar(3);
  }

  void SceneView::onEvent(EditorOverlay& editor, Event& event)
  {
    auto& mouse_evt = event.mouse;

    if (event.type == EventType::ON_MOUSE_DOWN || event.type == EventType::ON_MOUSE_UP)
    {
      m_OldMousePos = {k_InvalidMousePos, k_InvalidMousePos};

      if (event.type == EventType::ON_MOUSE_DOWN)
      {
        if (isPointOverSceneView({{mouse_evt.x, mouse_evt.y}}))
        {
          m_IsDraggingMouse = true;
        }
      }
      else
      {
        m_IsDraggingMouse = false;
        event.accept();
      }
    }
    else if (event.type == EventType::ON_MOUSE_MOVE)
    {
      m_MousePos = {float(mouse_evt.x), float(mouse_evt.y)};

      if (m_IsDraggingMouse && mouse_evt.button_state & MouseEvent::BUTTON_LEFT)
      {
        const float newx = float(mouse_evt.x);
        const float newy = float(mouse_evt.y);

        if (m_OldMousePos.x == k_InvalidMousePos)
        {
          m_OldMousePos.x = newx;
        }

        if (m_OldMousePos.y == k_InvalidMousePos)
        {
          m_OldMousePos.y = newy;
        }

        if (m_Camera)
        {
          Camera_mouse(
           &m_Camera->cpu_camera,
           (newx - m_OldMousePos.x) * m_MouseLookSpeed,
           (newy - m_OldMousePos.y) * -m_MouseLookSpeed);
        }

        m_OldMousePos.x = newx;
        m_OldMousePos.y = newy;
      }
    }

    if (m_IsDraggingMouse && event.isMouseEvent())
    {
      event.accept();
    }
  }
}  // namespace bifrost::editor
