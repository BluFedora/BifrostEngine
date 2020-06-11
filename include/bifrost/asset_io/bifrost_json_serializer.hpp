/******************************************************************************/
/*!
* @file   bifrost_json_serializer.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-19
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_JSON_SERIALIZER_HPP
#define BIFROST_JSON_SERIALIZER_HPP

#include "bifrost/data_structures/bifrost_intrusive_list.hpp"  // List<T>
#include "bifrost/utility/bifrost_json.hpp"                    // Value
#include "bifrost_asset_info.hpp"                              // ISerializer

namespace bifrost
{
  class Assets;

  class JsonSerializerWriter final : public ISerializer
  {
   private:
    json::Value                            m_Document;
    List<json::Value*>                     m_ObjectStack;
    HashTable<IBaseObject*, std::uint32_t> m_ReferenceMap;
    std::uint32_t                          m_ReferenceID;

   public:
    explicit JsonSerializerWriter(IMemoryManager& memory);

    json::Value& document() { return m_Document; }

    void registerReference(IBaseObject& object) override;
    bool beginDocument(bool is_array) override;
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
    void serialize(StringRange key, String& value) override;
    void serialize(StringRange key, BifrostUUID& value) override;
    void serialize(StringRange key, BaseAssetHandle& value) override;
    void serialize(StringRange key, BaseRef& value) override;
    using ISerializer::serialize;
    void popObject() override;
    void popArray() override;
    void endDocument() override;

   private:
    json::Value& currentObject() { return *m_ObjectStack.back(); }
    json::Value& pushX(StringRange key);
  };

  class JsonSerializerReader final : public ISerializer
  {
    friend class RefPatcherSerializer;

   public:
    struct ObjectStackNode final
    {
      json::Value* object;
      int          array_index;

      ObjectStackNode(json::Value* obj, int index) :
        object{obj},
        array_index{index}
      {
      }
    };

   private:
    Assets&                                m_Assets;
    json::Value&                           m_Document;
    List<ObjectStackNode>                  m_ObjectStack;
    HashTable<std::uint32_t, IBaseObject*> m_ReferenceMap;

   public:
    explicit JsonSerializerReader(Assets& assets, IMemoryManager& memory, json::Value& document);

    void registerReference(IBaseObject& object) override;
    bool beginDocument(bool is_array) override;
    bool hasKey(StringRange key) override;
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
    void serialize(StringRange key, String& value) override;
    void serialize(StringRange key, BifrostUUID& value) override;
    void serialize(StringRange key, BaseAssetHandle& value) override;
    void serialize(StringRange key, BaseRef& value) override;
    using ISerializer::serialize;
    void popObject() override;
    void popArray() override;
    void endDocument() override;

   private:
    ObjectStackNode& currentNode() { return m_ObjectStack.back(); }
    json::Value&     currentObject() { return *currentNode().object; }
  };

  class RefPatcherSerializer final : public ISerializer
  {
   private:
    JsonSerializerReader& m_JsonReader;

   public:
    explicit RefPatcherSerializer(JsonSerializerReader& reader) :
      ISerializer(SerializerMode::SAVING),
      m_JsonReader{reader}
    {
    }

    bool beginDocument(bool is_array) override { return m_JsonReader.beginDocument(is_array); }
    bool pushObject(StringRange key) override { return m_JsonReader.pushObject(key); }
    bool pushArray(StringRange key, std::size_t& size) override { return m_JsonReader.pushArray(key, size); }
    void serialize(StringRange key, bool& value) override { }
    void serialize(StringRange key, std::int8_t& value) override {}
    void serialize(StringRange key, std::uint8_t& value) override {}
    void serialize(StringRange key, std::int16_t& value) override {}
    void serialize(StringRange key, std::uint16_t& value) override {}
    void serialize(StringRange key, std::int32_t& value) override {}
    void serialize(StringRange key, std::uint32_t& value) override {}
    void serialize(StringRange key, std::int64_t& value) override {}
    void serialize(StringRange key, std::uint64_t& value) override {}
    void serialize(StringRange key, float& value) override {}
    void serialize(StringRange key, double& value) override {}
    void serialize(StringRange key, long double& value) override {}
    void serialize(StringRange key, String& value) override {}
    void serialize(StringRange key, BifrostUUID& value) override {}
    void serialize(StringRange key, BaseAssetHandle& value) override {}
    void serialize(StringRange key, BaseRef& value) override;
    void popObject() override { m_JsonReader.popObject(); }
    void popArray() override { m_JsonReader.popArray(); }
    void endDocument() override { m_JsonReader.endDocument(); }
  };
}  // namespace bifrost

#endif /* BIFROST_JSON_SERIALIZER_HPP */
