/******************************************************************************/
/*!
* @file   bifrost_editor_inspector.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Object Editing managment for the editor.
*
* @version 0.0.1
* @date    2020-03-21
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_EDITOR_INSPECTOR_HPP
#define BIFROST_EDITOR_INSPECTOR_HPP

#include "bifrost_editor_serializer.hpp"  // ImGuiSerializer
#include "bifrost_editor_window.hpp"      // EditorWindow

namespace bf::editor
{
  class Inspector final : public EditorWindow<Inspector>
  {
   private:
    Array<Selectable> m_LockedSelection;
    ImGuiSerializer   m_Serializer;
    bool              m_IsLocked;

   public:
    explicit Inspector(IMemoryManager& memory);

   protected:
    const char* title() const override { return "Inspector View"; }
    void        onDrawGUI(EditorOverlay& editor) override;

   private:
    void guiDrawSelection(Engine& engine, const Selectable& selectable);
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_INSPECTOR_HPP */
