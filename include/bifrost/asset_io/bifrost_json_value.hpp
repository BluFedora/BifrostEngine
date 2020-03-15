#ifndef TIDE_JSON_VALUE_HPP
#define TIDE_JSON_VALUE_HPP

#include "bifrost/data_structures/bifrost_hash_table.hpp" /* HashTable */
#include "bifrost/data_structures/bifrost_string.hpp"     /* String    */
#include "bifrost/data_structures/bifrost_variant.hpp"    /* Variant   */
#include <string>                                         /* */
#include <vector>                                         /* vector    */

namespace bifrost
{
  class JsonValue;

  using string_t    = std::string;
  using number_t    = double;
  using array_t     = std::vector<JsonValue>;
  using object_t    = HashTable<string_t, JsonValue, 32>;
  using boolean_t   = bool;
  using JsonValue_t = Variant<string_t, array_t, object_t, number_t, boolean_t>;

  // const int i0 = sizeof(string_t);     // 40
  // const int i1 = sizeof(number_t);     // 8
  // const int i2 = sizeof(array_t);      // 32
  // const int i3 = sizeof(object_t);     // 16
  // const int i4 = sizeof(boolean_t);    // 1
  // const int i5 = sizeof(JsonValue_t);  // 48

  class JsonValue : public JsonValue_t
  {
   public:
    // Constructors
    JsonValue() = default;
    JsonValue(std::initializer_list<std::pair<const string_t, JsonValue>>&& values);
    JsonValue(std::initializer_list<JsonValue>&& values);
    JsonValue(array_t&& arr);
    JsonValue(number_t number);
    JsonValue(const string_t& value);

    // Assignment
    JsonValue& operator=(number_t rhs);
    JsonValue& operator=(unsigned long rhs);
    JsonValue& operator=(int rhs);
    JsonValue& operator=(const string_t& rhs);
    JsonValue& operator=(const object_t& rhs);
    JsonValue& operator=(const array_t& rhs);
    JsonValue& operator=(boolean_t rhs);

    // Object API
    JsonValue& operator[](const string_t& key);
    JsonValue& operator[](const char* key);
    JsonValue& at(const string_t& key);
    JsonValue* at(const string_t& key) const;

    template<typename T>
    T get_or(const string_t& key, const T& default_value)
    {
      if (isObject())
      {
        auto* const value = get<object_t>().get(key);

        if (value && value->is<T>())
        {
          return value->as<T>();
        }
      }

      return default_value;
    }

    template<typename T>
    T get_or(const string_t& key, const T& default_value) const
    {
      if (isObject())
      {
        const auto* const value = get<object_t>().get(key);

        if (value && value->is<T>())
        {
          return value->as<T>();
        }
      }

      return default_value;
    }

    // Array API
    JsonValue&  operator[](array_t::size_type index);
    void        push_back(const JsonValue& value);
    JsonValue&  at(array_t::size_type index);
    std::size_t size() const;

    // Meta
    using JsonValue_t::as;
    using JsonValue_t::is;
    using JsonValue_t::type;

    template<typename T>
    const T& as_or(const T& default_value) const
    {
      if (is<T>()) { return as<T>(); }
      return default_value;
    }

    bool isString() const;
    bool isNumber() const;
    bool isArray() const;
    bool isObject() const;
    bool isBoolean() const;

    // IO
    void toString(std::string& out, bool pretty_print = true, uint8_t tab_size = 4) const;

    template<typename T>
    T& getOrCast()
    {
      return _castAndGet<T>();
    }

    template<typename T>
    const T& getOrCast() const
    {
      return _castAndGet<T>();
    }

   private:
    void        _toString(std::string& out, bool pretty_print = true, uint8_t tab_size = 4, uint32_t indent = 0) const;
    static void _addNSpaces(std::string& out, uint32_t indent);
    template<typename T>
    inline T& _castAndGet();
    template<typename T>
    inline const T& _castAndGet() const;
  };

  template<typename T>
  inline T& JsonValue::_castAndGet()
  {
    if (!is<T>())
    {
      set<T>();
    }
    return get<T>();
  }

  template<typename T>
  inline const T& JsonValue::_castAndGet() const
  {
    return get<T>();
  }
}  // namespace bifrost

#endif /* TIDE_JSON_VALUE_HPP */
