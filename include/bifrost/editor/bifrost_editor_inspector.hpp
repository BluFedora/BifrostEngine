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
  using Selectable  = Variant<IBaseObject*, Entity*, BaseAssetHandle>;
  using InspectorID = std::uint8_t;

  static constexpr char        k_InspectorWindowName[]       = "Inspector Window";
  static constexpr char        k_InspectorWindowID[5]        = {'#', '#', '#', 'i', 'v'};
  static constexpr char        k_InspectorWindowNameFormat[] = "%s###iv%i";
  static constexpr std::size_t k_Uint8MaxCharacters          = 3;
  static constexpr std::size_t k_InspectorIDBufferSize       = sizeof(k_InspectorWindowName) + sizeof(k_InspectorWindowID) + k_Uint8MaxCharacters;  // 'Inpector Window###iv<int><int><int><nul>'

  class Inspector final
  {
   private:
    static InspectorID s_InspectorIDGenerator;

   private:
    Selectable      m_SelectedObject;
    ImGuiSerializer m_Serializer;
    bool            m_IsLocked;
    char            m_ID[k_InspectorIDBufferSize];

   public:
    explicit Inspector(IMemoryManager& memory) :
      m_SelectedObject{},
      m_Serializer{memory},
      m_IsLocked{false},
      m_ID{'\0'}
    {
      const int id_number = int(s_InspectorIDGenerator++);

      string_utils::fmtBuffer(m_ID, sizeof(m_ID), nullptr, k_InspectorWindowNameFormat, k_InspectorWindowName, id_number);
    }

    const char* windowID() const { return m_ID; }

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
