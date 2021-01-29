/******************************************************************************/
/*!
 * @file   sr_meta_interface.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Defines the abstract interfaces that get implemented by "SR-CXXMeta-Gen"'s
 *   code generation output.
 *
 * @date 2021-26-01
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef SR_META_INTERFACE_HPP
#define SR_META_INTERFACE_HPP

#include <cstddef>      // size_t
#include <cstdint>      // Sized int types
#include <cstring>      // memcpy
#include <type_traits>  // remove_reference_t, remove_pointer_t, decay_t

namespace sr
{
  //
  // Holds a pointer to a compile-time constant string.
  //
  struct ConstStr
  {
    const char* const str    = nullptr;
    const std::size_t length = 0u;

    template<std::size_t N>
    ConstStr(const char (&string)[N]) :
      str{string},
      length{N - 1}
    {
    }
  };

  //
  // A non owning view into an array that contains `T`s.
  //
  template<typename T>
  struct ConstArrayView
  {
    const T* const first;
    const T* const last;

    ConstArrayView(const T* base_class_info_bgn, const T* base_class_info_end) :
      first{base_class_info_bgn},
      last{base_class_info_end}
    {
    }

    using iterator = const T**;

    iterator begin() const { return first; }
    iterator end() const { return last; }
  };

  //
  // A non owning view into an array that contains pointers to `T`s.
  //
  template<typename T>
  using ConstPtrArrayView = ConstArrayView<const T*>;

  namespace Meta
  {
    struct Type;

    template<typename T>
    Type* GetType();

    //
    // `GenericValue`s are very unsafe but allows
    // for passing around of data across virtual interface boundaries.
    //

    struct TypedObject
    {
      const Type* type;
      void*       ptr;
    };

    union GenericValue
    {
      bool          b1;
      char          ch;
      std::int8_t   i8;
      std::uint8_t  u8;
      std::int16_t  i16;
      std::uint16_t u16;
      std::int32_t  i32;
      std::uint32_t u32;
      std::int64_t  i64;
      std::uint64_t u64;
      std::size_t   umm;
      float         f32;
      double        f64;
      long double   f64_l;
      void*         ptr;
      const char*   str;
      TypedObject   obj;
    };

    template<typename T>
    GenericValue makeGenericValue(T&& object);

    template<typename T>
    T castGenericValue(GenericValue value);

    //
    // The subclass type for each meta info object.
    //
    enum class InfoType
    {
      NAMESPACE,           //!< A namespace just holds a list of types.
      PRIMITIVE_TYPE,      //!< Simple data types such as int's and floats.
      CLASS,               //!< User defined data types.
      ARRAY,               //!< An extension of Class that allows for manipulating the elements by index.
      UNION,               //!< A type that just has a list of strings indicating the field names.
      ENUM,                //!< List of EnumValues for easy enum-to-string and vice-versa.
      ENUM_VALUE,          //!< A pair consisting of a string and a integer value.
      PROPERTY,            //!< Abstract getter / setting pair.
      FIELD,               //!< An specialization of Property for member fields that actually exist on the Class.
      FUNCTION,            //!< Can be invoked with some GenericValue parameters.
      FUNCTION_PARAMETER,  //!< A pair of a Type and a name.
    };

    //
    // Base type for all meta objects.
    // Use the `BaseInfo::type` method to query what subclass you can cast to.
    //
    struct BaseInfo
    {
      const ConstStr name;

      template<std::size_t N>
      BaseInfo(const char (&name)[N]) :
        name{name}
      {
      }

      virtual InfoType type() const = 0;

      template<typename T>
      T* as()
      {
        return dynamic_cast<T*>(this);
      }

      virtual ~BaseInfo() = default;
    };

    //
    // All meta data types inherit from this class
    // as they all contain a size and an alignment.
    //
    struct Type : public BaseInfo
    {
      const std::size_t size      = 0u;
      const std::size_t alignment = 0u;

      template<std::size_t N>
      Type(const char (&type_name)[N], std::size_t size, std::size_t alignment) :
        BaseInfo(type_name),
        size{size},
        alignment{alignment}
      {
      }

      InfoType type() const override { return InfoType::PRIMITIVE_TYPE; }
    };

    struct FunctionParameter final : public BaseInfo
    {
      const Type* param_type;

      template<std::size_t N>
      FunctionParameter(const char (&function_param_name)[N],
                        const Type* type) :
        BaseInfo(function_param_name),
        param_type{type}
      {
      }

      InfoType type() const override { return InfoType::FUNCTION_PARAMETER; }
    };

    struct Function : public BaseInfo
    {
      const Type*                       return_value;
      ConstArrayView<FunctionParameter> parameters;

      template<std::size_t N>
      Function(const char (&function_name)[N],
               const Type*              return_value,
               const FunctionParameter* parameters_first,
               const FunctionParameter* parameters_last) :
        BaseInfo(function_name),
        return_value{return_value},
        parameters{parameters_first, parameters_last}
      {
      }

      InfoType             type() const override final { return InfoType::FUNCTION; }
      virtual GenericValue invoke(const GenericValue* args, std::size_t num_args) = 0;

      ~Function() override = default;
    };

    struct Namespace : public BaseInfo
    {
      ConstPtrArrayView<BaseInfo> declarations;

      template<std::size_t N>
      Namespace(const char (&namespace_name)[N],
                const BaseInfo** first_decl,
                const BaseInfo** last_decl) :
        BaseInfo{namespace_name},
        declarations{first_decl, last_decl}
      {
      }

      InfoType type() const override final { return InfoType::PRIMITIVE_TYPE; }
    };

    struct Property : public BaseInfo
    {
      const Type* qual_type;

      template<std::size_t N>
      Property(const char (&property_name)[N], const Type* type) :
        BaseInfo(property_name),
        qual_type(type)
      {
      }

      InfoType type() const override { return InfoType::PROPERTY; }

      // `out` is expected to be at least the size of `type.raw_type->size`.
      virtual void get(const void* instance, void* out) const = 0;

      // `in` must point to a buffer at least the size of `type.raw_type->size` in length.
      virtual void set(void* instance, const void* in) const = 0;

      ~Property() override = default;
    };

    struct Field final : public Property
    {
      std::size_t byte_offset = 0u;

      template<std::size_t N>
      Field(const char (&field_name)[N], const Type* type, std::size_t byte_offset) :
        Property(field_name, type),
        byte_offset(byte_offset)
      {
      }

      InfoType type() const override { return InfoType::FIELD; }

      void get(const void* instance, void* out) const override final
      {
        std::memcpy(out, static_cast<const char*>(instance) + byte_offset, qual_type->size);
      }

      void set(void* instance, const void* in) const override final
      {
        std::memcpy(static_cast<char*>(instance) + byte_offset, in, qual_type->size);
      }
    };

    struct Class : public Type
    {
      ConstPtrArrayView<Type>     base_classes;
      ConstPtrArrayView<Property> properties;
      ConstPtrArrayView<Function> methods;

      template<std::size_t N>
      Class(const char (&type_name)[N],
            std::size_t      size,
            std::size_t      alignment,
            const Type**     base_class_info_bgn,
            const Type**     base_class_info_end,
            const Property** properties_bgn,
            const Property** properties_end,
            const Function** methods_bgn,
            const Function** methods_end) :
        Type(type_name, size, alignment),
        base_classes{base_class_info_bgn, base_class_info_end},
        properties{properties_bgn, properties_end},
        methods{methods_bgn, methods_end}
      {
      }

      InfoType type() const override { return InfoType::CLASS; }
    };

    //
    // Interface for interacting with Array like types.
    //
    struct Array : public Class
    {
      template<std::size_t N>
      Array(const char (&type_name)[N],
            std::size_t      size,
            std::size_t      alignment,
            const Type**     base_class_info_bgn = nullptr,
            const Type**     base_class_info_end = nullptr,
            const Property** properties_bgn      = nullptr,
            const Property** properties_end      = nullptr,
            const Function** methods_bgn         = nullptr,
            const Function** methods_end         = nullptr) :
        Class(type_name, size, alignment, base_class_info_bgn, base_class_info_end, properties_bgn, properties_end, methods_bgn, methods_end)
      {
      }

      InfoType type() const override final { return InfoType::ARRAY; }

      virtual const Type*  elementType() const                                                 = 0;
      virtual std::size_t  numElements(const void* instance)                                   = 0;
      virtual GenericValue getElementAt(const void* instance, std::size_t index)               = 0;
      virtual void         setElementAt(void* instance, std::size_t index, GenericValue value) = 0;
    };

    //
    // Helper for Array to help with boilerplate.
    //
    template<typename TContainer, typename TElement>
    struct ArrayT : public Array
    {
      template<std::size_t N>
      ArrayT(const char (&type_name)[N],
             const Type**     base_class_info_bgn = nullptr,
             const Type**     base_class_info_end = nullptr,
             const Property** properties_bgn      = nullptr,
             const Property** properties_end      = nullptr,
             const Function** methods_bgn         = nullptr,
             const Function** methods_end         = nullptr) :
        Array(type_name, sizeof(TContainer), alignof(TContainer), base_class_info_bgn, base_class_info_end, properties_bgn, properties_end, methods_bgn, methods_end)
      {
      }

      const Type* elementType() const override
      {
        return GetType<TElement>();
      }

      std::size_t numElements(const void* instance) override
      {
        return static_cast<const TContainer*>(instance)->size();
      }

      GenericValue getElementAt(const void* instance, std::size_t index) override
      {
        return makeGenericValue((*static_cast<const TContainer*>(instance))[index]);
      }

      void setElementAt(void* instance, std::size_t index, GenericValue value) override
      {
        (*static_cast<TContainer*>(instance))[index] = castGenericValue<TElement>(value);
      }
    };

    /*
    struct Union : public Type
    {
      ConstArrayView<ConstStr> field_names = {nullptr, nullptr};
    };
    */

    struct EnumField final : public BaseInfo
    {
      const std::int64_t value;

      template<std::size_t N>
      EnumField(const char (&enum_field_name)[N], std::int64_t value) :
        BaseInfo(enum_field_name),
        value(value)
      {
      }

      InfoType type() const override { return InfoType::ENUM_VALUE; }
    };

    struct Enum : public Type
    {
      const Type*               underlying_type;  //!< Pointer to the (u)int Type that backs the enum.
      ConstArrayView<EnumField> fields;

      template<std::size_t N>
      Enum(const char (&enum_name)[N],
           std::size_t      size,
           std::size_t      alignment,
           const Type*      underlying_type,
           const EnumField* fields_bgn,
           const EnumField* fields_end) :
        Type(enum_name, size, alignment),
        underlying_type{underlying_type},
        fields{fields_bgn, fields_end}
      {
      }

      InfoType type() const override final { return InfoType::ENUM; }
    };

    // Type Resolution //

    template<typename T>
    struct TypeResolver
    {
      static Type* get() = delete;
    };

    //
    // Make sure even with a bunch of type qualifiers the type resolves the the decayed type.
    //

    template<typename T>
    using FullyDecayed = std::remove_reference_t<std::remove_pointer_t<std::decay_t<T>>>;

    // clang-format off
    template<typename T> struct TypeResolver<const T>           : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<volatile T>        : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<const volatile T>  : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<T*>                : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<const T*>          : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<const volatile T*> : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<const T&>          : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<volatile T&>       : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<const volatile T&> : public TypeResolver<FullyDecayed<T>> { };
    template<typename T> struct TypeResolver<T&>                : public TypeResolver<FullyDecayed<T>> { };
    // clang-format on

