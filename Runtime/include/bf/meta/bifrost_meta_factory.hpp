#ifndef BF_META_FACTORY_HPP
#define BF_META_FACTORY_HPP

#include "bifrost_meta_runtime_impl.hpp" /* TypeInfo<T> */

namespace bf::meta
{
  //
  // Mix-in class to inherit from that 'automatically' registers
  // a type with the meta system on application startup
  // by abusing C++'s static initialization system.
  //
    
  template<typename BaseT>
  class AutoRegisterType
  {
    friend BaseT;

   private:
    // So that only 'Base' can create 'BaseT'.
    struct PrivateCtorTag final
    {
    };

   public:
    // 'T' is the class you would like to register.
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
    AutoRegisterType() = default;
  };

  template<typename BaseT>
  template<typename T>
  bool AutoRegisterType<BaseT>::Base<T>::s_IsRegistered = registerImpl();
}  // namespace bf::meta

#endif /* BF_META_FACTORY_HPP */
