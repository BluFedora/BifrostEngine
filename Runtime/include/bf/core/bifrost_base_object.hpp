/******************************************************************************/
/*!
 * @file   bf_base_object.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  All reflectable / serializable engine objects inherit from this class.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_BASE_OBJECT_HPP
#define BF_BASE_OBJECT_HPP

#include "bf/ListView.hpp"                  /* ListNode<T>         */
#include "bf/asset_io/bf_classlist.hpp"     /* BaseObjectClassID   */
#include "bf/meta/bifrost_meta_factory.hpp" /* AutoRegisterType<T> */

namespace bf
{
  // Forward Declarations

  class ISerializer;
  class IDocument;

  //

  struct ResourceID
  {
    std::uint32_t id = {};  //!, 0 is an invalid id.

    bool operator==(const ResourceID& rhs) const
    {
      return id == rhs.id;
    }
  };

  // Use this interface if you want to refer to objects generically.

  class bfPureInterface(IBaseObject)
  {
    friend class IDocument;

    // TODO(SR): Having data members in an interface probably breaks some of my codebase guidelines.
   private:
    ResourceID m_FileID;  //!< The local id unique inside of the particular document the object is part of.

   public:
    ResourceID fileID() const { return m_FileID; }

   public:
    virtual ClassID::Type            classID() const = 0;
    virtual meta::BaseClassMetaInfo* type() const    = 0;
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

#endif /* BF_BASE_OBJECT_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2019-2021 Shareef Abdoul-Raheem

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
