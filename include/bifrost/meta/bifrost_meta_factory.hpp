
// Impl Inspired by: [http://www.nirfriedman.com/2018/04/29/unforgettable-factory/]
#ifndef BIFROST_META_FACTORY_HPP
#define BIFROST_META_FACTORY_HPP

#include "bifrost/data_structures/bifrost_hash_table.hpp" /* HashTable<K, V>       */
#include "bifrost/memory/bifrost_imemory_manager.hpp"     /* IMemoryManager        */
#include "bifrost_meta_member.hpp"                        /* membersOf, is_class_v */
#include "bifrost_meta_utils.hpp"                         /* for_each              */

#include <string_view> /* string_view */

namespace bifrost::meta
{
  template<typename BaseT, typename... CtorArgs>
  class Factory
  {
    friend BaseT;

   private:
    using CreateFunction = BaseT* (*)(IMemoryManager&, CtorArgs...);

    // So that only 'Base' can create 'BaseT'.
    struct PrivateCtorTag final
    {
    };

   public:
    // Extra 'T's are registering extra classes to this same ap.
    template<typename... T>
    class Base : public BaseT
    {
      friend T;

     private:
      static bool s_IsRegistered;

      static bool registerImpl()
      {
        for_each_template<T...>([](auto t) {
          using Type = bfForEachTemplateT(t);

          for_each(meta::membersOf<Type>(), [](const auto& member) {
            if constexpr (meta::is_class_v<decltype(member)>)
            {
              s_FactoryData[member.name()] = [](IMemoryManager& allocator, CtorArgs... args) -> BaseT* {
                return allocator.alloc_t<Type>(std::forward<CtorArgs>(args)...);
              };
            }
          });
        });

        return true;
      }

     public:
      Base() :
        BaseT(PrivateCtorTag{})
      {
        // Force the Compiler to register the type.
        (void)s_IsRegistered;
      }
    };

   private:
    static HashTable<std::string_view, CreateFunction> s_FactoryData;

   public:
    static BaseT* instanciate(std::string_view class_name, IMemoryManager& allocator, CtorArgs... args)
    {
      auto* const ctor = s_FactoryData.get(class_name);

      if (ctor)
      {
        return (*ctor)(allocator, std::forward<CtorArgs>(args)...);
      }

      // TODO(Shareef): Throw exception?
      return nullptr;
    }

   private:
    Factory() = default;
  };

  template<typename BaseT, typename... CtorArgs>
  HashTable<std::string_view, typename Factory<BaseT, CtorArgs...>::CreateFunction> Factory<BaseT, CtorArgs...>::s_FactoryData = {};

  template<typename BaseT, typename... CtorArgs>
  template<typename... T>
  bool Factory<BaseT, CtorArgs...>::Base<T...>::s_IsRegistered = registerImpl();
}  // namespace bifrost::meta

#endif /* BIFROST_META_FACTORY_HPP */
