/******************************************************************************/
/*!
* @file   bf_json.cpp
* @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
* @brief
*   C++ Wrapper around the C Json API with conversions to/from a C++ Value.
*
* @version 0.0.1
* @date    2020-03-17
*
* @copyright Copyright (c) 2020-2021
*/
/******************************************************************************/
#include "bf/utility/bifrost_json.hpp"

#include "bf/utility/bifrost_json.h" /* Json API */

namespace bf::json
{
  // TODO(SR): This should probably be runtime configuarble but I like my pretty print so...
  static constexpr bool k_DoPrettyPrint = true;

  static StringRange fromJsonString(const bfJsonString& value)
  {
    return StringRange(value.string, value.length);
  }

  static bfJsonString toJsonString(const StringRange& value)
  {
    return bfJsonString{value.begin(), value.length()};
  }

  Value parse(char* source, std::size_t source_length)
  {
    struct Context final
    {
      Value             document;
      bf::Array<Value*> value_stack;
      StringRange       key;

      Context() :
        document{},
        value_stack{*g_DefaultAllocator.temporary}
      {
      }
    };

    LinearAllocatorScope mem_scope = *g_DefaultAllocator.temporary;
    Context              ctx       = {};

    bfJsonParser_fromString(
     source, source_length, [](bfJsonParserContext* ctx, bfJsonEvent event, void* user_data) {
       Context* const user_context = static_cast<Context*>(user_data);

       switch (event)
       {
         case BF_JSON_EVENT_BEGIN_DOCUMENT:
         {
           user_context->value_stack.push(&user_context->document);
           break;
         }
         case BF_JSON_EVENT_BEGIN_ARRAY:
         case BF_JSON_EVENT_BEGIN_OBJECT:
         {
           const StringRange key           = fromJsonString(bfJsonParser_key(ctx));
           Value* const      current_value = &user_context->value_stack.back()->add(key);

           if (event == BF_JSON_EVENT_BEGIN_ARRAY)
             current_value->set<Array>();
           else
             current_value->set<Object>();

           user_context->value_stack.push(current_value);
           break;
         }
         case BF_JSON_EVENT_END_ARRAY:
         case BF_JSON_EVENT_END_OBJECT:
         case BF_JSON_EVENT_END_DOCUMENT:
         {
           user_context->value_stack.pop();
           break;
         }
         case BF_JSON_EVENT_KEY_VALUE:
         {
           const StringRange key        = fromJsonString(bfJsonParser_key(ctx));
           Value&            value_data = *user_context->value_stack.back();

           switch (bfJsonParser_valueType(ctx))
           {
             case BF_JSON_VALUE_STRING:
             {
               value_data.add(key, String(fromJsonString(bfJsonParser_valAsString(ctx))));
               break;
             }
             case BF_JSON_VALUE_NUMBER:
             {
               value_data.add(key, bfJsonParser_valAsNumber(ctx));
               break;
             }
             case BF_JSON_VALUE_BOOLEAN:
             {
               value_data.add(key, bfJsonParser_valAsBoolean(ctx) == true);
               break;
             }
             case BF_JSON_VALUE_NULL:
             {
               value_data.add(key, Value());

               break;
             }
               bfInvalidDefaultCase();
           }

           break;
         }
         case BF_JSON_EVENT_PARSE_ERROR:
         {
          // TODO(SR): Better error logging.
#if _MSC_VER
           __debugbreak();
#else
           throw "Invalid json";
#endif
           break;
         }
           bfInvalidDefaultCase();
       }
     },
     &ctx);

    return ctx.document;
  }

  static void toStringRecNewline(bfJsonWriter* json_writer)
  {
    if (k_DoPrettyPrint)
    {
      bfJsonWriter_write(json_writer, "\n", 1);
    }
  }

  static void toStringRecIndent(bfJsonWriter* json_writer, int indent_level)
  {
    if (k_DoPrettyPrint)
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
    // TODO(SR): This should probably use the global temp allocator.

    bfJsonWriter* json_writer = bfJsonWriter_newCRTAlloc();

    toStringRec(json_writer, json, 0);

    out.clear();
    out.reserve(bfJsonWriter_length(json_writer));

    bfJsonWriter_forEachBlock(
     json_writer, [](const bfJsonStringBlock* block, void* ud) {
       String* const     out_buffer = static_cast<String*>(ud);
       const StringRange block_str  = fromJsonString(bfJsonStringBlock_string(block));

       out_buffer->append(block_str);
     },
     &out);

    bfJsonWriter_deleteCRT(json_writer);
  }

  Value::Value(detail::ObjectInitializer&& values) :
    Base_t()
  {
    set<Object>(std::forward<detail::ObjectInitializer>(values));
  }

  Value::Value(detail::ArrayInitializer&& values) :
    Base_t()
  {
    set<Array>(std::forward<detail::ArrayInitializer>(values));
  }

  Value::Value(const char* value) :
    Base_t(String(value))
  {
    set<String>(value);
  }

  Value::Value(int value) :
    Base_t(Number(value))
  {
  }

  Value::Value(std::uint64_t value) :
    Base_t(Number(value))
  {
  }

  Value::Value(std::int64_t value) :
    Base_t(Number(value))
  {
  }

  Value& Value::operator=(detail::ObjectInitializer&& values)
  {
    set<Object>(std::forward<detail::ObjectInitializer>(values));
    return *this;
  }

  Value& Value::operator=(detail::ArrayInitializer&& values)
  {
    set<Array>(std::forward<detail::ArrayInitializer>(values));
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
    return cast<Array>()[index];
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
    cast<Array>().push(item);
  }

  Value& Value::push()
  {
    return cast<Array>().emplace(nullptr);
  }

  void Value::insert(std::size_t index, const Value& item)
  {
    cast<Array>().insert(index, item);
  }

  Value& Value::back()
  {
    return cast<Array>().back();
  }

  void Value::pop()
  {
    cast<Array>().pop();
  }

  void Value::add(StringRange key, const Value& value)
  {
    add(key) = value;
  }

  Value& Value::add(StringRange key)
  {
    if (isObject())
    {
      return (*this)[key];
    }
    else if (isArray())
    {
      return as<Array>().emplace();
    }
    else
    {
      return *this;
    }
  }
}  // namespace bf::json

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
