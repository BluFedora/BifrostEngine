/*!
 * @file   bifrost_meta_variant.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Defines a variant for holding all the data types that can
 *   inspected by this engine.
 *
 * @version 0.0.1
 * @date    2020-05-31
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_META_VARIANT_HPP
#define BIFROST_META_VARIANT_HPP

#include "bifrost/asset_io/bifrost_base_asset_handle.hpp" /* BaseAssetHandle, + All Serializable Classes */
#include "bifrost/bifrost_math.hpp"                       /* bfColor4f, bfColor4u                        */
#include "bifrost/data_structures/bifrost_string.hpp"     /* String                                      */
#include "bifrost/data_structures/bifrost_variant.hpp"    /* Variant<Ts...>                              */
#include "bifrost/ecs/bifrost_entity_ref.hpp"             /* EntityRef                                     */
#include "bifrost/utility/bifrost_uuid.h"                 /* BifrostUUID                                 */
#include "bifrost_meta_function_traits.hpp"               /* ParameterPack<Ts...>                        */
#include "bifrost_meta_utils.hpp"                         /* overloaded                                  */

#include <cstddef> /* byte                                                                     */
#include <cstdint> /* uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t */

namespace bifrost::meta
{
  class BaseClassMetaInfo;

  // [https://devblogs.microsoft.com/oldnewthing/20190710-00/?p=102678]
  template<typename, typename = void>
  constexpr bool is_type_complete_v = false;

  template<typename T>
  constexpr bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;

  struct MetaObject final
  {
    BaseClassMetaInfo* type_info;
    union
    {
      void*         object_ref;
      std::uint64_t enum_value;
    };
  };

  using MetaValueTypes = ParameterPack<
   bool,
   std::byte,
   std::int8_t,
   std::uint8_t,
   std::int16_t,
   std::uint16_t,
   std::int32_t,
   std::uint32_t,
   std::int64_t,
   std::uint64_t,
   float,
   double,
   long double,
   Vec2f,
   Vec3f,
   Quaternionf,
   bfColor4f,
   bfColor4u,
   String,
   IBaseObject*,
   BaseAssetHandle,
   EntityRef,
   BifrostUUIDNumber,
   BifrostUUID>;

  using MetaPrimitiveTypes = MetaValueTypes::extend<MetaObject>;
  using MetaVariant        = MetaPrimitiveTypes::apply<Variant>;

  namespace detail
  {
    template<typename T, typename TBase>
    static constexpr bool isSameOrBase()
    {
      using RawT     = std::decay_t<T>;
      using RawTBase = std::decay_t<TBase>;

      if constexpr (is_type_complete_v<std::remove_pointer_t<RawT>>)
      {
        return std::is_same_v<RawT, RawTBase> || std::is_base_of_v<std::remove_pointer_t<RawTBase>, std::remove_pointer_t<RawT>>;
      }
      else
      {
        // #pragma message("Undefined class type, this is ok but I want to check this very once in a while.")
        return std::is_same_v<RawT, RawTBase>;
      }
    }

    MetaVariant make(void*, BaseClassMetaInfo*);
    void*       doBaseObjStuff(IBaseObject* obj, BaseClassMetaInfo* type_info);
    bool        isEnum(const MetaObject& obj);
  }  // namespace detail

  // TODO(Shareef): This should be in an 'API' header.
  template<typename... Args, typename F>
  static void forEachParameterPack(ParameterPack<Args...>, F&& f)
  {
    for_each_template<Args...>(std::forward<F&&>(f));
  }

  template<typename... Args>
  static void debug()
  {
    __debugbreak();
  }

  template<bool... Args>
  static void debug()
  {
    __debugbreak();
  }

  template<typename T, typename TT, typename TTT>
  static void isValueType_(MetaVariant& result_value, bool& result, TTT&& data)
  {
    using RawTT = std::decay_t<TT>;
    using RawT  = std::decay_t<T>;

    if constexpr (std::is_convertible_v<RawT, RawTT> && detail::isSameOrBase<std::remove_reference_t<T>, std::remove_reference_t<TT>>())
    {
      if (!result)
      {
        result_value.set<TT>(data);
        result = true;
      }
    }
  }

