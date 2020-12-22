/******************************************************************************/
/*!
* @file   bifrost_editor_serializer.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-16
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bf/editor/bifrost_editor_serializer.hpp"

#include "bf/LinearAllocator.hpp"  // LinearAllocator
#include "bf/asset_io/bf_base_asset.hpp"
#include "bf/asset_io/bifrost_assets.hpp"
#include "bf/asset_io/bifrost_script.hpp"
#include "bf/bifrost_math.h"
#include "bf/core/bifrost_engine.hpp"
#include "bf/ecs/bifrost_behavior.hpp"
#include "bf/ecs/bifrost_entity.hpp"
#include "bf/math/bifrost_vec2.h"
#include "bf/math/bifrost_vec3.h"
#include "bf/script/bifrost_script_behavior.hpp"

namespace bf::editor
{
  static constexpr int   s_MaxDigitsUInt64  = 20;
  static constexpr float s_DragSpeed        = 0.05f;
  static constexpr float k_SmallWindowWidth = 150.0f;

  namespace InspectorRegistry
  {
    struct RegistryValue final
    {
      Callback callback;
      void*    user_data;
    };

    static HashTable<meta::BaseClassMetaInfo*, RegistryValue> s_Registry = {};  // NOLINT(clang-diagnostic-exit-time-destructors)

    void overrideInspectorImpl(meta::BaseClassMetaInfo* type_info, Callback callback, void* user_data)
    {
      s_Registry[type_info] = {callback, user_data};
    }
  }  // namespace InspectorRegistry

  ImGuiSerializer::ImGuiSerializer(IMemoryManager& memory) :
    ISerializer(SerializerMode::INSPECTING),
    m_IsOpenStack{memory},
    m_HasChangedStack{memory},
    m_NameBuffer{'\0'},
    m_Assets{nullptr},
    m_IsInCustomCallback{false}
  {
  }

  bool ImGuiSerializer::beginDocument(bool is_array)
  {
    auto& obj    = m_IsOpenStack.emplace();
    obj.is_array = is_array;
    string_utils::fmtBuffer(obj.name, k_FieldNameBufferSize, nullptr, "__DOCUMENT");

    beginChangeCheck();
    return true;
  }

  bool ImGuiSerializer::pushObject(StringRange key)
  {
    setNameBuffer(key);
    const bool is_open = ImGui::TreeNode(m_NameBuffer);

    if (is_open)
    {
      auto& obj    = m_IsOpenStack.emplace();
      obj.is_array = false;

      if (!string_utils::fmtBuffer(obj.name, k_FieldNameBufferSize, nullptr, "%.*s", int(key.length()), key.begin()))
      {
        bfLogWarn("Failed to format name string in ImGuiSerializer::pushObject.");
      }
    }

    return is_open;
  }

  bool ImGuiSerializer::pushArray(StringRange key, std::size_t& size)
  {
    setNameBuffer(key);
    const bool is_open = ImGui::TreeNode(m_NameBuffer);

    if (is_open)
    {
      auto& obj       = m_IsOpenStack.emplace();
      obj.is_array    = true;
      obj.array_index = 0;

      if (!string_utils::fmtBuffer(obj.name, k_FieldNameBufferSize, nullptr, "%.*s", int(key.length()), key.begin()))
      {
        bfLogWarn("Failed to format name string in ImGuiSerializer::pushArray.");
      }
    }

    size = 0;
    return is_open;
  }

  void ImGuiSerializer::serialize(StringRange key, bool& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::Checkbox(m_NameBuffer, &value);
  }

  void ImGuiSerializer::serialize(StringRange key, std::int8_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_S8, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, std::uint8_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_U8, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, std::int16_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_S16, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, std::uint16_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_U16, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, std::int32_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_S32, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, std::uint32_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_U32, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, std::int64_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_S64, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, std::uint64_t& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_U64, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, float& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_Float, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, double& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_Double, &value, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, long double& value)
  {
    double value_d = double(value);

    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_Double, &value_d, s_DragSpeed, nullptr, nullptr, "%.3f", 1.0f);
    value = value_d;
  }

  void ImGuiSerializer::serialize(StringRange key, Vec2f& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalarN(m_NameBuffer, ImGuiDataType_Float, &value.x, 2, s_DragSpeed, nullptr, nullptr, "%.3f", 1.0f);
  }

  void ImGuiSerializer::serialize(StringRange key, Vec3f& value)
  {
    const float available_width = ImGui::GetContentRegionAvail().x;

    setNameBuffer(key);

    if (available_width <= k_SmallWindowWidth)
    {
      if (ImGui::TreeNodeEx(m_NameBuffer, ImGuiTreeNodeFlags_DefaultOpen))
      {
        serialize("x", value.x);
        serialize("y", value.y);
        serialize("z", value.z);

        ImGui::TreePop();
      }
    }
    else
    {
      hasChangedTop() |= ImGui::DragScalarN(m_NameBuffer, ImGuiDataType_Float, &value.x, 3, s_DragSpeed, nullptr, nullptr, "%.3f", 1.0f);
    }

#if 0
    if (did_change)
    {
      bfLogPrint("Editing: %s", m_NameBuffer);
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
      bfLogPush("Finished Editing:");
      for (const auto& obj : m_IsOpenStack)
      {
        bfLogPrint("%s->", obj.name);
      }
      bfLogPrint("%s", m_NameBuffer);
      bfLogPop();
    }
#endif
  }

  void ImGuiSerializer::serialize(StringRange key, Quaternionf& value)
  {
    Vector3f euler_deg;

    bfQuaternionf_toEulerDeg(&value, &euler_deg);

    beginChangeCheck();

    serialize(key, euler_deg);

    if (endChangedCheck())
    {
      value = bfQuaternionf_fromEulerDeg(euler_deg.x, euler_deg.y, euler_deg.z);
    }
  }

  void ImGuiSerializer::serialize(StringRange key, bfColor4f& value)
  {
    const float         available_width    = ImGui::GetContentRegionAvail().x;
    ImGuiColorEditFlags color_picker_flags = ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel;

    if (available_width <= k_SmallWindowWidth)
    {
      color_picker_flags |= ImGuiColorEditFlags_NoInputs;

      // TODO(SR): Also draw a Vec4 like vertical GUI?
    }

    setNameBuffer(key);

    hasChangedTop() |= ImGui::ColorEdit4(m_NameBuffer, &value.r, color_picker_flags);
  }

  void ImGuiSerializer::serialize(StringRange key, bfColor4u& value)
  {
    static constexpr float k_ToFloatingPoint = 1.0f / 255.0f;
    static constexpr float k_ToUint8Point    = 255.0f;

    const float         available_width    = ImGui::GetContentRegionAvail().x;
    ImGuiColorEditFlags color_picker_flags = ImGuiColorEditFlags__OptionsDefault;

    if (available_width <= k_SmallWindowWidth)
    {
      color_picker_flags |= ImGuiColorEditFlags_NoInputs;

      // TODO(SR): Also draw a Vec4 like vertical GUI?
    }

    setNameBuffer(key);

    bfColor4f value4f =
     {
      float(value.r) * k_ToFloatingPoint,
      float(value.g) * k_ToFloatingPoint,
      float(value.g) * k_ToFloatingPoint,
      float(value.a) * k_ToFloatingPoint,
     };

    const bool has_changed = ImGui::ColorEdit4(m_NameBuffer, &value4f.r, color_picker_flags);

    if (has_changed)
    {
      value.r = std::uint8_t(std::round(value4f.r * k_ToUint8Point));
      value.g = std::uint8_t(std::round(value4f.g * k_ToUint8Point));
      value.b = std::uint8_t(std::round(value4f.b * k_ToUint8Point));
      value.a = std::uint8_t(std::round(value4f.a * k_ToUint8Point));
    }

    hasChangedTop() |= has_changed;
  }

  void ImGuiSerializer::serialize(StringRange key, String& value)
  {
    setNameBuffer(key);

    hasChangedTop() |= imgui_ext::inspect(m_NameBuffer, value);
  }

  void ImGuiSerializer::serialize(StringRange key, bfUUIDNumber& value)
  {
    setNameBuffer(key);

    char as_string[37];
    bfUUID_numberToString(value.data, as_string);

    ImGui::Text("UUID <%s>", as_string);
  }

  void ImGuiSerializer::serialize(StringRange key, bfUUID& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::InputText(m_NameBuffer, value.as_string.data, sizeof(value.as_string), ImGuiInputTextFlags_ReadOnly);
  }

  void ImGuiSerializer::serialize(StringRange key, BaseAssetHandle& value)
  {
    setNameBuffer(key);

    ImGui::PushID(&value);

    if (value)
    {
      if (ImGui::Button("clear"))
      {
        value.release();
        hasChangedTop() |= true;
      }

      ImGui::SameLine();
    }

    const char* const preview_name  = value ? value.info()->filePathRel().begin() : "<null>";
    BaseAssetInfo*    assigned_info = nullptr;

    if (ImGui::BeginCombo(m_NameBuffer, preview_name, ImGuiComboFlags_HeightLargest))
    {
      for (const auto& asset_info_it : m_Assets->assetMap())
      {
        BaseAssetInfo* const asset_info = asset_info_it.value();

        if (Assets::isHandleCompatible(value, asset_info))
        {
          const bool is_selected = asset_info == value.info();

          if (ImGui::Selectable(asset_info->filePathRel().begin(), is_selected, ImGuiSelectableFlags_None))
          {
            if (!is_selected)
            {
              assigned_info = asset_info;
            }
          }
        }
      }

      ImGui::EndCombo();
    }

    if (ImGui::BeginDragDropTarget())
    {
      const ImGuiPayload* payload = ImGui::GetDragDropPayload();

      if (payload && payload->IsDataType("Asset.UUID"))
      {
        const bfUUID* data = static_cast<const bfUUID*>(payload->Data);

        assert(payload->DataSize == sizeof(bfUUID));

        const auto info = m_Assets->findAssetInfo(*data);

        if (info && Assets::isHandleCompatible(value, info) && ImGui::AcceptDragDropPayload("Asset.UUID", ImGuiDragDropFlags_None))
        {
          assigned_info = info;
        }
      }

      ImGui::EndDragDropTarget();
    }

    if (assigned_info)
    {
      if (m_Assets->tryAssignHandle(value, assigned_info))
      {
        hasChangedTop() |= true;
      }
    }

    ImGui::PopID();
  }

  void ImGuiSerializer::serialize(StringRange key, IARCHandle& value)
  {
    setNameBuffer(key);

    ImGui::PushID(&value);

    if (value.isValid())
    {
      if (ImGui::Button("clear"))
      {
        value.assign(nullptr);
        hasChangedTop() |= true;
      }

      ImGui::SameLine();
    }

    const IBaseAsset* const current_asset  = value.handle();
    const char* const       preview_name   = value.isValid() ? current_asset->relativePath().begin() : "<null>";
    IBaseAsset*             assigned_asset = nullptr;

    if (ImGui::BeginCombo(m_NameBuffer, preview_name, ImGuiComboFlags_HeightLargest))
    {
      m_Assets->forEachAssetOfType(
       value.typeInfo(), [current_asset, &assigned_asset](IBaseAsset* asset) {
         const bool is_selected = current_asset == asset;

         if (ImGui::Selectable(asset->relativePath().begin(), is_selected, ImGuiSelectableFlags_None))
         {
           if (!is_selected)
           {
             assigned_asset = asset;
           }
         }
       });

      ImGui::EndCombo();
    }

    if (ImGui::BeginDragDropTarget())
    {
      const ImGuiPayload* payload = ImGui::GetDragDropPayload();

      if (payload && payload->IsDataType("Asset.UUID"))
      {
        const bfUUID* data = static_cast<const bfUUID*>(payload->Data);

        assert(payload->DataSize == sizeof(bfUUID));

        IBaseAsset* const dragged_asset = m_Assets->findAsset(data->as_number);

        if (dragged_asset &&
            dragged_asset->type() == current_asset->type() &&
            ImGui::AcceptDragDropPayload("Asset.UUID", ImGuiDragDropFlags_None))
        {
          assigned_asset = dragged_asset;
        }
      }

      ImGui::EndDragDropTarget();
    }

    if (assigned_asset)
    {
      value.assign(assigned_asset);
      hasChangedTop() |= true;
    }

    ImGui::PopID();
  }

  void ImGuiSerializer::serialize(StringRange key, EntityRef& value)
  {
    setNameBuffer(key);

    ImGui::PushID(&value);

    if (value)
    {
      if (ImGui::Button("clear"))
      {
        value = nullptr;
        hasChangedTop() |= true;
      }

      ImGui::SameLine();
    }

    Entity* const     entity          = value.object();
    const char* const preview_name    = entity ? entity->name().c_str() : "<null>";
    Entity*           assigned_entity = entity;

    if (ImGui::BeginCombo(m_NameBuffer, preview_name, ImGuiComboFlags_HeightRegular))
    {
      const auto scene = m_Assets->engine().currentScene();

      if (scene)
      {
        for (Entity* const scene_entity : scene->rootEntities())
        {
          const bool is_selected = scene_entity == entity;

          ImGui::PushID(scene_entity);

          if (ImGui::Selectable(scene_entity->name().c_str(), is_selected, ImGuiSelectableFlags_None))
          {
            if (!is_selected)
            {
              assigned_entity = scene_entity;
            }
          }

          ImGui::PopID();
        }
      }

      ImGui::EndCombo();
    }

    // TODO(SR): All drag / drop areas should be put in function for maintenance reasons.
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DROP_ENTITY", ImGuiDragDropFlags_None))
      {
        assigned_entity = *static_cast<Entity**>(payload->Data);
      }

      ImGui::EndDragDropTarget();
    }

    if (assigned_entity != entity)
    {
      value = EntityRef(assigned_entity);

      hasChangedTop() |= true;
    }

    ImGui::PopID();
  }

  void ImGuiSerializer::serialize(StringRange key, meta::MetaObject& value)
  {
    auto* const type_info = value.type_info;

    if (type_info->isEnum())
    {
      setNameBuffer(key);

      const std::uint64_t original_value = value.enum_value;
      const char*         preview_name   = nullptr;

      for (const auto& props : type_info->properties())
      {
        if (meta::variantToCompatibleT<std::uint64_t>(props->get(nullptr)) == original_value)
        {
          preview_name = props->name().data();
          break;
        }
      }

      if (ImGui::BeginCombo(m_NameBuffer, preview_name, ImGuiComboFlags_None))
      {
        for (const auto& props : type_info->properties())
        {
          const std::uint64_t props_value = meta::variantToCompatibleT<std::uint64_t>(props->get(nullptr));

          if (ImGui::Selectable(props->name().data(), props_value == original_value, ImGuiSelectableFlags_None))
          {
            value.enum_value = props_value;
          }
        }

        ImGui::EndCombo();
      }

      hasChangedTop() |= original_value != value.enum_value;
    }
    else
    {
      ISerializer::serialize(key, value);
    }
  }

  void ImGuiSerializer::serialize(meta::MetaVariant& value)
  {
    // If the user calls this function from the custom
    // callback then perform default inspection.

    if (m_IsInCustomCallback)
    {
      ISerializer::serialize(value);
      return;
    }

    auto* const custom_entry = InspectorRegistry::s_Registry.at(variantTypeInfo(value));

    if (custom_entry)
    {
      m_IsInCustomCallback = true;
      custom_entry->callback(*this, value, custom_entry->user_data);
      m_IsInCustomCallback = false;
    }
    else
    {
      ISerializer::serialize(value);
    }
  }

  void ImGuiSerializer::popObject()
  {
    ImGui::TreePop();
    m_IsOpenStack.pop();
  }

  void ImGuiSerializer::popArray()
  {
    popObject();
  }

  void ImGuiSerializer::endDocument()
  {
    endChangedCheck();
    m_IsOpenStack.pop();
  }

  void ImGuiSerializer::beginChangeCheck()
  {
    m_HasChangedStack.emplace(false);
  }

  bool ImGuiSerializer::endChangedCheck()
  {
    const bool result = m_HasChangedStack.back();
    m_HasChangedStack.pop();

    // Adopt the status of enclosed change check scopes.
    if (!m_HasChangedStack.isEmpty())
    {
      hasChangedTop() |= result;
    }

    return result;
  }

  void ImGuiSerializer::setNameBuffer(StringRange key)
  {
    static_assert(sizeof(m_NameBuffer) == k_FieldNameBufferSize, "Sanity check to make sure I didn't upgrade 'm_NameBuffer' to a String or something.");

    using namespace string_utils;

    bool  name_is_valid;
    auto& obj = top();

    if (obj.is_array)
    {
      name_is_valid = fmtBuffer(m_NameBuffer, k_FieldNameBufferSize, nullptr, "%i", obj.array_index);
      ++obj.array_index;
    }
    else
    {
      name_is_valid = fmtBuffer(m_NameBuffer, k_FieldNameBufferSize, nullptr, "%.*s", int(key.length()), key.begin());
    }

    //
    // The 'fmtBuffer' functions is safe from buffer overflow,
    // but it is still nice to know if we encountered a cut off name!
    //
    if (!name_is_valid)
    {
      bfLogWarn("Field name Too Long (len > %i): %.*s", k_FieldNameBufferSize, int(key.length()), key.begin());
    }
  }

  static int ImGuiStringCallback(ImGuiInputTextCallbackData* data);
  static int ImGuiStdStringCallback(ImGuiInputTextCallbackData* data);

  bool imgui_ext::inspect(const char* label, String& string, ImGuiInputTextFlags flags)
  {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label, const_cast<char*>(string.cstr()), string.capacity(), flags, &ImGuiStringCallback, static_cast<void*>(&string));
  }

  bool imgui_ext::inspect(const char* label, const char* hint, String& string, ImGuiInputTextFlags flags)
  {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputTextWithHint(label, hint, const_cast<char*>(string.cstr()), string.capacity(), flags, &ImGuiStringCallback, static_cast<void*>(&string));
  }

  bool imgui_ext::inspect(const char* label, std::string& string, ImGuiInputTextFlags flags)
  {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label, const_cast<char*>(string.c_str()), string.capacity(), flags, &ImGuiStdStringCallback, static_cast<void*>(&string));
  }

  bool imgui_ext::inspect(Engine& engine, Entity& entity, ImGuiSerializer& serializer)
  {
    ImGui::PushID(&entity);

    entity.serialize(serializer);

    bool has_missing_component = false;

    ComponentStorage::forEachType([&has_missing_component, &engine, &entity, &serializer](auto t) {
      using T = bfForEachTemplateT(t);

      const StringRange& component_name = g_EngineComponentInfo[t.index].name;

      if (entity.has<T>())
      {
        LinearAllocatorScope mem_scope{engine.tempMemory()};
        char*                label = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "[%.*s]", int(component_name.length()), component_name.begin());

        ImGui::PushID(label);

        bool is_open = true;
        if (ImGui::CollapsingHeader(label, &is_open, ImGuiTreeNodeFlags_None))
        {
          ImGui::Indent();

          bool is_active = entity.isComponentActive<T>();

          serializer.beginChangeCheck();
          serializer.serialize("Is Active", is_active);

          if (serializer.endChangedCheck())
          {
            entity.setComponentActive<T>(is_active);
          }

          serializer.serializeT(entity.get<T>());
          ImGui::Unindent();

          ImGui::Separator();
        }

        if (!is_open)
        {
          entity.remove<T>();
          has_missing_component = true;
        }

        ImGui::PopID();
      }
      else
      {
        has_missing_component = true;
      }
    });

    IBehavior* behavior_to_remove = nullptr;

    for (IBehavior* behavior : entity.behaviors())
    {
      const auto& name = behavior->type()->name();

      LinearAllocatorScope mem_scope{engine.tempMemory()};
      char*                label = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "[%.*s]", int(name.length()), name.data());

      ImGui::PushID(behavior);

      ImGui::Separator();

      bool is_open = true;
      if (ImGui::CollapsingHeader(label, &is_open, ImGuiTreeNodeFlags_None))
      {
        ImGui::Indent();

        behavior->serialize(serializer);

        ImGui::Unindent();

        ImGui::Separator();
      }

      if (!is_open)
      {
        behavior_to_remove = behavior;
      }

      ImGui::PopID();
    }

    if (behavior_to_remove)
    {
      entity.removeBehavior(behavior_to_remove);
    }

    ImGui::PopID();

    if (has_missing_component)
    {
      if (ImGui::BeginCombo("Add Component", "+ Component", ImGuiComboFlags_None))
      {
        ComponentStorage::forEachType([&entity, &engine](auto t) {
          using T = bfForEachTemplateT(t);

          const StringRange& component_name = g_EngineComponentInfo[t.index].name;

          if (!entity.has<T>())
          {
            LinearAllocatorScope mem_scope{engine.tempMemory()};
            char*                label = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "Add %.*s", int(component_name.length()), component_name.begin());

            if (ImGui::Selectable(label))
            {
              entity.add<T>();
            }
          }
        });

        ImGui::EndCombo();
      }
    }

    if (ImGui::BeginCombo("Add Behavior", "+ Behavior", ImGuiComboFlags_None))
    {
      const auto* ibehavior_class_info = meta::typeInfoGet<IBehavior>();

      for (auto& type : meta::gRegistry())
      {
        const auto& name      = type.key();
        auto* const type_info = type.value();
        const auto& bases     = type_info->baseClasses();

        if (!bases.isEmpty() && bases[0] == ibehavior_class_info)
        {
          LinearAllocatorScope mem_scope{engine.tempMemory()};

          char* label = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "Add (%.*s)", int(name.length()), name.data());

          if (ImGui::Selectable(label))
          {
            entity.addBehavior(type_info);
          }
        }
      }

      const auto* const script_class_info = meta::typeInfoGet<AssetScriptInfo>();

      for (const auto& asset : engine.assets().assetMap())
      {
        auto* const asset_info = asset.value();

        if (asset_info->typeInfo() == script_class_info)
        {
          const auto& path  = asset_info->filePathRel();
          char*       label = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "Add (%.*s) Script", int(path.length()), path.begin());

          if (ImGui::Selectable(label))
          {
            ScriptBehavior* const behav = entity.addBehavior<ScriptBehavior>();

            behav->setScriptPath(asset_info->filePathRel());
          }
        }
      }

      ImGui::EndCombo();
    }

    return false;
  }

  static int ImGuiStringCallback(ImGuiInputTextCallbackData* data)
  {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
      auto* str = static_cast<String*>(data->UserData);
      IM_ASSERT(data->Buf == str->cstr());
      str->resize(data->BufTextLen);
      data->Buf = const_cast<char*>(str->cstr());
    }

    return 0;
  }

  static int ImGuiStdStringCallback(ImGuiInputTextCallbackData* data)
  {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
      auto* str = static_cast<std::string*>(data->UserData);
      IM_ASSERT(data->Buf == str->c_str());
      str->resize(data->BufTextLen);
      data->Buf = const_cast<char*>(str->c_str());
    }

    return 0;
  }
}  // namespace bf::editor
