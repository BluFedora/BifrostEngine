#include "bifrost/asset_io/bifrost_json_value.hpp"

#include "bifrost/bifrost_std.h"

#include <cmath> /* isfinite */

namespace bifrost
{
  void JsonValue::toString(std::string& out, bool pretty_print, uint8_t tab_size) const
  {
    _toString(out, pretty_print, tab_size, 0);
  }

  static void addString(std::string& out, const std::string& source)
  {
    out += '"';

    // [https://en.wikipedia.org/wiki/Escape_sequences_in_C]
    for (auto c : source)
    {
      switch (c)
      {
        case '"': out += "\\\""; break;
        case '\'': out += "\\\'"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        case '\\': out += "\\\\"; break;
        default: out += c; break;
      }
    }

    out += '"';

    /*
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'v': return '\v';
    case '?': return '\?';
    */
  }

  void JsonValue::_toString(std::string& out, bool pretty_print, uint8_t tab_size, uint32_t indent) const
  {
    switch (type())
    {
      case invalid_type():
      {
        out += "null";
        break;
      }
      case type_of<string_t>():
      {
        addString(out, get<string_t>());
        break;
      }
      case type_of<array_t>():
      {
        out += pretty_print ? "[\n" : "[";

        const auto& as_array = get<array_t>();

        for (const auto& elem : as_array)
        {
          _addNSpaces(out, indent + tab_size);
          elem._toString(out, pretty_print, tab_size, indent + tab_size);

          if (&elem != &as_array.back())  // not last element
          {
            out += ",";

            if (pretty_print)
            {
              out += "\n";
            }
          }
        }

        if (pretty_print)
          out += "\n";
        _addNSpaces(out, indent);
        out += "]";
        break;
      }
      case type_of<object_t>():
      {
        out += pretty_print ? "{\n" : "{";

        for (auto it = get<object_t>().begin(); it != get<object_t>().end(); ++it)
        {
          _addNSpaces(out, indent + tab_size);

          addString(out, it->key());
          out += " : ";
          it->value()._toString(out, pretty_print, tab_size, indent + tab_size);

          if (it.next() != get<object_t>().end())  // not last element
          {
            out += ",";

            if (pretty_print)
            {
              out += "\n";
            }
          }
        }

        if (pretty_print)
          out += "\n";
        _addNSpaces(out, indent);
        out += "}";
        break;
      }
      case type_of<number_t>():
      {
        const auto value = get<number_t>();

        if (std::isfinite(value))
        {
          out += std::to_string(value);
        }
        else /* NOTE(Shareef): Maybe this should be reported cuz this is hella bad. */
        {
          out += "0.0";
        }
        break;
      }
      case type_of<boolean_t>():
      {
        out += get<boolean_t>() ? "true" : "false";
        break;
      }
        bfInvalidDefaultCase();
    }
  }

  void JsonValue::_addNSpaces(std::string& out, uint32_t indent)
  {
    for (uint32_t i = 0; i < indent; ++i)
    {
      out += " ";
    }
  }

  JsonValue::JsonValue(std::initializer_list<std::pair<const string_t, JsonValue>>&& values)
  {
    set<object_t>(std::forward<decltype(values)>(values));
  }

  JsonValue::JsonValue(std::initializer_list<JsonValue>&& values)
  {
    set<array_t>(std::forward<decltype(values)>(values));
  }

  JsonValue::JsonValue(array_t&& arr)
  {
    set<array_t>(arr);
  }

  JsonValue::JsonValue(number_t number)
  {
    set<number_t>(number);
  }

  JsonValue::JsonValue(const string_t& value)
  {
    set<string_t>(value);
  }

  JsonValue& JsonValue::operator=(number_t rhs)
  {
    set<number_t>(rhs);
    return *this;
  }

  JsonValue& JsonValue::operator=(unsigned long rhs)
  {
    set<number_t>(rhs);
    return *this;
  }

  JsonValue& JsonValue::operator=(int rhs)
  {
    set<number_t>(rhs);
    return *this;
  }

  JsonValue& JsonValue::operator=(const string_t& rhs)
  {
    set<string_t>(rhs);
    return *this;
  }

  JsonValue& JsonValue::operator=(const object_t& rhs)
  {
    set<object_t>(rhs);
    return *this;
  }

  JsonValue& JsonValue::operator=(const array_t& rhs)
  {
    set<array_t>(rhs);
    return *this;
  }

  JsonValue& JsonValue::operator=(boolean_t rhs)
  {
    set<boolean_t>(rhs);
    return *this;
  }

  JsonValue& JsonValue::operator[](const string_t& key)
  {
    return _castAndGet<object_t>()[key];
  }

  JsonValue& JsonValue::operator[](const char* key)
  {
    return _castAndGet<object_t>()[key];
  }

  JsonValue& JsonValue::at(const string_t& key)
  {
    return *_castAndGet<object_t>().at(key);
  }

  JsonValue* JsonValue::at(const string_t& key) const
  {
    if (isObject())
    {
      return as<object_t>().at(key);
    }

    return nullptr;
  }

  JsonValue& JsonValue::operator[](const array_t::size_type index)
  {
    return _castAndGet<array_t>()[index];
  }

  void JsonValue::push_back(const JsonValue& value)
  {
    _castAndGet<array_t>().push_back(value);
  }

  JsonValue& JsonValue::at(const array_t::size_type index)
  {
    return _castAndGet<array_t>().at(index);
  }

  std::size_t JsonValue::size() const
  {
    if (isArray())
    {
      return _castAndGet<array_t>().size();
    }

    return 0;
  }

  bool JsonValue::isString() const { return is<string_t>(); }
  bool JsonValue::isNumber() const { return is<number_t>(); }
  bool JsonValue::isArray() const { return is<array_t>(); }
  bool JsonValue::isObject() const { return is<object_t>(); }
  bool JsonValue::isBoolean() const { return is<boolean_t>(); }
}  // namespace bifrost
