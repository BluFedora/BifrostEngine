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

#include "bifrost/memory/bifrost_c_allocator.hpp" /* CAllocator       */
#include "bifrost/utility/bifrost_json.h"         /* Bifrost Json API */

namespace bifrost::json
{
  static CAllocator s_ArrayAllocator = {};

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
         case BIFROST_JSON_EVENT_END_DOCUMENT:
         {
           Value* document_data = static_cast<Value*>(bfJsonParser_userStorage(ctx));

           user_context->document = *document_data;

           document_data->~Value();
           break;
         }
         case BIFROST_JSON_EVENT_BEGIN_DOCUMENT:
         case BIFROST_JSON_EVENT_BEGIN_ARRAY:
         case BIFROST_JSON_EVENT_BEGIN_OBJECT:
         {
           Value* value_data = new (bfJsonParser_userStorage(ctx)) Value();

           if (event == BIFROST_JSON_EVENT_BEGIN_ARRAY)
           {
             value_data->set<detail::Array_t>(s_ArrayAllocator);
           }
           else
           {
             value_data->set<detail::Object_t>();
           }
           break;
         }
         case BIFROST_JSON_EVENT_END_ARRAY:
         case BIFROST_JSON_EVENT_END_OBJECT:
         {
           Value* parent_value = static_cast<Value*>(bfJsonParser_parentUserStorage(ctx));
           Value* array_data   = static_cast<Value*>(bfJsonParser_userStorage(ctx));

           if (parent_value->isObject())
           {
             (*parent_value)[user_context->last_key] = *array_data;
           }
           else
           {
             parent_value->push(*array_data);
           }

           array_data->~Value();
           break;
         }
         case BIFROST_JSON_EVENT_KEY:
         {
           user_context->last_key = bfJsonParser_asString(ctx);
           break;
         }
         case BIFROST_JSON_EVENT_VALUE:
         {
           Value& value_data = *static_cast<Value*>(bfJsonParser_userStorage(ctx));

           switch (bfJsonParser_valueType(ctx))
           {
             case BIFROST_JSON_VALUE_STRING:
             {
               value_data.add(user_context->last_key, String(bfJsonParser_asString(ctx)));
               break;
             }
             case BIFROST_JSON_VALUE_NUMBER:
             {
               value_data.add(user_context->last_key, bfJsonParser_asNumber(ctx));
               break;
             }
             case BIFROST_JSON_VALUE_BOOLEAN:
             {
               value_data.add(user_context->last_key, bfJsonParser_asBoolean(ctx) != 0u);
               break;
             }
             case BIFROST_JSON_VALUE_NULL:
             {
               value_data.add(user_context->last_key, Value());

               break;
             }
               bfInvalidDefaultCase();
           }

           break;
         }
         case BIFROST_JSON_EVENT_PARSE_ERROR:
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

  static void toStringRecNewline(BifrostJsonWriter* json_writer)
  {
    if (s_PrettyPrint)
    {
      bfJsonWriter_write(json_writer, "\n", 1);
    }
  }

  static void toStringRecIndent(BifrostJsonWriter* json_writer, int indent_level)
  {
    if (s_PrettyPrint)
    {
      bfJsonWriter_indent(json_writer, indent_level * 4);
    }
  }

  static void toStringRec(BifrostJsonWriter* json_writer, const Value& value, int current_indent)
  {
    if (value.isObject())
    {
      bfJsonWriter_beginObject(json_writer);
      toStringRecNewline(json_writer);

      const detail::Object_t& obj           = value.as<detail::Object_t>();
      bool                    is_first_item = true;

      for (const auto& it : obj)
      {
        if (!is_first_item)
        {
          bfJsonWriter_next(json_writer);
          toStringRecNewline(json_writer);
        }

        toStringRecIndent(json_writer, current_indent + 1);
        bfJsonWriter_key(json_writer, it.key());
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

      const detail::Array_t& arr           = value.as<detail::Array_t>();
      bool                   is_first_item = true;

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
      bfJsonWriter_valueString(json_writer, value.as<String>());
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
    BifrostJsonWriter* json_writer = bfJsonWriter_new(&malloc);

    toStringRec(json_writer, json, 0);

    out.reserve(bfJsonWriter_length(json_writer));

    bfJsonWriter_forEachBlock(
     json_writer, [](const bfJsonStringBlock* block, void* ud) {
       String* const     out_buffer = static_cast<String*>(ud);
       const StringRange block_str  = bfJsonStringBlock_string(block);
       out_buffer->append(block_str.begin(), block_str.length());
     },
     &out);

    bfJsonWriter_delete(json_writer, &free);
  }

  Value::Value(detail::ObjectInitializer&& values) :
    Base_t()
  {
    set<detail::Object_t>(std::forward<detail::ObjectInitializer>(values));
  }

  Value::Value(detail::ArrayInitializer&& values) :
    Base_t()
  {
    set<detail::Array_t>(s_ArrayAllocator, std::forward<detail::ArrayInitializer>(values));
  }

  Value::Value(const char* value) :
    Base_t()
  {
    set<String>(value);
  }

  Value::Value(int value) :
    Base_t()
  {
    set<double>(double(value));
  }

  Value::Value(std::uint64_t value) :
    Base_t()
  {
    set<double>(double(value));
  }

  Value::Value(std::int64_t value) :
    Base_t()
  {
    set<double>(double(value));
  }

  Value& Value::operator=(detail::ObjectInitializer&& values)
  {
    set<detail::Object_t>(std::forward<detail::ObjectInitializer>(values));
    return *this;
  }

  Value& Value::operator=(detail::ArrayInitializer&& values)
  {
    set<detail::Array_t>(s_ArrayAllocator, std::forward<detail::ArrayInitializer>(values));
    return *this;
  }

  Value& Value::operator[](const StringRange& key)
  {
    return cast<detail::Object_t>()[key];
  }

  Value& Value::operator[](const char* key)
  {
    return cast<detail::Object_t>()[key];
  }

  Value* Value::at(const StringRange& key) const
  {
    if (isObject())
    {
      return as<detail::Object_t>().at(key);
    }

    return nullptr;
  }

  Value& Value::operator[](int index)
  {
    return cast<detail::Array_t>(s_ArrayAllocator)[index];
  }

  void Value::push(const Value& item)
  {
    cast<detail::Array_t>(s_ArrayAllocator).push(item);
  }

  void Value::insert(std::size_t index, const Value& item)
  {
    cast<detail::Array_t>(s_ArrayAllocator).insert(index, item);
  }

  Value& Value::back()
  {
    return cast<detail::Array_t>(s_ArrayAllocator).back();
  }

  void Value::pop()
  {
    cast<detail::Array_t>(s_ArrayAllocator).pop();
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
}  // namespace bifrost::json
