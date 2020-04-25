#include "bifrost/editor/bifrost_editor_window.hpp"

namespace bifrost::editor
{
  EditorWindowID BaseEditorWindow::s_TypeIDCounter = 0;

  const char* BaseEditorWindow::fullImGuiTitle(IMemoryManager& memory) const
  {
    return string_utils::fmtAlloc(memory, nullptr, "%s###%p", title(), static_cast<const void*>(this));
  }

  void BaseEditorWindow::uiShow(EditorOverlay& editor)
  {
    LinearAllocator&     temp_allocator = editor.engine().tempMemory();
    LinearAllocatorScope mem_scope      = {temp_allocator};
    const char*          title_id       = fullImGuiTitle(temp_allocator);

    ImGui::SetNextWindowDockID(m_DockID, ImGuiCond_Once);

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

            ImGui::EndMenu();
          }

          ImGui::EndMenuBar();
        }
      }

      onDrawGUI(editor);
    }
    ImGui::End();
  }

  void BaseEditorWindow::selectionChange(const Selectable& selectable)
  {
    onSelectionChanged(selectable);
  }
}  // namespace bifrost::editor
