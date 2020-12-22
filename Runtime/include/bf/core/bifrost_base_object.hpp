/******************************************************************************/
/*!
 * @file   bifrost_base_object.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  All reflectable / serializable engine objects inherit from this class.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BIFROST_BASE_OBJECT_HPP
#define BIFROST_BASE_OBJECT_HPP

#include "bf/meta/bifrost_meta_factory.hpp" /* AutoRegisterType<T> */

namespace bf
{
  // Forward Declarations

  class ISerializer;

  // Use this interface if you want to refer to objects generically.

  class bfPureInterface(IBaseObject)
  {
   public:
    virtual meta::BaseClassMetaInfo* type() const = 0;
    virtual void                     reflect(ISerializer & serializer);
    virtual ~IBaseObject() = default;
  };

  namespace detail
  {
    // clang-format off
    class BaseObjectT : public IBaseObject, public meta::AutoRegisterType<BaseObjectT>
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

    static meta::BaseClassMetaInfo* staticType() { return meta::typeInfoGet<T>(); }
    meta::BaseClassMetaInfo*        type() const override { return staticType(); }
  };
}  // namespace bf

#endif /* BIFROST_BASE_OBJECT_HPP */
