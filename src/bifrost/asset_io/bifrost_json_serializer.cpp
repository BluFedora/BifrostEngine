/******************************************************************************/
/*!
* @file   bifrost_json_serializer.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-19
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bifrost/asset_io/bifrost_json_serializer.hpp"

#include "bifrost/asset_io/bifrost_assets.hpp"  // Assets

namespace bifrost
{
  static constexpr StringRange k_SerializeObjectRefID = "__ObjectRefID__";
  static constexpr auto        k_InvalidRefID         = std::numeric_limits<std::uint32_t>::max();

  JsonSerializerWriter::JsonSerializerWriter(IMemoryManager& memory) :
    ISerializer(SerializerMode::SAVING),
    m_Document{},
    m_ObjectStack{memory},
    m_ReferenceMap{},
    m_ReferenceID{0u}
  {
  }

  void JsonSerializerWriter::registerReference(IBaseObject& object)
  {
    std::uint32_t ref_id;

    if (!m_ReferenceMap.has(&object))
    {
      ref_id = m_ReferenceID++;

      m_ReferenceMap.emplace(&object, ref_id);
    }
    else
    {
      ref_id = *m_ReferenceMap.at(&object);
    }

    serialize(k_SerializeObjectRefID, ref_id);
  }

  bool JsonSerializerWriter::beginDocument(bool is_array)
  {
    if (is_array)
    {
      m_Document = json::detail::ArrayInitializer{};
    }
    else
    {
      m_Document = json::detail::ObjectInitializer{};
    }

    m_ObjectStack.emplaceBack(&m_Document);

    return true;
  }

  bool JsonSerializerWriter::pushObject(StringRange key)
  {
    pushX(key) = json::detail::ObjectInitializer{};

    return true;
  }

  bool JsonSerializerWriter::pushArray(StringRange key, std::size_t& size)
  {
    pushX(key) = json::detail::ArrayInitializer{};

    size = 0;

    return true;
  }

  void JsonSerializerWriter::serialize(StringRange key, bool& value)
  {
    currentObject().add(key, json::Boolean(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::int8_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::uint8_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::int16_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::uint16_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::int32_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::uint32_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::int64_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, std::uint64_t& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, float& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, double& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, long double& value)
  {
    currentObject().add(key, json::Number(value));
  }

  void JsonSerializerWriter::serialize(StringRange key, String& value)
  {
    currentObject().add(key, value);
  }

  void JsonSerializerWriter::serialize(StringRange key, BifrostUUID& value)
  {
    currentObject().add(key, String(value.as_string));
  }

  void JsonSerializerWriter::serialize(StringRange key, BaseAssetHandle& value)
  {
    if (value)
    {
      pushObject(key);
      serialize("uuid", const_cast<BifrostUUID&>(value.info()->uuid()));
      popObject();
    }
    else
    {
      currentObject().add(key, json::Value{});
    }
  }

  void JsonSerializerWriter::serialize(StringRange key, BaseRef& value)
  {
    if (pushObject(key))
    {
      std::uint32_t id = k_InvalidRefID;

      if (value.object())
      {
        registerReference(*value.object());

        id = m_ReferenceMap[value.object()];
      }

      serialize(k_SerializeObjectRefID, id);

      popObject();
    }
  }

  void JsonSerializerWriter::popObject()
  {
    m_ObjectStack.popBack();
  }

  void JsonSerializerWriter::popArray()
  {
    m_ObjectStack.popBack();
  }

  void JsonSerializerWriter::endDocument()
  {
    m_ObjectStack.popBack();
  }

  json::Value& JsonSerializerWriter::pushX(StringRange key)
  {
    auto& current_obj = currentObject();

    json::Value* const current = current_obj.isArray() ? &current_obj.push() : &current_obj[key];
    m_ObjectStack.emplaceBack(current);

    return *current;
  }

  JsonSerializerReader::JsonSerializerReader(Assets& assets, IMemoryManager& memory, json::Value& document) :
    ISerializer(SerializerMode::LOADING),
    m_Assets{assets},
    m_Document{document},
    m_ObjectStack{memory}
  {
  }

  void JsonSerializerReader::registerReference(IBaseObject& object)
  {
    std::uint32_t serialized_id = k_InvalidRefID;

    serialize(k_SerializeObjectRefID, serialized_id);

    if (serialized_id != k_InvalidRefID)
    {
      m_ReferenceMap[serialized_id] = &object;
    }
  }

  bool JsonSerializerReader::beginDocument(bool is_array)
  {
    const bool is_valid = m_Document.isArray() == is_array;

    if (is_valid)
    {
      m_ObjectStack.emplaceBack(&m_Document, is_array ? 0 : -1);
    }

    return is_valid;
  }

  bool JsonSerializerReader::hasKey(StringRange key)
  {
    auto& current_obj = currentObject();

    return current_obj.isObject() && current_obj.at(key) != nullptr;
  }

  bool JsonSerializerReader::pushObject(StringRange key)
  {
    json::Value& current_obj = currentObject();

    if (current_obj.isArray())
    {
      m_ObjectStack.emplaceBack(&current_obj[currentNode().array_index++], -1);
    }
    else if (current_obj.isObject())
    {
      m_ObjectStack.emplaceBack(&current_obj[key], -1);
    }
    else
    {
      return false;
    }

    return true;
  }

  bool JsonSerializerReader::pushArray(StringRange key, std::size_t& size)
  {
    json::Value& current_obj = currentObject();
    json::Value* object;

    if (current_obj.isArray())
    {
      object = &current_obj[currentNode().array_index++];
    }
    else if (current_obj.isObject())
    {
      object = &current_obj[key];
    }
    else
    {
      return false;
    }

    size = object->size();

    m_ObjectStack.emplaceBack(object, 0);
    return true;
  }

  void JsonSerializerReader::serialize(StringRange key, bool& value)
  {
    auto& object = currentNode();

    if (object.array_index > -1 && object.object->isArray())
    {
      json::Array& arr = object.object->as<json::Array>();

      if (object.array_index < int(arr.size()))
      {
        auto& json_number = arr[object.array_index];

        if (json_number.isBoolean())
        {
          value = json_number.as<json::Boolean>();
        }

        ++object.array_index;
      }
    }
    else if (object.object->isObject())
    {
      if (hasKey(key))
      {
        value = object.object->get(key, json::Boolean{});
      }
    }
  }

  template<typename T>
  static void readNumber(JsonSerializerReader::ObjectStackNode& object, StringRange key, T& value)
  {
    if (object.array_index > -1 && object.object->isArray())
    {
      json::Array& arr = object.object->as<json::Array>();

      if (object.array_index < int(arr.size()))
      {
        auto& json_number = arr[object.array_index];

        if (json_number.isNumber())
        {
          value = T(json_number.as<json::Number>());
        }

        ++object.array_index;
      }
    }
    else if (object.object->isObject())
    {
      if (object.object->at(key) != nullptr)
      {
        value = T(object.object->get(key, json::Number{}));
      }
    }
  }

  void JsonSerializerReader::serialize(StringRange key, std::int8_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, std::uint8_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, std::int16_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, std::uint16_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, std::int32_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, std::uint32_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, std::int64_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, std::uint64_t& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, float& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, double& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, long double& value)
  {
    readNumber(currentNode(), key, value);
  }

  void JsonSerializerReader::serialize(StringRange key, String& value)
  {
    ObjectStackNode& object = currentNode();

    if (object.array_index > -1)
    {
      if (object.object->isArray())
      {
        json::Array& arr = object.object->as<json::Array>();

        if (object.array_index < int(arr.size()))
        {
          auto& json_number = arr[object.array_index];

          if (json_number.isString())
          {
            value = json_number.as<json::String>();
          }

          ++object.array_index;
        }
      }
    }
    else if (object.object->isObject())
    {
      if (hasKey(key))
      {
        value = object.object->get(key, json::String{});
      }
    }
  }

  void JsonSerializerReader::serialize(StringRange key, BifrostUUID& value)
  {
    String str;
    serialize(key, str);

    if (!str.isEmpty())
    {
      value = bfUUID_fromString(str.cstr());
    }
  }

  void JsonSerializerReader::serialize(StringRange key, BaseAssetHandle& value)
  {
    if (pushObject(key))
    {
      BifrostUUID uuid;
      serialize("uuid", uuid);

      BaseAssetInfo* const info = m_Assets.findAssetInfo(uuid);

      if (info)
      {
        m_Assets.tryAssignHandle(value, info);
      }

      popObject();
    }
  }

  void JsonSerializerReader::serialize(StringRange key, BaseRef& value)
  {
    (void)key;
    (void)value;
  }

  void JsonSerializerReader::popObject()
  {
    m_ObjectStack.popBack();
  }

  void JsonSerializerReader::popArray()
  {
    popObject();
  }

  void JsonSerializerReader::endDocument()
  {
    m_ObjectStack.popBack();
  }

  void RefPatcherSerializer::serialize(StringRange key, BaseRef& value)
  {
    if (pushObject(key))
    {
      std::uint32_t id = k_InvalidRefID;
      m_JsonReader.serialize(k_SerializeObjectRefID, id);

      if (id != k_InvalidRefID)
      {
        IBaseObject** const obj = m_JsonReader.m_ReferenceMap.at(id);

        if (obj)
        {
          value.bind(*obj);
        }
      }

      popObject();
    }
  }
}  // namespace bifrost
