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

#include "bifrost/asset_io/bifrost_asset_handle.hpp"    // BaseAssetHandle
#include "bifrost/data_structures/bifrost_variant.hpp"  // Variant<Ts...>
#include "bifrost_editor_serializer.hpp"                // ImGuiSerializer

namespace bifrost
{
  class Entity;
  class IBaseObject;
}  // namespace bifrost

namespace bifrost::editor
{
  using Selectable = Variant<IBaseObject*, Entity*, BaseAssetHandle>;

  class Inspector final
  {
   private:
    Selectable      m_SelectedObject;
    ImGuiSerializer m_Serializer;
    bool            m_IsLocked;

   public:
    Inspector(IMemoryManager& memory) :
      m_SelectedObject{},
      m_Serializer{memory},
      m_IsLocked{false}
    {
    }

    void select(IBaseObject* object) { selectImpl(object); }
    void select(Entity* object) { selectImpl(object); }
    void select(const BaseAssetHandle& handle) { selectImpl(handle); }
    void select(std::nullptr_t) { m_SelectedObject.destroy(); }

    void uiShow(EditorOverlay& editor);

   private:
    template<typename T>
    void selectImpl(T&& item)
    {
      if (!m_IsLocked)
      {
        m_SelectedObject = item;
      }
    }
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_INSPECTOR_HPP */
