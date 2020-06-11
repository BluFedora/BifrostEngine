#ifndef BIFROST_META_FACTORY_HPP
#define BIFROST_META_FACTORY_HPP

#include "bifrost_meta_member.hpp"                        /* membersOf, is_class_v */
#include "bifrost_meta_runtime_impl.hpp"                  /* TypeInfo<T>           */
#include "bifrost_meta_utils.hpp"                         /* for_each              */

namespace bifrost::meta
{
  template<typename BaseT>
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
      friend NthTypeOf<0, T...>;

     private:
      static bool s_IsRegistered;

      static bool registerImpl()
      {
        for_each_template<T...>([](auto t) {
          using Type = bfForEachTemplateT(t);
          TypeInfo<Type>::get();
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

  template<typename BaseT>
  template<typename... T>
  bool Factory<BaseT>::Base<T...>::s_IsRegistered = registerImpl();
}  // namespace bifrost::meta

#endif /* BIFROST_META_FACTORY_HPP */
