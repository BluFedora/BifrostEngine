#ifndef BIFROST_META_FACTORY_HPP
#define BIFROST_META_FACTORY_HPP

#include "bifrost/memory/bifrost_imemory_manager.hpp"     /* IMemoryManager        */
#include "bifrost_meta_member.hpp"                        /* membersOf, is_class_v */
#include "bifrost_meta_runtime_impl.hpp"                  /* TypeInfo_<T>          */
#include "bifrost_meta_utils.hpp"                         /* for_each              */

namespace bifrost::meta
{
  template<typename BaseT, typename... CtorArgs>
  class Factory
  {
    friend BaseT;

   private:
    // So that only 'Base' can create 'BaseT'.
    struct PrivateCtorTag final
    {
    };

   public:
    // Extra 'T's are registering extra classes.
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
              if (!TypeInfo<Type>::get())
              {
                // TypeInfo<Type>::get() = gRttiMemory().alloc_t<ClassMetaInfo<Type>>(member.name());
              }
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
    Factory() = default;
  };

  template<typename BaseT, typename... CtorArgs>
  template<typename... T>
  bool Factory<BaseT, CtorArgs...>::Base<T...>::s_IsRegistered = registerImpl();
}  // namespace bifrost::meta

#endif /* BIFROST_META_FACTORY_HPP */
