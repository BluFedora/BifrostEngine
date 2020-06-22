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

    std::pair<Entity*, Entity*> parent_to{nullptr, nullptr}; /* {parent, child} */

    for (Entity* const root_entity : current_scene->rootEntities())
    {
      ImGui::PushID(root_entity);

      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

      const auto is_selected = false;

      if (root_entity->children().isEmpty())
      {
        flags |= ImGuiTreeNodeFlags_Bullet;
      }

      if (is_selected)
      {
        flags |= ImGuiTreeNodeFlags_Selected;
      }

      const bool is_opened = ImGui::TreeNodeEx(root_entity->name().c_str(), flags);

      if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
      {
        editor.select(root_entity);
      }

      ImGuiDragDropFlags src_flags = 0;
      src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;      // Keep the source displayed as hovered
      src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers;  // Because our dragging is local, we disable the
                                                                 // feature of opening foreign treenodes/tabs while
                                                                 // dragging
                                                                 // src_flags |=
                                                                 // ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide
                                                                 // the tooltip
      if (ImGui::BeginDragDropSource(src_flags))
      {
        if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        {
          ImGui::Text("Moving \"%s\"", root_entity->name().c_str());
        }

        ImGui::SetDragDropPayload("DROP_ENTITY", &root_entity, sizeof(Entity*));
        ImGui::EndDragDropSource();
      }

      if (ImGui::BeginDragDropTarget())
      {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        if (payload && payload->IsDataType("DROP_ENTITY"))
        {
          Entity* data = *static_cast<Entity**>(payload->Data);

          assert(payload->DataSize == sizeof(Entity*));

          if (ImGui::AcceptDragDropPayload("DROP_ENTITY", ImGuiDragDropFlags_None))
          {
            bfLogPrint("%s was dropped onto %s", data->name().c_str(), root_entity->name().c_str());

            parent_to.first  = root_entity;
            parent_to.second = data;
          }
        }

        ImGui::EndDragDropTarget();
      }

      ImGui::InvisibleButton("Reorder", ImVec2{ImGui::GetWindowSize().x, 2.0f});

      if (ImGui::BeginDragDropTarget())
      {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        if (payload && payload->IsDataType("DROP_ENTITY"))
        {
          const Entity* data = *static_cast<const Entity**>(payload->Data);

          assert(payload->DataSize == sizeof(Entity*));

          if (ImGui::AcceptDragDropPayload("DROP_ENTITY", ImGuiDragDropFlags_None))
          {
            bfLogPrint("%s was dropped after %s", data->name().c_str(), root_entity->name().c_str());
          }
        }

        ImGui::EndDragDropTarget();
      }

      if (is_opened)
      {
        for (Entity& child : root_entity->children())
        {
          bfTransform_flushChanges(&child.transform());

          if (ImGui::TreeNodeEx(child.name().c_str(), ImGuiTreeNodeFlags_Bullet))
          {
            ImGui::TreePop();
          }
        }

        ImGui::TreePop();
      }

      bfTransform_flushChanges(&root_entity->transform());

      ImGui::PopID();
    }

    if (parent_to.first != nullptr && parent_to.second != nullptr)
    {
      parent_to.second->setParent(parent_to.first);
    }
  }

  void HierarchyView::guiEntityList(EditorOverlay& editor, Entity* entity)
  {
    ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnArrow;

    ImGui::PushID(entity);



    ImGui::PopID();
  }
}  // namespace bifrost::editor
