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

    if (!m_SearchQuery.isEmpty())
    {
      if (ImGui::Button("clear"))
      {
        m_SearchQuery.clear();
      }

      ImGui::SameLine();
    }

    imgui_ext::inspect("Search", m_SearchQuery, ImGuiInputTextFlags_CharsUppercase);

    if (ImGui::Button("Add Entity"))
    {
      current_scene->addEntity();

      engine.assets().markDirty(current_scene);
    }

    ImGui::Separator();

    static bool is_overlay = false;

    ImGui::Checkbox("Is Overlay", &is_overlay);

    for (Entity* const root_entity : current_scene->rootEntities())
    {
      ImGui::PushID(root_entity);
      if (ImGui::Selectable(root_entity->name().cstr()))
      {
        editor.select(root_entity);
      }

      bfTransform_flushChanges(&root_entity->transform());

      engine.debugDraw().addAABB(
       root_entity->transform().world_position,
       root_entity->transform().world_scale,
       bfColor4u_fromUint32(BIFROST_COLOR_SALMON),
       0.0f,
       is_overlay);

      ImGui::PopID();
    }
  }
}  // namespace bifrost::editor
