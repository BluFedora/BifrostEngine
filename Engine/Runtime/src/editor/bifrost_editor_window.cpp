#include "bf/editor/bifrost_editor_window.hpp"

namespace bf::editor
{
  EditorWindowID BaseEditorWindow::s_TypeIDCounter = 0;

  static int s_IDCounter = 1;

  BaseEditorWindow::BaseEditorWindow() :
    m_IsOpen{true},
    m_IsFocused{false},
    m_IsVisible{true},
    m_DockID{0},
    m_InstanceID{s_IDCounter++}
  {
  }

  const char* BaseEditorWindow::fullImGuiTitle(IMemoryManager& memory) const
  {
    return string_utils::fmtAlloc(memory, nullptr, "%s###%p%i", title(), static_cast<const void*>(this), m_InstanceID);
  }

  void BaseEditorWindow::handleEvent(EditorOverlay& editor, Event& event)
  {
    onEvent(editor, event);
  }

  void BaseEditorWindow::update(EditorOverlay& editor, float delta_time)
  {
    onUpdate(editor, delta_time);
  }

  void BaseEditorWindow::uiShow(EditorOverlay& editor)
  {
    if (m_DockID)
    {
      ImGui::SetNextWindowDockID(m_DockID, ImGuiCond_Once);
    }

    const auto old_window_padding = ImGui::GetStyle().WindowPadding;

    onPreDrawGUI(editor);

    {
      LinearAllocator&     temp_allocator = editor.engine().tempMemory();
      LinearAllocatorScope mem_scope      = {temp_allocator};
      const char*          title_id       = fullImGuiTitle(temp_allocator);

      m_IsVisible = ImGui::Begin(title_id, &m_IsOpen, ImGuiWindowFlags_MenuBar);
    }

    if (m_IsVisible)
    {
      if (ImGui::IsWindowDocked())
      {
        const auto dock_id = ImGui::GetWindowDockID();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, old_window_padding);

        // TODO(SR): editor.addWindow<T> should not be called here
        // since it may invalidate pointers while iterating the window array.

        if (ImGui::BeginMenuBar())
        {
          if (ImGui::BeginMenu("Window"))
          {
            if (ImGui::MenuItem("Inspector"))
            {
              auto& new_window = editor.addWindow<Inspector>(allocator());

              new_window.m_DockID = dock_id;
            }

            if (ImGui::MenuItem("Scene"))
            {
              auto& new_window = editor.addWindow<SceneView>();

              new_window.m_DockID = dock_id;
            }

            if (ImGui::MenuItem("Game"))
            {
              auto& new_window = editor.addWindow<GameView>();

              new_window.m_DockID = dock_id;
            }

            ImGui::EndMenu();
          }

          ImGui::EndMenuBar();
        }

        ImGui::PopStyleVar(1);
      }

      m_IsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
      onDrawGUI(editor);
    }
    else
    {
      m_IsFocused = false;
    }

    ImGui::End();
    onPostDrawGUI(editor);
  }
}  // namespace bf::editor
