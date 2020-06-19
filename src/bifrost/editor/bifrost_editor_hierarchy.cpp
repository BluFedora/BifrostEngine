#include "bifrost/editor/bifrost_editor_hierarchy.hpp"

#include "bifrost/editor/bifrost_editor_serializer.hpp"

#include "bifrost/editor/bifrost_editor_overlay.hpp"

namespace bifrost::editor
{
  HierarchyView::HierarchyView() :
    m_SearchQuery{""}  // Filled with initial data for ImGui.
  {
  }

  void HierarchyView::onDrawGUI(EditorOverlay& editor)
  {
    auto&      engine        = editor.engine();
    const auto current_scene = engine.currentScene();

    if (!current_scene)
    {
      ImGui::TextUnformatted("(No Scene Open)");

      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Create a new Scene by right clicking a folder 'Create->Scene'\nThen double click the newly created Scene asset.");
      }

      return;
    }

    ImGui::Separator();

    imgui_ext::inspect("Search", m_SearchQuery, ImGuiInputTextFlags_CharsUppercase);

    if (!m_SearchQuery.isEmpty())
    {
      ImGui::SameLine();

      if (ImGui::Button("clear"))
      {
        m_SearchQuery.clear();
      }
    }

    if (ImGui::Button("Add Entity"))
    {
      current_scene->addEntity();

      engine.assets().markDirty(current_scene);
    }

    ImGui::Separator();

    for (Entity* const root_entity : current_scene->rootEntities())
    {
      ImGui::PushID(root_entity);
      if (ImGui::Selectable(root_entity->name().cstr()))
      {
        editor.select(root_entity);
      }

      bfTransform_flushChanges(&root_entity->transform());

      ImGui::PopID();
    }
  }
}  // namespace bifrost::editor
