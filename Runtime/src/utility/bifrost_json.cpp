/******************************************************************************/
/*!
* @file   bifrost_json.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   C++ Wrapper around the C Json API with conversions to/from a C++ Value.
*
* @version 0.0.1
* @date    2020-03-17
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bifrost/utility/bifrost_json.hpp"

#include "bf/CRTAllocator.hpp"            /* CRTAllocator */
#include "bifrost/utility/bifrost_json.h" /* Json API     */

namespace bf::json
{
  static CRTAllocator s_ArrayAllocator = {};  // TODO(SR): Think about this allocation scheme a bit more??

  static Value** readStorage(bfJsonParserContext* ctx)
  {
    return static_cast<Value**>(bfJsonParser_userStorage(ctx));
  }

  static Value* readStorageValue(bfJsonParserContext* ctx)
  {
    return *readStorage(ctx);
  }

  static Value* readParentStorageValue(bfJsonParserContext* ctx)
  {
    return *static_cast<Value**>(bfJsonParser_parentUserStorage(ctx));
  }

  static Value* makeChildItem(Value& parent, const StringRange& key)
  {
    if (parent.isObject())
    {
      return &parent[key];
    }

    if (parent.isArray())
    {
      parent.push(Value());
      return &parent.back();
    }

    return nullptr;
  }

  static void writeStorage(bfJsonParserContext* ctx, Value* value)
  {
    *readStorage(ctx) = value;
  }

  static StringRange fromJsonString(const bfJsonString& value)
  {
    return StringRange(value.string, value.length);
  }

  static bfJsonString toJsonString(const StringRange& value)
  {
    bfJsonString result;

    result.string = value.begin();
    result.length = value.length();

    return result;
  }

  Value fromString(char* source, std::size_t source_length)
  {
    struct Context final
    {
      Value       document = {};
      StringRange last_key = {};
    };

    Context ctx{};

    bfJsonParser_fromString(
     source, source_length, [](bfJsonParserContext* ctx, bfJsonEvent event, void* user_data) {
       Context* user_context = static_cast<Context*>(user_data);

       switch (event)
       {
         case BF_JSON_EVENT_BEGIN_DOCUMENT:
         {
           writeStorage(ctx, nullptr);
           break;
         }
         case BF_JSON_EVENT_END_DOCUMENT:
         {
           break;
         }
         case BF_JSON_EVENT_BEGIN_ARRAY:
         case BF_JSON_EVENT_BEGIN_OBJECT:
         {
           Value* const parent_value  = readParentStorageValue(ctx);
           Value* const current_value = parent_value == nullptr ? &user_context->document : makeChildItem(*parent_value, user_context->last_key);

           if (event == BF_JSON_EVENT_BEGIN_ARRAY)
           {
             current_value->set<Array>(s_ArrayAllocator);
           }
           else
           {
             current_value->set<Object>();
           }

           writeStorage(ctx, current_value);
           break;
         }
         case BF_JSON_EVENT_END_ARRAY:
         case BF_JSON_EVENT_END_OBJECT:
         {
           break;
         }
         case BF_JSON_EVENT_KEY:
         {
           user_context->last_key = fromJsonString(bfJsonParser_asString(ctx));
           break;
         }
         case BF_JSON_EVENT_VALUE:
         {
           Value* current_value = readStorageValue(ctx);

           if (!current_value)
           {
             current_value = &user_context->document;
           }

           Value& value_data = *current_value;

           switch (bfJsonParser_valueType(ctx))
           {
             case BF_JSON_VALUE_STRING:
             {
               value_data.add(user_context->last_key, String(fromJsonString(bfJsonParser_asString(ctx))));
               break;
             }
             case BF_JSON_VALUE_NUMBER:
             {
               value_data.add(user_context->last_key, bfJsonParser_asNumber(ctx));
               break;
             }
             case BF_JSON_VALUE_BOOLEAN:
             {
               value_data.add(user_context->last_key, bfJsonParser_asBoolean(ctx) != 0u);
               break;
             }
             case BF_JSON_VALUE_NULL:
             {
               value_data.add(user_context->last_key, Value());

               break;
             }
               bfInvalidDefaultCase();
           }

           break;
         }
         case BF_JSON_EVENT_PARSE_ERROR:
         {
#if _MSC_VER
           __debugbreak();
#endif
           break;
         }
           bfInvalidDefaultCase();
       }
     },
     &ctx);

    return ctx.document;
  }

  static bool s_PrettyPrint = true;

  static void toStringRecNewline(bfJsonWriter* json_writer)
  {
    if (s_PrettyPrint)
    {
      bfJsonWriter_write(json_writer, "\n", 1);
    }
  }

  static void toStringRecIndent(bfJsonWriter* json_writer, int indent_level)
  {
    if (s_PrettyPrint)
    {
      bfJsonWriter_indent(json_writer, indent_level * 4);
    }
  }

