#include "bifrost/editor/bifrost_editor_overlay.hpp"

#include "bifrost/bifrost.hpp"
#include "bifrost/core/bifrost_engine.hpp"
#include "bifrost/data_structures/bifrost_intrusive_list.hpp"

#include <imgui/imgui.h>          /* ImGUI::* */
#include <nativefiledialog/nfd.h> /* nfd**    */

namespace bifrost::editor
{
  using namespace intrusive;

  struct TestNode
  {
    Node<TestNode> update_list;
    int            data;

    TestNode(int data) :
      update_list{},
      data{data}
    {
    }

    ~TestNode()
    {
      bfLogPrint("~%i", data);
    }
  };

  void EditorOverlay::onCreate(BifrostEngine& engine)
  {
    // TestNode nodes_memory[] = {0, 1, 2, 3, 4, 5};
    // 
    // ListView<TestNode> my_list{&TestNode::update_list};
    // 
    // for (auto& node : nodes_memory)
    // {
    //   my_list.pushBack(node);
    // }
    // 
    // bfLogPrint("Node List = {");
    // 
    // for (const auto& node : my_list)
    // {
    //   bfLogPrint("%i,", node.data);
    // }
    // 
    // bfLogPrint("}");
    // 
    // // auto i = my_list.erase(my_list.makeIterator(nodes_memory[4]));
    // 
    // my_list.popFront();
    // 
    // bfLogPrint("Node List = {");
    // 
    // for (const auto& node : my_list)
    // {
    //   bfLogPrint("%i,", node.data);
    // }
    // 
    // bfLogPrint("}");

    // List<TestNode> l{engine.mainMemory()};
    // 
    // for (int i = 0; i < 16; ++i)
    // {
    //   l.emplaceBack(i);
    // }
    // 
    // bfLogPrint("Node List = {");
    // 
    // for (const auto& node : l)
    // {
    //   bfLogPrint("%i,", node.data);
    // }
    // 
    // bfLogPrint("}");
  }

  void EditorOverlay::onLoad(BifrostEngine& engine)
  {
  }

  void EditorOverlay::onEvent(BifrostEngine& engine, Event& event)
  {
  }

  void EditorOverlay::onUpdate(BifrostEngine& engine, float delta_time)
  {
    float menu_bar_height;

    if (ImGui::BeginMainMenuBar())
    {
      menu_bar_height = ImGui::GetWindowSize().y;

      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::BeginMenu("New"))
        {
          if (ImGui::MenuItem("Project"))
          {
            nfdchar_t*        outPath = nullptr;
            const nfdresult_t result  = NFD_OpenDialog(nullptr, nullptr, &outPath);

            if (result == NFD_OKAY)
            {
              puts("Success!");
              puts(outPath);
              free(outPath);
            }
            else if (result == NFD_CANCEL)
            {
              puts("User pressed cancel.");
            }
            else
            {
              printf("Error: %s\n", NFD_GetError());
            }
          }

          ImGui::EndMenu();
        }

        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }
  }

  void EditorOverlay::onUnload(BifrostEngine& engine)
  {
  }

  void EditorOverlay::onDestroy(BifrostEngine& engine)
  {
  }
}  // namespace bifrost::editor
