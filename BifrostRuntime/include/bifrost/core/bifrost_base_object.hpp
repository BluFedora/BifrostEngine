/******************************************************************************/
/*!
 * @file   bifrost_base_object.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  All reflectable / serializable engine objects will inherit from this class.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BIFROST_BASE_OBJECT_HPP
#define BIFROST_BASE_OBJECT_HPP

#include "bifrost/meta/bifrost_meta_factory.hpp" /* Factory<T> */

class Engine;

namespace bf
{
  class bfPureInterface(IBaseObject)
  {
   public:
    virtual meta::BaseClassMetaInfo* type() = 0;
    virtual ~IBaseObject()                  = default;
  };

  // NOTE(Shareef): Use this class as the base type.

  namespace detail
  {
    // clang-format off
    class BaseObjectT : public meta::Factory<BaseObjectT>, public IBaseObject
    // clang-format on
    {
     protected:
      explicit BaseObjectT(PrivateCtorTag) {}
    };
  }  // namespace detail

  // NOTE(Shareef): Inherit from this
  template<typename T>
  class BaseObject : public detail::BaseObjectT::Base<T>
  {
   public:
    using Base = BaseObject<T>;

    static meta::BaseClassMetaInfo* staticType()
    {
      return meta::TypeInfo<meta::NthTypeOf<0, T>>::get();
    }

    meta::BaseClassMetaInfo* type() override
    {
      return staticType();
    }
  };
}  // namespace bifrost

#endif /* BIFROST_BASE_OBJECT_HPP */
