/******************************************************************************/
/*!
* @file   bf_json.hpp
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
#ifndef BF_JSON_HPP
#define BF_JSON_HPP

#include "bf/data_structures/bifrost_array.hpp"      /* Array<T>        */
#include "bf/data_structures/bifrost_hash_table.hpp" /* HashTable<K, V> */
#include "bf/data_structures/bifrost_string.hpp"     /* String          */
#include "bf/data_structures/bifrost_variant.hpp"    /* Variant<Ts...>  */

#include <initializer_list> /* initializer_list */

namespace bf::json
{
  class Value;

  using Pair    = std::pair<String, Value>;
  using Object  = HashTable<String, Value, 16>;
  using Array   = ::bf::Array<Value>;
  using String  = ::bf::String;
  using Number  = double;
  using Boolean = bool;

  namespace detail
  {
    using BaseValue         = Variant<Object, Array, String, Number, Boolean>;
    using ObjectInitializer = std::initializer_list<Pair>;
    using ArrayInitializer  = std::initializer_list<Value>;
  }  // namespace detail

  Value parse(char* source, std::size_t source_length);
  void  toString(const Value& json, String& out);

  class Value final : public detail::BaseValue
  {
   public:
    using Base_t = detail::BaseValue;

   public:
    // Constructors
    Value() = default;

    template<typename T>
    Value(const T& data_in) noexcept :
      Base_t(data_in)
    {
    }

    template<std::size_t N>
    Value(const char (&data_in)[N]) noexcept :
      Base_t(String(data_in))
    {
    }

    Value(detail::ObjectInitializer&& values);
    Value(detail::ArrayInitializer&& values);
    Value(const char* value);
    Value(int value);
    Value(std::uint64_t value);
    Value(std::int64_t value);

    // Assignment

    Value& operator=(detail::ObjectInitializer&& values);
    Value& operator=(detail::ArrayInitializer&& values);

    // Meta API

    using Base_t::as;
    using Base_t::is;
    using Base_t::type;
    bool isObject() const { return Base_t::is<Object>(); }
    bool isArray() const { return Base_t::is<Array>(); }
    bool isString() const { return Base_t::is<String>(); }
    bool isNumber() const { return Base_t::is<double>(); }
    bool isBoolean() const { return Base_t::is<bool>(); }

    // Cast API

    template<typename T>
    const T& asOr(const T& default_value) const
    {
      if (Base_t::is<T>())
      {
        return Base_t::as<T>();
      }

      return default_value;
    }

    template<typename T, typename... Args>
    T& cast(Args&&... args)
    {
      if (!Base_t::is<T>())
      {
        set<T>(std::forward<Args>(args)...);
      }

      return Base_t::as<T>();
    }

    // Object API

    Value& operator[](const StringRange& key);
    Value& operator[](const char* key);
    Value* at(const StringRange& key) const;

    template<typename T>
    T get(const StringRange& key, const T& default_value = {}) const
    {
      if (isObject())
      {
        Value* const value = at(key);

        return value && value->is<T>() ? value->as<T>() : default_value;
      }

      return default_value;
    }

    // Array API

    Value&      operator[](int index);  // Must be an int (rather than size_t) as to not conflict with the 'const char* key' overload.
    std::size_t size() const;
    void        push(const Value& item);
    Value&      push();
    void        insert(std::size_t index, const Value& item);
    Value&      back();
    void        pop();

    // Special Operations

    //
    // If isObject() then adds {key, value} to the map.
    // If isArray    then value is pushed.
    // Else this object is assigned to value.
    //
    // Only Objects use the key parameter.
    //
    void   add(StringRange key, const Value& value);
    Value& add(StringRange key);
  };
}  // namespace bf::json

#endif /* BF_JSON_HPP */

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
