#include "bf/editor/bifrost_editor_hierarchy.hpp"

#include "bf/editor/bf_editor_icons.hpp"
#include "bf/editor/bifrost_editor_overlay.hpp"
#include "bf/editor/bifrost_editor_serializer.hpp"

namespace bf::editor
{
  static Array<Entity*> listViewToArray(IMemoryManager& memory, const ListView<Entity>& list)
  {
    Array<Entity*> result{memory};

    for (Entity& entity : list)
    {
      result.push(&entity);
    }

    return result;
  }

  HierarchyView::HierarchyView() :
    m_SearchQuery{""},  // Filled with initial data for ImGui since ImGui functions do not accept nullptr as an input string.
    m_ExpandedState{allocator()},
    m_FilteredIn{allocator()},
    m_FilteredInBecauseOfChild{allocator()}
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
        ImGui::Separator();

        if (ImGui::BeginMenu(ICON_FA_PLUS))
        {
          if (ImGui::MenuItem("Create Empty"))
          {
            current_scene->addEntity();
            engine.assets().markDirty(current_scene.handle());
          }

          ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::EndMenuBar();
      }

      /*
      if (ImGui::Button(" " ICON_FA_ARROWS_ALT " "))
      {
      }
      ImGui::SameLine();
      if (ImGui::Button(" " ICON_FA_UNDO " "))
      {
      }
      ImGui::SameLine();
      if (ImGui::Button(" " ICON_FA_EXPAND_ALT " "))
      {
      }
      ImGui::SameLine();
      */

      imgui_ext::inspect("###SearchBar", ICON_FA_SEARCH "  Search...", m_SearchQuery, ImGuiInputTextFlags_CharsUppercase);

      const bool do_filter_entities = !m_SearchQuery.isEmpty();

      if (do_filter_entities)
      {
        ImGui::SameLine();

        if (ImGui::Button("clear"))
        {
          m_SearchQuery.clear();
        }
      }

      ImGui::Separator();