  static void toStringRec(bfJsonWriter* json_writer, const Value& value, int current_indent)
  {
    if (value.isObject())
    {
      bfJsonWriter_beginObject(json_writer);
      toStringRecNewline(json_writer);

      const Object& obj           = value.as<Object>();
      bool          is_first_item = true;

      for (const auto& it : obj)
      {
        if (!is_first_item)
        {
          bfJsonWriter_next(json_writer);
          toStringRecNewline(json_writer);
        }

        toStringRecIndent(json_writer, current_indent + 1);
        bfJsonWriter_key(json_writer, toJsonString(it.key()));
        toStringRec(json_writer, it.value(), current_indent + 1);
        is_first_item = false;
      }

      toStringRecNewline(json_writer);
      toStringRecIndent(json_writer, current_indent);
      bfJsonWriter_endObject(json_writer);
    }
    else if (value.isArray())
    {
      bfJsonWriter_beginArray(json_writer);
      toStringRecNewline(json_writer);

      const Array& arr           = value.as<Array>();
      bool         is_first_item = true;

      for (const auto& it : arr)
      {
        if (!is_first_item)
        {
          bfJsonWriter_next(json_writer);
          toStringRecNewline(json_writer);
        }

        toStringRecIndent(json_writer, current_indent + 1);
        toStringRec(json_writer, it, current_indent + 1);
        is_first_item = false;
      }

      toStringRecNewline(json_writer);
      toStringRecIndent(json_writer, current_indent);
      bfJsonWriter_endArray(json_writer);
    }
    else if (value.isString())
    {
      bfJsonWriter_valueString(json_writer, toJsonString(value.as<String>()));
    }
    else if (value.isNumber())
    {
      bfJsonWriter_valueNumber(json_writer, value.as<double>());
    }
    else if (value.isBoolean())
    {
      bfJsonWriter_valueBoolean(json_writer, value.as<bool>());
    }
    else
    {
      bfJsonWriter_valueNull(json_writer);
    }
  }

  void toString(const Value& json, String& out)
  {
    bfJsonWriter* json_writer = bfJsonWriter_new([](size_t size, void*) { return malloc(size); }, nullptr);

    toStringRec(json_writer, json, 0);

    out.reserve(bfJsonWriter_length(json_writer));
    out.clear();

    bfJsonWriter_forEachBlock(
     json_writer, [](const bfJsonStringBlock* block, void* ud) {
       String* const     out_buffer = static_cast<String*>(ud);
       const StringRange block_str  = fromJsonString(bfJsonStringBlock_string(block));
       out_buffer->append(block_str.begin(), block_str.length());
     },
     &out);

    bfJsonWriter_delete(json_writer, [](void* ptr, void*) { free(ptr); });
  }

  Value::Value(detail::ObjectInitializer&& values) :
    Base_t()
  {
    set<Object>(std::forward<detail::ObjectInitializer>(values));
  }

  Value::Value(detail::ArrayInitializer&& values) :
    Base_t()
  {
    set<Array>(s_ArrayAllocator, std::forward<detail::ArrayInitializer>(values));
  }

  Value::Value(const char* value) :
    Base_t()
  {
    set<String>(value);
  }

  Value::Value(int value) :
    Base_t()
  {
    set<Number>(Number(value));
  }

  Value::Value(std::uint64_t value) :
    Base_t()
  {
    set<Number>(Number(value));
  }

  Value::Value(std::int64_t value) :
    Base_t()
  {
    set<Number>(Number(value));
  }

  Value& Value::operator=(detail::ObjectInitializer&& values)
  {
    set<Object>(std::forward<detail::ObjectInitializer>(values));
    return *this;
  }

  Value& Value::operator=(detail::ArrayInitializer&& values)
  {
    set<Array>(s_ArrayAllocator, std::forward<detail::ArrayInitializer>(values));
    return *this;
  }

  Value& Value::operator[](const StringRange& key)
  {
    return cast<Object>()[key];
  }

  Value& Value::operator[](const char* key)
  {
    return cast<Object>()[key];
  }

  Value* Value::at(const StringRange& key) const
  {
    if (isObject())
    {
      return as<Object>().at(key);
    }

    return nullptr;
  }

  Value& Value::operator[](int index)
  {
    return cast<Array>(s_ArrayAllocator)[index];
  }

  std::size_t Value::size() const
  {
    if (isArray())
    {
      return as<Array>().size();
    }

    return 0u;
  }

  void Value::push(const Value& item)
  {
    cast<Array>(s_ArrayAllocator).push(item);
  }

  Value& Value::push()
  {
    return cast<Array>(s_ArrayAllocator).emplace(nullptr);
  }

  void Value::insert(std::size_t index, const Value& item)
  {
    cast<Array>(s_ArrayAllocator).insert(index, item);
  }

  Value& Value::back()
  {
    return cast<Array>(s_ArrayAllocator).back();
  }

  void Value::pop()
  {
    cast<Array>(s_ArrayAllocator).pop();
  }

  void Value::add(StringRange key, const Value& value)
  {
    if (isObject())
    {
      (*this)[key] = value;
    }
    else if (isArray())
    {
      push(value);
    }
    else
    {
      *this = value;
    }
  }
}  // namespace bf::json
