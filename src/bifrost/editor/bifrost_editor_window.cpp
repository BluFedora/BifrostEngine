#include "bifrost/editor/bifrost_editor_window.hpp"

namespace bifrost::editor
{
  EditorWindowID BaseEditorWindow::s_TypeIDCounter = 0;

  static int s_IDCounter = 1;

  BaseEditorWindow::BaseEditorWindow():
    m_IsOpen{true},
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

  void BaseEditorWindow::uiShow(EditorOverlay& editor)
  {
    LinearAllocator&     temp_allocator = editor.engine().tempMemory();
    LinearAllocatorScope mem_scope      = {temp_allocator};
    const char*          title_id       = fullImGuiTitle(temp_allocator);

    if (m_DockID)
    {
      ImGui::SetNextWindowDockID(m_DockID, ImGuiCond_Once);
    }

    onPreDrawGUI(editor);

    if (ImGui::Begin(title_id, &m_IsOpen, ImGuiWindowFlags_MenuBar))
    {
      const auto is_docked = ImGui::IsWindowDocked();

      if (is_docked)
      {
        const auto dock_id = ImGui::GetWindowDockID();

        if (ImGui::BeginMenuBar())
        {
          if (ImGui::BeginMenu("Add Tab"))
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

            ImGui::EndMenu();
          }

          ImGui::EndMenuBar();
        }
      }

      onDrawGUI(editor);
    }
    ImGui::End();
    onPostDrawGUI(editor);
  }

  void BaseEditorWindow::selectionChange(const Selectable& selectable)
  {
    onSelectionChanged(selectable);
  }
}  // namespace bifrost::editor
