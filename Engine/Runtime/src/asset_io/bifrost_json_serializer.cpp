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
#include "bf/asset_io/bifrost_json_serializer.hpp"

#include "bf/asset_io/bf_base_asset.hpp"
#include "bf/asset_io/bf_document.hpp"     // BaseDocument
#include "bf/asset_io/bifrost_assets.hpp"  // Assets
#include "bf/ecs/bf_entity.hpp"       // Entity

namespace bf
{
  JsonSerializerWriter::JsonSerializerWriter(IMemoryManager& memory) :
    ISerializer(SerializerMode::SAVING),
    m_Document{},
    m_ObjectStack{memory}
  {
  }

  bool JsonSerializerWriter::beginDocument()
  {
    m_Document = json::detail::ObjectInitializer{};
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

  void JsonSerializerWriter::serialize(StringRange key, IARCHandle& value)
  {
    if (value.isValid())
    {
      IBaseAsset* const asset_handle = value.handle();
      
      // We can only serialize references to assets that are backed by file.
      if (asset_handle->hasDocument())
      {
        ResourceReference resource_ref = asset_handle->toRef();

        pushObject(key);
        serialize("docID", resource_ref.doc_id);
        serialize("fileID", resource_ref.file_id.id);
        popObject();
      }
    }
    else
    {
      currentObject().add(key, json::Value{});
    }
  }

  void JsonSerializerWriter::serialize(StringRange key, EntityRef& value)
  {
    if (pushObject(key))
    {
      serialize("uuid", value.m_ID);
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

  bool JsonSerializerReader::beginDocument()
  {
    m_ObjectStack.emplaceBack(&m_Document, -1);
    return true;
  }

  bool JsonSerializerReader::hasKey(StringRange key)
  {
    const auto& current_obj = currentObject();

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
      const json::Array& arr = object.object->as<json::Array>();

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
      json::Value* const j_value = object.object->at(key);

      if (j_value && j_value->isNumber())
      {
        value = T(j_value->as<json::Number>());
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
        const json::Array& arr = object.object->as<json::Array>();

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

  void JsonSerializerReader::serialize(StringRange key, IARCHandle& value)
  {
    if (pushObject(key))
    {
      bfUUIDNumber doc_id;
      ResourceID   file_id;

      serialize("docID", doc_id);
      serialize("fileID", file_id.id);

      IBaseAsset* found_asset = m_Assets.findAsset(ResourceReference{doc_id, file_id});

      // If the type info of the found asset is not compatible with the ARC handle then just assign nullptr.
      if (found_asset && value.typeInfo() != found_asset->type())
      {
        found_asset = nullptr;
      }

      value.assign(found_asset);

      popObject();
    }
  }

  void JsonSerializerReader::serialize(StringRange key, EntityRef& value)
  {
    if (pushObject(key))
    {
      serialize("uuid", value.m_ID);
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
}  // namespace bf
