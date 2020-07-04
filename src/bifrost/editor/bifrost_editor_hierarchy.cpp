#include "bifrost/editor/bifrost_editor_hierarchy.hpp"

#include "bifrost/editor/bifrost_editor_overlay.hpp"
#include "bifrost/editor/bifrost_editor_serializer.hpp"

namespace bifrost::editor
{
  HierarchyView::HierarchyView() :
    m_SearchQuery{""}  // Filled with initial data for ImGui since ImGui functions do not accept nullptr as an input string.
  {
  }

  void HierarchyView::onDrawGUI(EditorOverlay& editor)
  {
    auto&      engine        = editor.engine();
    const auto current_scene = engine.currentScene();

    if (current_scene)
    {
      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("+ Entity"))
        {
          if (ImGui::MenuItem("Create Empty"))
          {
            current_scene->addEntity();

            engine.assets().markDirty(current_scene);
          }

          ImGui::EndMenu();
        }

        ImGui::Separator();

        imgui_ext::inspect("###SearchBar", "Search...", m_SearchQuery, ImGuiInputTextFlags_CharsUppercase);

        if (!m_SearchQuery.isEmpty())
        {
          ImGui::SameLine();

          if (ImGui::Button("clear"))
          {
            m_SearchQuery.clear();
          }
        }

        ImGui::EndMenuBar();
      }

      const ImVec2 old_item_spacing = ImGui::GetStyle().ItemSpacing;

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{old_item_spacing.x, 0.0f});

      for (Entity* const root_entity : current_scene->rootEntities())
      {
        guiEntityList(editor, root_entity);
      }

      ImGui::PopStyleVar();
    }
    else
    {
      ImGui::TextUnformatted("(No Scene Open)");

      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Create a new Scene by right clicking a folder 'Create->Scene'\nThen double click the newly created Scene asset.");
      }
    }
  }

  void HierarchyView::guiEntityList(EditorOverlay& editor, Entity* entity) const
  {
    Selection&                  selection       = editor.selection();
    const bool                  has_children    = !entity->children().isEmpty();
    const bool                  is_selected     = editor.selection().contains(entity);
    ImGuiTreeNodeFlags          tree_node_flags = ImGuiTreeNodeFlags_OpenOnArrow;
    std::pair<Entity*, Entity*> parent_to       = {nullptr, nullptr}; /* {parent, child} */

    ImGui::PushID(entity);

    if (!has_children)
    {
      tree_node_flags |= ImGuiTreeNodeFlags_Bullet;
    }

    if (is_selected)
    {
      tree_node_flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool is_opened = ImGui::TreeNodeEx(entity->name().c_str(), tree_node_flags);

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
        ImGui::Text("ENTITY: \"%s\"", entity->name().c_str());
      }

      ImGui::SetDragDropPayload("DROP_ENTITY", &entity, sizeof(Entity*));
      ImGui::EndDragDropSource();
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
      selection.clear();
      selection.select(entity);
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
          bfLogPrint("%s was dropped onto %s", data->name().c_str(), entity->name().c_str());

          parent_to.first  = entity;
          parent_to.second = data;
        }
      }

      ImGui::EndDragDropTarget();
    }

    if (is_opened)
    {
      if (has_children)
      {
        for (Entity& child : entity->children())
        {
          guiEntityList(editor, &child);
        }
      }

      ImGui::TreePop();
    }

    ImGui::InvisibleButton("Reorder", ImVec2{ImGui::GetWindowContentRegionWidth(), 2.0f});

    if (ImGui::BeginDragDropTarget())
    {
      const ImGuiPayload* payload = ImGui::GetDragDropPayload();

      if (payload && payload->IsDataType("DROP_ENTITY"))
      {
        const Entity* data = *static_cast<const Entity**>(payload->Data);

        assert(payload->DataSize == sizeof(Entity*));

        if (data != entity && ImGui::AcceptDragDropPayload("DROP_ENTITY", ImGuiDragDropFlags_None))
        {
          bfLogPrint("%s was dropped after %s", data->name().c_str(), entity->name().c_str());
        }
      }

      ImGui::EndDragDropTarget();
    }

    ImGui::PopID();

    if (parent_to.first != nullptr && parent_to.second != nullptr)
    {
      parent_to.second->setParent(parent_to.first);
    }

    bfTransform_flushChanges(&entity->transform());
  }
}  // namespace bifrost::editor