      const ImVec2 old_item_spacing = ImGui::GetStyle().ItemSpacing;

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{old_item_spacing.x, 0.0f});

      auto&                temp_mem = engine.tempMemory();
      LinearAllocatorScope mem_scope{temp_mem};
      Array<Entity*>       top_level_entities{temp_mem};
      Array<Entity*>       root_entities_as_array = listViewToArray(temp_mem, current_scene->rootEntities());

      if (do_filter_entities)
      {
        Array<Entity*>        entities_to_process{temp_mem, root_entities_as_array};
        UnorderedSet<Entity*> processed_entities{temp_mem};

        m_FilteredIn.clear();
        m_FilteredInBecauseOfChild.clear();

        while (!entities_to_process.isEmpty())
        {
          Entity* entity = entities_to_process.back();
          entities_to_process.pop();

          for (Entity& child : entity->children())
          {
            entities_to_process.push(&child);
          }

          if (isEntityFilteredIn(entity))
          {
            m_FilteredIn.insert(entity);

            while (entity && processed_entities.find(entity) == processed_entities.end())
            {
              processed_entities.insert(entity);

              Entity* const parent = entity->parent();

              if (!parent)
              {
                top_level_entities.push(entity);
              }

              entity = parent;

              if (entity && !isEntityFilteredIn(entity))
              {
                m_FilteredInBecauseOfChild.insert(entity);
              }
            }
          }
        }
      }
      else
      {
        top_level_entities.copyFrom(root_entities_as_array);
      }

      std::pair<Entity*, Entity*> parent_to = {nullptr, nullptr};  // {parent, child}

      for (Entity* const root_entity : top_level_entities)
      {
        guiEntityList(parent_to, editor, root_entity);
      }

      if (parent_to.first != nullptr && parent_to.second != nullptr)
      {
        parent_to.second->setParent(parent_to.first);
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

  void HierarchyView::guiEntityList(std::pair<Entity*, Entity*>& parent_to, EditorOverlay& editor, Entity* entity)
  {
    const bool         has_children              = !entity->children().isEmpty();
    Selection&         selection                 = editor.selection();
    const bool         is_selected               = selection.contains(entity);
    const bool         is_active                 = entity->isActive();
    ImGuiTreeNodeFlags tree_node_flags           = ImGuiTreeNodeFlags_OpenOnArrow;
    const auto         expanded_state_it         = m_ExpandedState.find(entity);  // Stored for faster 'm_ExpandedState.erase'.
    const bool         expanded_state            = expanded_state_it != m_ExpandedState.end();
    const bool         do_filter_entities        = !m_SearchQuery.isEmpty();
    const bool         is_filtered_in            = !do_filter_entities || m_FilteredIn.find(entity) != m_FilteredIn.end();
    const bool         is_filtered_in_bcuz_child = !do_filter_entities || m_FilteredInBecauseOfChild.find(entity) != m_FilteredInBecauseOfChild.end();

    if (!is_filtered_in_bcuz_child && !is_filtered_in)
    {
      return;
    }

    ImGui::PushID(entity);

    if (!has_children) tree_node_flags |= ImGuiTreeNodeFlags_Bullet;
    if (is_selected) tree_node_flags |= ImGuiTreeNodeFlags_Selected;

    if (!is_active)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
    }

    if (do_filter_entities && is_filtered_in)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    }

    bool is_opened;
    {
      LinearAllocator&     tmp_alloc = editor.engine().tempMemory();
      LinearAllocatorScope scope     = tmp_alloc;

      ImGui::SetNextItemOpen(expanded_state || do_filter_entities, ImGuiCond_Always);

      char* tree_node_name = string_utils::fmtAlloc(tmp_alloc, nullptr, "%s %s", ICON_FA_DICE_D6, entity->name().c_str());
      is_opened            = ImGui::TreeNodeEx(tree_node_name, tree_node_flags);
    }

    if (!do_filter_entities)
    {
      if (is_opened != expanded_state)
      {
        if (is_opened)
        {
          m_ExpandedState.insert(entity);
        }
        else
        {
          m_ExpandedState.erase(expanded_state_it);
        }
      }
    }

    if (do_filter_entities && is_filtered_in)
    {
      ImGui::PopStyleColor();
    }

    if (!is_active)
    {
      ImGui::PopStyleColor();
    }

    ImGuiDragDropFlags src_flags = 0;
    src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;      // Keep the source displayed as hovered
    src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers;  // Because our dragging is local, we disable the
                                                               // feature of opening foreign tree nodes/tabs while
                                                               // dragging
                                                               // src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide
                                                               // the tooltip

    if (ImGui::BeginPopupContextItem(nullptr))
    {
      if (ImGui::Selectable("Toggle Active"))
      {
        editor.history().performLambdaAction(
         "Toggle Entity Active",
         [entity, original_state = entity->isActiveSelf()](UndoRedoEventType evt) {
           if (evt == UndoRedoEventType::ON_REDO)
           {
             entity->setActiveSelf(!original_state);
           }
           else if (evt == UndoRedoEventType::ON_UNDO)
           {
             entity->setActiveSelf(original_state);
           }
         });
      }

      // if (ImGui::Selectable("Delete"))
      {
        // editor.undoRedo().doCommand(cmd::deleteEntity(*entity));
      }

      ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropSource(src_flags))
    {
      if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
      {
        ImGui::Text("ENTITY: \"%s\"", entity->name().c_str());
      }

      ImGui::SetDragDropPayload("DROP_ENTITY", &entity, sizeof(Entity*));
      ImGui::EndDragDropSource();
    }

    if (!ImGui::IsItemToggledOpen())
    {
      if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
      {
        selection.clear();
        selection.select(entity);
      }
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

    if (is_opened)
    {
      for (Entity& child : entity->children())
      {
        guiEntityList(parent_to, editor, &child);
      }

      ImGui::TreePop();
    }

    ImGui::PopID();
  }

  bool HierarchyView::isEntityFilteredIn(const Entity* entity) const
  {
    using namespace string_utils;

    const String& name     = entity->name();
    bool          is_found = stringMatchPercent(name, m_SearchQuery) >= 0.65f;

    if (!is_found)
    {
      tokenize(name, ' ', [this, &is_found](StringRange element) {
        is_found = is_found || stringMatchPercent(element, m_SearchQuery) >= 0.65f;
      });
    }

    return is_found || findSubstringI(name, m_SearchQuery) != nullptr;
  }
}  // namespace bf::editor