  template<typename T>
  static bool isValueType(MetaVariant& result_value, T&& data)
  {
    bool result = false;

    forEachParameterPack(
     MetaValueTypes{},
     [&result, &data, &result_value](auto t) {
       using TT = bfForEachTemplateT(t);

       isValueType_<std::remove_reference_t<T>, TT>(result_value, result, data);
     });

    return result;
  }

  template<typename T>
  BaseClassMetaInfo* typeInfoGet();

  template<typename T>
  T* stripReference(T* ptr)
  {
    return ptr;
  }

  template<typename T>
  T* stripReference(T& ptr)
  {
    return &ptr;
  }

  template<typename T>
  MetaVariant makeVariant(T&& data)
  {
    MetaVariant res;

    if (!isValueType(res, data))
    {
      res = detail::make((void*)stripReference(data), typeInfoGet<T>());
    }

    return res;
  }

  BaseClassMetaInfo* variantTypeInfo(const MetaVariant& value);

  template<typename T>
  bool isVariantCompatible(const MetaVariant& value)
  {
    bool result = false;
    auto ov     = overloaded{
     [&result](const auto& arg) -> void {
       using ActualT = std::decay_t<decltype(arg)>;

       (void)arg;

       result = std::is_convertible_v<ActualT, std::decay_t<T>> || detail::isSameOrBase<T, ActualT>();
     },
     [&result](const MetaObject& meta_obj) -> void {
       result = meta_obj.type_info == typeInfoGet<T>();
     }};

    visit_all(ov,
              const_cast<MetaVariant&>(value));

    return result;
  }

  template<typename T>
  T variantToCompatibleT(const MetaVariant& value)
  {
    using TDecay = std::decay_t<std::conditional_t<std::is_reference_v<T>, std::add_pointer_t<std::remove_reference_t<T>>, T>>;

    if constexpr (MetaVariant::canContainT<T>())
    {
      if (value.is<TDecay>())
      {
        return value.as<TDecay>();
      }
    }

    if constexpr (is_type_complete_v<std::remove_pointer_t<T>>)
    {
      // NOTE(Shareef): The following "if constexpr" cannot be merged with the surrounding one due to
      // "is_base_of_v" only being allowed to be used on incomplete class types.
      // Short circuiting using "&&" seemed to not work on MSVC 2019-Preview as of June 3rd, 2020.
      // TODO(SR): Investigate more on the issue?

      if constexpr (std::is_base_of_v<std::remove_pointer_t<IBaseObject>, std::remove_pointer_t<T>>)
      {
        if (value.is<IBaseObject*>())
        {
          return (T)detail::doBaseObjStuff(value.as<IBaseObject*>(), typeInfoGet<TDecay>());
        }
      }
    }

    TDecay return_value;

    auto ov = overloaded{
     [&return_value](const auto& arg) {
       using ActualT = std::decay_t<decltype(arg)>;

       if constexpr (std::is_constructible_v<TDecay, const ActualT&>)
       {
         return_value = T(arg);
       }
     },
     [&return_value](const MetaObject& meta_obj) {
       if (detail::isEnum(meta_obj))
       {
         if constexpr (std::is_enum_v<TDecay> || std::is_integral_v<TDecay>)
         {
           return_value = static_cast<T>(const_cast<MetaObject&>(meta_obj).enum_value);
         }
       }
       else
       {
         if constexpr (std::is_pointer_v<TDecay>)
         {
           // This check is back for base class shenanigans (Ex: Asset System Stuff)
           // if (meta_obj.type_info == typeInfoGet<TDecay>())  // TODO(Shareef): This check isn't required if 'isVariantCompatible' is always called first..
           {
             return_value = static_cast<TDecay>(const_cast<MetaObject&>(meta_obj).object_ref);
           }
         }
         else
         {
           return_value = *static_cast<TDecay*>(const_cast<MetaObject&>(meta_obj).object_ref);
         }
       }
     }};

    if (value.valid())
    {
      bifrost::visit_all(ov, value);
    }

    if constexpr (std::is_reference_v<T>)
    {
      return *return_value;
    }
    else
    {
      return return_value;
    }
  }

// TODO(SR): CLean this mess up.
#if defined(_MSC_VER)
#define DISABLE_WARNING_PUSH __pragma(warning(push))
#define DISABLE_WARNING_POP __pragma(warning(pop))
#define DISABLE_WARNING(warningNumber) __pragma(warning(disable \
                                                        : warningNumber))  // NOLINT(bugprone-macro-parentheses)

