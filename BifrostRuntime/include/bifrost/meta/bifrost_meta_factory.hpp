#ifndef BIFROST_META_FACTORY_HPP
#define BIFROST_META_FACTORY_HPP

#include "bifrost_meta_runtime_impl.hpp" /* TypeInfo<T> */

namespace bf::meta
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
    template<typename T>
    class Base : public BaseT
    {
      friend T;

     private:
      static bool s_IsRegistered;

      static bool registerImpl()
      {
        return TypeInfo<T>::get() != nullptr;
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
  template<typename T>
  bool Factory<BaseT>::Base<T>::s_IsRegistered = registerImpl();
}  // namespace bifrost::meta

#endif /* BIFROST_META_FACTORY_HPP */
