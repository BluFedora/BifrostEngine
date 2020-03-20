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
  JsonSerializerWriter::JsonSerializerWriter(IMemoryManager& memory) :
    ISerializer(SerializerMode::SAVING),
    m_Document{},
    m_ObjectStack{memory}
  {
    m_ObjectStack.emplaceBack(&m_Document);
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

    return true;
  }

  bool JsonSerializerWriter::pushObject(StringRange key)
  {
    json::Value* const current = &currentObject()[key];

    *current = json::detail::ObjectInitializer{};
    m_ObjectStack.emplaceBack(current);

    return true;
  }

  bool JsonSerializerWriter::pushArray(StringRange key)
  {
    json::Value* const current = &currentObject()[key];

    *current = json::detail::ArrayInitializer{};
    m_ObjectStack.emplaceBack(current);

    return true;
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

  JsonSerializerReader::JsonSerializerReader(Assets& assets, IMemoryManager& memory, json::Value& document) :
    ISerializer(SerializerMode::LOADING),
    m_Assets{assets},
    m_Document{document},
    m_ObjectStack{memory}
  {
  }

  bool JsonSerializerReader::beginDocument(bool is_array)
  {
    m_ObjectStack.emplaceBack(&m_Document, is_array ? 0 : -1);
    return true;
  }

  bool JsonSerializerReader::pushObject(StringRange key)
  {
    m_ObjectStack.emplaceBack(&currentObject()[key], -1);
    return true;
  }

  bool JsonSerializerReader::pushArray(StringRange key)
  {
    m_ObjectStack.emplaceBack(&currentObject()[key], 0);
    return true;
  }

  template<typename T>
  static void readNumber(JsonSerializerReader::ObjectStackNode& object, StringRange key, T& value)
  {
    if (object.array_index > -1)
    {
      if (object.object->isArray())
      {
        json::Array& arr = object.object->as<json::Array>();

        if (object.array_index < arr.size())
        {
          auto& json_number = arr[object.array_index];

          if (json_number.isNumber())
          {
            value = T(json_number.as<json::Number>());
          }

          ++object.array_index;
        }
      }
    }
    else if (object.object->isObject())
    {
      value = T(object.object->get(key, json::Number{}));
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

        if (object.array_index < arr.size())
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
      value = object.object->get(key, json::String{});
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
}  // namespace bifrost