#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(4100)
#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(4505)
#define DISABLE_WARNING_CONVERSION DISABLE_WARNING(4244)
  // other warnings you want to deactivate...

#elif defined(__GNUC__) || defined(__clang__)
#define DO_PRAGMA(X) _Pragma(#X)
#define DISABLE_WARNING_PUSH DO_PRAGMA(GCC diagnostic push)
#define DISABLE_WARNING_POP DO_PRAGMA(GCC diagnostic pop)
#define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored #warningName)

#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(-Wunused - parameter)
#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(-Wunused - function)
#define DISABLE_WARNING_CONVERSION DISABLE_WARNING(-Warith - conversion)

  // other warnings you want to deactivate...

#else
#define DISABLE_WARNING_PUSH
#define DISABLE_WARNING_POP
#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
#define DISABLE_WARNING_UNREFERENCED_FUNCTION
  // other warnings you want to deactivate...

#endif

  // TODO(Shareef): Find a way not to have this be copied code?
  DISABLE_WARNING_PUSH
  DISABLE_WARNING_CONVERSION

  template<typename T0, typename T1>
  static T1 convert(T0 value)
  {
    return T1(value);
  }

  DISABLE_WARNING_POP

  template<typename T>
  bool variantToCompatibleT2(void* return_value, const MetaVariant& value)
  {
    if (!value.valid())
    {
      return false;
    }

    using TDecay = std::decay_t<T>;

    if constexpr (MetaVariant::canContainT<T>())
    {
      if (value.is<TDecay>())
      {
        new (return_value) TDecay(value.as<TDecay>());
        return true;
      }
    }

    if constexpr (std::is_base_of_v<std::remove_pointer_t<IBaseObject>, std::remove_pointer_t<T>>)
    {
      if (value.is<IBaseObject*>())
      {
        T* const rv = (T*)return_value;

        *rv = detail::doBaseObjStuff(value.as<IBaseObject*>(), typeInfoGet<TDecay>());

        return *rv != nullptr;
      }
    }

    bool found_match = false;

    auto ov = overloaded{
     [&return_value, &found_match](const auto& arg) {
       using ActualT = std::decay_t<decltype(arg)>;

       if constexpr (std::is_convertible_v<ActualT, TDecay>)
       {
         const T arg_t = convert<ActualT, TDecay>(arg);  //static_cast<const T>(arg);
         new (return_value) T(arg_t);
         found_match = true;
       }
     },
     [&return_value, &found_match](const MetaObject& meta_obj) {
       if (detail::isEnum(meta_obj))
       {
         if constexpr (std::is_enum_v<TDecay> || std::is_integral_v<TDecay>)
         {
           new (return_value) T(static_cast<T>(const_cast<MetaObject&>(meta_obj).enum_value));
           found_match = true;
         }
       }
       else
       {
         if constexpr (std::is_pointer_v<TDecay>)
         {
           if (meta_obj.type_info == typeInfoGet<TDecay>())  // TODO(Shareef): This check isn't required if 'isVariantCompatible' is always called first..
           {
             new (return_value) T(static_cast<T>(const_cast<MetaObject&>(meta_obj).object_ref));
             found_match = true;
           }
         }
       }
     }};

    if (value.valid())
    {
      bifrost::visit_all(ov, value);
    }

    return found_match;
  }

#undef DISABLE_WARNING_PUSH
#undef DISABLE_WARNING_POP
#undef DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
#undef DISABLE_WARNING_UNREFERENCED_FUNCTION
}  // namespace bifrost::meta

#endif /* BIFROST_META_VARIANT_HPP */