#define SR_IMPL_GET_PRIMITIVE_TYPE(T)                \
  template<>                                         \
  struct TypeResolver<T>                             \
  {                                                  \
    static Type* get()                               \
    {                                                \
      static Type s_Type{#T, sizeof(T), alignof(T)}; \
      return &s_Type;                                \
    }                                                \
  }

    // SR_IMPL_GET_PRIMITIVE_TYPE(std::byte);
    SR_IMPL_GET_PRIMITIVE_TYPE(bool);
    SR_IMPL_GET_PRIMITIVE_TYPE(char);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::int8_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::uint8_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::int16_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::uint16_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::int32_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::uint32_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::int64_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(std::uint64_t);
    SR_IMPL_GET_PRIMITIVE_TYPE(float);
    SR_IMPL_GET_PRIMITIVE_TYPE(double);
    SR_IMPL_GET_PRIMITIVE_TYPE(long double);
    SR_IMPL_GET_PRIMITIVE_TYPE(void*);
#undef SR_IMPL_GET_PRIMITIVE_TYPE

    template<typename T>
    Type* GetType()
    {
      return TypeResolver<T>::get();
    }

#define SR_DEFINE_PRIMITIVE_GEN_VAL(T, member)  \
  template<>                                    \
  GenericValue makeGenericValue<T>(T && object) \
  {                                             \
    GenericValue result;                        \
    result.member = object;                     \
    return result;                              \
  }                                             \
                                                \
  template<>                                    \
  T castGenericValue<T>(GenericValue value)     \
  {                                             \
    T result = value.member;                    \
    return result;                              \
  }

    SR_DEFINE_PRIMITIVE_GEN_VAL(bool, b1)
    // SR_IMPL_GET_PRIMITIVE_TYPE(std::byte);
    SR_DEFINE_PRIMITIVE_GEN_VAL(char, ch);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::int8_t, i8);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::uint8_t, u8);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::int16_t, i16);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::uint16_t, u16);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::int32_t, i32);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::uint32_t, u32);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::int64_t, i64);
    SR_DEFINE_PRIMITIVE_GEN_VAL(std::uint64_t, u64);
    SR_DEFINE_PRIMITIVE_GEN_VAL(float, f32);
    SR_DEFINE_PRIMITIVE_GEN_VAL(double, f64);
    SR_DEFINE_PRIMITIVE_GEN_VAL(long double, f64_l);
    SR_DEFINE_PRIMITIVE_GEN_VAL(void*, ptr);
#undef SR_DEFINE_PRIMITIVE_GEN_VAL

    template<typename T>
    GenericValue makeGenericValue(T&& object)
    {
      GenericValue result;
      result.obj = {GetType<T>(), static_cast<void*>(const_cast<void*>(&object))};
      return result;
    }

    template<typename T>
    T castGenericValue(GenericValue value)
    {
      return *static_cast<T*>(value.obj.ptr);
    }
  }  // namespace Meta
}  // namespace sr

#endif /* SR_META_INTERFACE_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

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
