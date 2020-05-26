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
#include "bifrost_asset_handle.hpp"                            // ISerializer

namespace bifrost
{
  class Assets;

  class JsonSerializerWriter final : public ISerializer
  {
   private:
    json::Value        m_Document;
    List<json::Value*> m_ObjectStack;

   public:
    explicit JsonSerializerWriter(IMemoryManager& memory);

    json::Value& document() { return m_Document; }

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
    void serialize(StringRange key, std::uint64_t& enum_value, meta::BaseClassMetaInfo* type_info) override;
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
    Assets&               m_Assets;
    json::Value&          m_Document;
    List<ObjectStackNode> m_ObjectStack;

   public:
    explicit JsonSerializerReader(Assets& assets, IMemoryManager& memory, json::Value& document);

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
    void serialize(StringRange key, std::uint64_t& enum_value, meta::BaseClassMetaInfo* type_info) override;
    using ISerializer::serialize;
    void popObject() override;
    void popArray() override;
    void endDocument() override;

   private:
    ObjectStackNode& currentNode() { return m_ObjectStack.back(); }
    json::Value&     currentObject() { return *currentNode().object; }
  };
}  // namespace bifrost

#endif /* BIFROST_JSON_SERIALIZER_HPP */
