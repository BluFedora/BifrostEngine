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
#include "bifrost/editor/bifrost_editor_serializer.hpp"

#include "bifrost/asset_io/bifrost_assets.hpp"
#include "bifrost/math/bifrost_vec2.h"
#include "bifrost/math/bifrost_vec3.h"
#include "bifrost/memory/bifrost_linear_allocator.hpp"

namespace bifrost::editor
{
  static constexpr int   s_MaxDigitsUInt64 = 20;
  static constexpr float s_DragSpeed       = 0.05f;

  ImGuiSerializer::ImGuiSerializer(IMemoryManager& memory) :
    ISerializer(SerializerMode::INSPECTING),
    m_IsOpenStack{memory},
    m_HasChangedStack{memory},
    m_NameBuffer{'\0'},
    m_Assets{nullptr}
  {
  }

  bool ImGuiSerializer::beginDocument(bool is_array)
  {
    auto& obj    = m_IsOpenStack.emplace();
    obj.is_array = is_array;

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
    }

    size = 0;
    return is_open;
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
    hasChangedTop() |= ImGui::DragScalar(m_NameBuffer, ImGuiDataType_Double, &value_d, s_DragSpeed);
    value = value_d;
  }

  void ImGuiSerializer::serialize(StringRange key, Vec2f& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalarN(m_NameBuffer, ImGuiDataType_Float, &value.x, 2, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, Vec3f& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::DragScalarN(m_NameBuffer, ImGuiDataType_Float, &value.x, 3, s_DragSpeed);
  }

  void ImGuiSerializer::serialize(StringRange key, String& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= imgui_ext::inspect(m_NameBuffer, value);
  }

  void ImGuiSerializer::serialize(StringRange key, BifrostUUID& value)
  {
    setNameBuffer(key);
    hasChangedTop() |= ImGui::InputText(m_NameBuffer, value.as_string, sizeof(value.as_string), ImGuiInputTextFlags_ReadOnly);
  }

  void ImGuiSerializer::serialize(StringRange key, BaseAssetHandle& value)
  {
    setNameBuffer(key);

    if (value)
    {
      if (ImGui::Button("clear"))
      {
        value.release();
        hasChangedTop() |= true;
      }

      ImGui::SameLine();
    }

    const char* const preview_name  = value ? value.info()->path().c_str() : "<null>";
    BaseAssetInfo*    assigned_info = nullptr;

    if (ImGui::BeginCombo(m_NameBuffer, preview_name, ImGuiComboFlags_HeightLargest))
    {
      for (const auto& asset_info_it : m_Assets->assetMap())
      {
        BaseAssetInfo* const asset_info = asset_info_it.value();

        if (Assets::isHandleCompatible(value, asset_info))
        {
          const bool is_selected = asset_info == value.info();

          if (ImGui::Selectable(asset_info->path().cstr(), is_selected, ImGuiSelectableFlags_None))
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
        const BifrostUUID* data = static_cast<const BifrostUUID*>(payload->Data);

        assert(payload->DataSize == sizeof(BifrostUUID));

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
  }

  void ImGuiSerializer::serialize(StringRange key, std::uint64_t& enum_value, meta::BaseClassMetaInfo* type_info)
  {
    setNameBuffer(key);

    const std::uint64_t mask           = type_info->enumValueMask();
    const std::uint64_t original_value = enum_value & mask;
    std::uint64_t       new_value      = original_value;
    const char*         preview_name   = nullptr;

    for (const auto& props : type_info->properties())
    {
      if ((props->get(&enum_value).as<std::uint64_t>() & mask) == original_value)
      {
        preview_name = props->name().data();
        break;
      }
    }

    if (ImGui::BeginCombo(m_NameBuffer, preview_name, ImGuiComboFlags_None))
    {
      for (const auto& props : type_info->properties())
      {
        const std::uint64_t value       = props->get(&enum_value).as<std::uint64_t>() & mask;
        const bool          is_selected = value == original_value;

        if (ImGui::Selectable(props->name().data(), is_selected, ImGuiSelectableFlags_None))
        {
          if (!is_selected)
          {
            new_value = value;
          }
        }
      }

      ImGui::EndCombo();
    }

    const bool has_changed = original_value != new_value;

    hasChangedTop() |= has_changed;

    if (has_changed)
    {
      type_info->enumValueWrite(enum_value, new_value);
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
    return result;
  }

  void ImGuiSerializer::setNameBuffer(StringRange key)
  {
    auto& obj = top();

    if (obj.is_array)
    {
      string_utils::fmtBuffer(m_NameBuffer, sizeof(m_NameBuffer), nullptr, "%i", obj.array_index);
      ++obj.array_index;
    }
    else
    {
      string_utils::fmtBuffer(m_NameBuffer, sizeof(m_NameBuffer), nullptr, "%.*s", int(key.length()), key.begin());
    }
  }

  static int ImGuiStringCallback(ImGuiInputTextCallbackData* data);
  static int ImGuiStdStringCallback(ImGuiInputTextCallbackData* data);

  bool imgui_ext::inspect(const char* label, String& string, ImGuiInputTextFlags flags)
  {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label, const_cast<char*>(string.cstr()), string.capacity(), flags, &ImGuiStringCallback, static_cast<void*>(&string));
  }

  bool imgui_ext::inspect(const char* label, std::string& string, ImGuiInputTextFlags flags)
  {
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label, const_cast<char*>(string.c_str()), string.capacity(), flags, &ImGuiStdStringCallback, static_cast<void*>(&string));
  }

  template<typename T>
  static bool InspectInt(const char* label, Any& object, bool* did_change = nullptr)
  {
    if (object.is<T>())
    {
      int value = int(object.as<T>());

      if (ImGui::DragInt(label, &value))
      {
        if constexpr (std::is_unsigned_v<T>)
        {
          value = value < 0 ? 0 : value;
        }

        object.assign<T>(value);

        if (did_change)
        {
          *did_change = true;
        }
      }
      else
      {
        if (did_change)
        {
          *did_change = false;
        }
      }

      return true;
    }

    return false;
  }

  template<typename T>
  static bool InspectFloat(const char* label, Any& object, bool* did_change = nullptr)
  {
    if (object.is<T>())
    {
      float value = float(object.as<T>());

      if (ImGui::DragFloat(label, &value))
      {
        object.assign<T>(value);

        if (did_change)
        {
          *did_change = true;
        }
      }
      else
      {
        if (did_change)
        {
          *did_change = false;
        }
      }

      return true;
    }

    if (object.is<T*>())
    {
      float value = float(*object.as<T*>());

      if (ImGui::DragFloat(label, &value))
      {
        *object.as<T*>() = T(value);

        if (did_change)
        {
          *did_change = true;
        }
      }
      else
      {
        if (did_change)
        {
          *did_change = false;
        }
      }

      return true;
    }

    return false;
  }

  bool imgui_ext::inspect(const char* label, Any& object, meta::BaseClassMetaInfo* info)
  {
    bool did_change = false;

    if (object.isEmpty())
    {
      ImGui::Text("%s : <empty>", label);
    }
    else if (InspectInt<char>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::int8_t>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::uint8_t>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::int16_t>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::uint16_t>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::int32_t>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::uint32_t>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::int64_t>(label, object, &did_change))
    {
    }
    else if (InspectInt<std::uint64_t>(label, object, &did_change))
    {
    }
    else if (InspectFloat<float>(label, object, &did_change))
    {
    }
    else if (InspectFloat<double>(label, object, &did_change))
    {
    }
    else if (InspectFloat<long double>(label, object, &did_change))
    {
    }
    else if (object.is<Vec3f>())
    {
      did_change = ImGui::DragFloat3(label, &object.as<Vec3f>().x);
    }
    else if (object.is<Vec3f*>())
    {
      did_change = ImGui::DragFloat3(label, &object.as<Vec3f*>()->x);
    }
    else if (object.is<std::string>())
    {
      did_change = inspect(label, object.as<std::string>());
    }
    else if (object.is<std::string*>())
    {
      did_change = inspect(label, *object.as<std::string*>());
    }
    else if (object.is<void*>())
    {
      ImGui::Text("%s : <%p>", label, object.as<void*>());
    }
    else if (object.is<bool>())
    {
      ImGui::Checkbox(label, &object.as<bool>());
    }
    else if (object.is<BaseAssetHandle>())
    {
    }
    else if (object.is<std::byte>())
    {
      ImGui::Text("%s : <byte>", label);
    }
    else if (info)
    {
      if (object.as<void*>() != nullptr)
      {
        if (ImGui::TreeNode(label))
        {
          for (auto& prop : info->properties())
          {
            auto field_value = prop->get(object);

            if (inspect(prop->name().data(), field_value, prop->type()))
            {
              prop->set(object, field_value);
              did_change = true;
            }
          }

          if (info->isArray())
          {
            const std::size_t size = info->arraySize(object);

            for (std::size_t i = 0; i < size; ++i)
            {
              char            label_buffer[s_MaxDigitsUInt64 + 1];
              LinearAllocator label_alloc{label_buffer, sizeof(label_buffer)};

              auto* const idx_label = string_utils::fmtAlloc(label_alloc, nullptr, "%i", int(i));
              auto        element   = info->arrayGetElementAt(object, i);

              if (inspect(idx_label, element, info->containedType()))
              {
                info->arraySetElementAt(object, i, element);
              }
            }
          }

          ImGui::TreePop();
        }
      }
      else
      {
        if (ImGui::Button("Create (DOES NOT WORK)"))
        {
          // object     = info->instantiate(g_Engine->mainMemory());
          did_change = true;
        }
      }
    }
    else
    {
      ImGui::Text("%s : <unknown>", label);
    }

    return did_change;
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
}  // namespace bifrost::editor
