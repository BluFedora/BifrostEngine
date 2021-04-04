/******************************************************************************/
/*!
* @file   bifrost_editor_serializer.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-16
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_EDITOR_SERIALIZER_HPP
#define BIFROST_EDITOR_SERIALIZER_HPP

#include "bf/asset_io/bf_iserializer.hpp"  // ISerializer
#include "bifrost_editor_undo_redo.hpp"

#include <imgui/imgui.h>  // ImGui::*

namespace bf
{
  class Entity;
  class Assets;
}  // namespace bf

namespace bf::editor
{
  static constexpr int k_FieldNameBufferSize = 128;  //!< You shouldn't have a field with a name bigger than this right?

  class ImGuiSerializer;

  struct SerializerChangeInfo
  {
    static constexpr std::uint8_t HAS_BEGAN_CHANGING    = bfBit(0);
    static constexpr std::uint8_t HAS_BEEN_CHANGED      = bfBit(1);
    static constexpr std::uint8_t HAS_FINISHED_CHANGING = bfBit(2);

    std::uint8_t flags = 0x0;

    // The default checks is just if it has changed this frame.
    operator bool() const { return hasChanged(); }
    bool hasBeganChanging() const { return check(HAS_BEGAN_CHANGING); }
    bool hasChanged() const { return check(HAS_BEEN_CHANGED); }
    bool hasFinishedChanging() const { return check(HAS_FINISHED_CHANGING); }
    bool check(std::uint8_t flag) const { return flags & flag; }
    void set(std::uint8_t flag) { flags |= flag; }
  };

  //
  // For adding custom callbacks for certain types.
  //
  namespace InspectorRegistry
  {
    using Callback = void (*)(ImGuiSerializer& serializer, meta::MetaVariant& object, void* user_data);

    void overrideInspectorImpl(meta::BaseClassMetaInfo* type_info, Callback callback, void* user_data);

    template<typename T>
    void overrideInspector(Callback callback, void* user_data = nullptr)
    {
      overrideInspectorImpl(meta::TypeInfo<T>::get(), callback, user_data);
    }
  }  // namespace InspectorRegistry

  class ImGuiSerializer final : public ISerializer
  {
    struct ObjectStackInfo
    {
      char name[k_FieldNameBufferSize];
      bool is_array;
      int  array_index;
    };

   private:
    Array<ObjectStackInfo>      m_IsOpenStack;
    Array<SerializerChangeInfo> m_HasChangedStack;
    char                        m_NameBuffer[k_FieldNameBufferSize];
    Assets*                     m_Assets;
    bool                        m_IsInCustomCallback;

   public:
    explicit ImGuiSerializer(IMemoryManager& memory);

    void setAssets(Assets* assets) { m_Assets = assets; }

    bool beginDocument() override;
    bool pushObject(StringRange key) override;
    bool pushArray(StringRange key, std::size_t& size) override;
    void serialize(StringRange key, bool& value) override;
    void serialize(StringRange key, std::int8_t& value) override;
    void serialize(StringRange key, std::uint8_t& value) override;
    void serialize(StringRange key, std::int16_t& value) override;
    void serialize(StringRange key, std::uint16_t& value) override;
    void serialize(StringRange key, std::int32_t& value) override;
    void serialize(StringRange key, std::uint32_t& value) override;
    void serialize(StringRange key, std::int64_t& value) override;
    void serialize(StringRange key, std::uint64_t& value) override;
    void serialize(StringRange key, float& value) override;
    void serialize(StringRange key, double& value) override;
    void serialize(StringRange key, long double& value) override;
    void serialize(StringRange key, Vec2f& value) override;
    void serialize(StringRange key, Vec3f& value) override;
    void serialize(StringRange key, Quaternionf& value) override;
    void serialize(StringRange key, bfColor4f& value) override;
    void serialize(StringRange key, bfColor4u& value) override;
    void serialize(StringRange key, String& value) override;
    void serialize(StringRange key, bfUUIDNumber& value) override;
    void serialize(StringRange key, bfUUID& value) override;
    void serialize(StringRange key, IARCHandle& value) override;
    void serialize(StringRange key, EntityRef& value) override;
    void serialize(StringRange key, meta::MetaObject& value) override;
    void serialize(meta::MetaVariant& value) override;
    using ISerializer::serialize;
    void popObject() override;
    void popArray() override;
    void endDocument() override;

    void                 beginChangedCheck();
    SerializerChangeInfo endChangedCheck();

   public: /* private: */
    ObjectStackInfo& top() { return m_IsOpenStack.back(); }
    void             setNameBuffer(StringRange key);
    void             updateTopChangedStackItem();
    void             setTopChangedStackItemFlags(std::uint8_t flags);
  };

  namespace imgui_ext
  {
    bool inspect(const char* label, String& string, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
    bool inspect(const char* label, const char* hint, String& string, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
    bool inspect(const char* label, std::string& string, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
    void inspect(Engine& engine, Entity& entity, ImGuiSerializer& serializer);
    bool inspect(History& history, Engine& engine, Entity& entity, ImGuiSerializer& serializer);
  }  // namespace imgui_ext
}  // namespace bf::editor

#endif /* BIFROST_EDITOR_SERIALIZER_HPP */