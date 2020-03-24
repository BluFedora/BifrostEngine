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

  class JsonValue : public JsonValue_t
  {
   public:
    // Constructors
     [[deprecated("New Json Utilities should be used instead")]]
    JsonValue() = default;
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue(std::initializer_list<std::pair<const string_t, JsonValue>>&& values);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue(std::initializer_list<JsonValue>&& values);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue(array_t&& arr);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue(number_t number);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue(const string_t& value);

    // Assignment
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator=(number_t rhs);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator=(unsigned long rhs);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator=(int rhs);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator=(const string_t& rhs);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator=(const object_t& rhs);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator=(const array_t& rhs);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator=(boolean_t rhs);

    // Object API
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator[](const string_t& key);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& operator[](const char* key);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue& at(const string_t& key);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue* at(const string_t& key) const;

    template<typename T>
    [[deprecated("New Json Utilities should be used instead")]]
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
    [[deprecated("New Json Utilities should be used instead")]]
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
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue&  operator[](array_t::size_type index);
    [[deprecated("New Json Utilities should be used instead")]]
    void        push_back(const JsonValue& value);
    [[deprecated("New Json Utilities should be used instead")]]
    JsonValue&  at(array_t::size_type index);
    [[deprecated("New Json Utilities should be used instead")]]
    std::size_t size() const;

    // Meta
    using JsonValue_t::as;
    using JsonValue_t::is;
    using JsonValue_t::type;

    template<typename T>
    [[deprecated("New Json Utilities should be used instead")]]
    const T& as_or(const T& default_value) const
    {
      if (is<T>()) { return as<T>(); }
      return default_value;
    }

    [[deprecated("New Json Utilities should be used instead")]]
    bool isString() const;
    [[deprecated("New Json Utilities should be used instead")]]
    bool isNumber() const;
    [[deprecated("New Json Utilities should be used instead")]]
    bool isArray() const;
    [[deprecated("New Json Utilities should be used instead")]]
    bool isObject() const;
    [[deprecated("New Json Utilities should be used instead")]]
    bool isBoolean() const;

    // IO
    [[deprecated("New Json Utilities should be used instead")]]
    void toString(std::string& out, bool pretty_print = true, uint8_t tab_size = 4) const;

    template<typename T>
    [[deprecated("New Json Utilities should be used instead")]]
    T& getOrCast()
    {
      return _castAndGet<T>();
    }

    template<typename T>
    [[deprecated("New Json Utilities should be used instead")]]
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
