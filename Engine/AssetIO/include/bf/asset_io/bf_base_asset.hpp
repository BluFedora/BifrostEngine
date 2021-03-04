/******************************************************************************/
/*!
 * @file   bf_base_asset.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Interface for creating asset types the engine can use.
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_BASE_ASSET_HPP
#define BF_BASE_ASSET_HPP

#include "bf/bf_core.h"                     // bfPureInterface
#include "bf/core/bifrost_base_object.hpp"  // IBaseObject

namespace bf
{
  class Assets;
  class ISerializer;
  class IDocument;

  struct ResourceReference
  {
    bfUUIDNumber doc_id  = {};  //!< The document to refer to, if empty it is a local/internal reference.
    ResourceID   file_id = {};  //!< The item within the `doc_id`'s document to get.
  };

  // clang-format off
  class bfPureInterface(IBaseAsset) : public IBaseObject
  // clang-format on
  {
    friend class Assets;
    friend class IDocument;

   private:
    String               m_Name;                 //!< String name not used for any useful work, just for eye candy.
    IDocument*           m_Document;             //!< The document that owns this asset, nullptr if the asset is not backed by an actual file.
    ListNode<IBaseAsset> m_DocResourceListNode;  //!< Used with 'IDocument::m_Assets' to make an intrusive non-owning linked list.

   public:
    IBaseAsset();

    // Accessors //

    [[nodiscard]] IDocument&    document() const { return *m_Document; }
    [[nodiscard]] bool          hasDocument() const { return m_Document != nullptr; }
    [[nodiscard]] const String& name() const { return m_Name; }

    // Ref Count //

    void acquire();
    void release();

    // Misc //

    ResourceReference toRef() const;
  };

  namespace detail
  {
    // clang-format off
    class BaseAssetT : public IBaseAsset, public meta::AutoRegisterType<BaseAssetT>, private NonCopyMoveable<BaseAssetT>
    {
     protected:
      explicit BaseAssetT(PrivateCtorTag) :
        IBaseAsset(),
        AutoRegisterType<BaseAssetT>(),
        NonCopyMoveable<BaseAssetT>()
      {}
    };
    // clang-format on
  }  // namespace detail

  // clang-format off
  template<typename T>
  class BaseAsset : public detail::BaseAssetT::Base<T>
  // clang-format on
  {
   public:
    using Base = BaseAsset<T>;

    BaseAsset() = default;

    meta::BaseClassMetaInfo* type() const final override
    {
      return meta::typeInfoGet<T>();
    }
  };

  //
  // ARC = Automatic Reference Count
  //

  //
  // This interface exists so that you can manipulate an
  // `ARC` handle generically particularly in serialization
  // and editor code.
  //
  // clang-format off
  class bfPureInterface(IARCHandle)
  // clang-format on
  {
   public:
    virtual bool                     isValid() const noexcept   = 0;
    virtual meta::BaseClassMetaInfo* typeInfo() const noexcept  = 0;
    virtual void                     assign(IBaseAsset * asset) = 0;
    virtual IBaseAsset*              handle() const noexcept    = 0;
    virtual ~IARCHandle()                                       = default;
  };

  //
  // Automatically handles calling acquire and release on the associated IBaseAsset pointer.
  //

  template<typename T>
  class ARC final : public IARCHandle
  {
    // using T = IBaseAsset;

    static_assert(!std::is_pointer_v<T>, "T must not be a pointer.");
    static_assert(std::is_convertible_v<T*, IBaseAsset*>, "The type must implement the IBaseAsset interface.");

   private:
    T* m_Handle;

   public:
    ARC(T* handle = nullptr) :
      IARCHandle(),
      m_Handle{handle}
    {
      acquire();
    }

    // NOTE(Shareef): Useful to set a handle to nullptr to represent null.
    ARC(std::nullptr_t) :
      IARCHandle(),
      m_Handle{nullptr}
    {
    }

    ARC(const ARC& rhs) :
      IARCHandle(),
      m_Handle{rhs.m_Handle}
    {
      acquire();
    }

    // clang-format off

    ARC(ARC&& rhs) noexcept :
      IARCHandle(),
      m_Handle{std::exchange(rhs.m_Handle, nullptr)}
    {
    }

    // clang-format on

    ARC& operator=(const ARC& rhs)  // NOLINT(bugprone-unhandled-self-assignment)
    {
      if (this != &rhs)
      {
        reassign(rhs.m_Handle);
      }

      return *this;
    }

    ARC& operator=(ARC&& rhs) noexcept
    {
      m_Handle = std::exchange(rhs.m_Handle, m_Handle);

      return *this;
    }

    ARC& operator=(std::nullptr_t)
    {
      release();
      m_Handle = nullptr;

      return *this;
    }

         operator bool() const noexcept { return isValid(); }
    T&   operator*() const noexcept { return *m_Handle; }
    T*   operator->() const noexcept { return m_Handle; }
    bool operator==(const ARC& rhs) const noexcept { return m_Handle == rhs.m_Handle; }
    bool operator!=(const ARC& rhs) const noexcept { return m_Handle != rhs.m_Handle; }

    T* typedHandle() const noexcept
    {
      return m_Handle;
    }

    ~ARC()
    {
      release();
    }

    // IARCHandle Interface

    bool                     isValid() const noexcept override { return m_Handle != nullptr; }
    meta::BaseClassMetaInfo* typeInfo() const noexcept override { return meta::typeInfoGet<T>(); }
    void                     assign(IBaseAsset* asset) override { reassign(asset); }
    IBaseAsset*              handle() const noexcept override { return m_Handle; }

   private:
    void reassign(IBaseAsset* asset)
    {
      if (m_Handle != asset)
      {
        assert(!asset || asset->type() == typeInfo() && "Either must be assigning nullptr or the types must match.");

        release();
        m_Handle = static_cast<T*>(asset);
        acquire();
      }
    }

    void acquire() const
    {
      if (m_Handle)
      {
        m_Handle->acquire();
      }
    }

    // NOTE(SR):
    //   This function does not set 'm_Handle' to nullptr.
    //   That is because it is a redundant assignment to 'm_Handle', ex: copy assignment.
    void release() const
    {
      if (m_Handle)
      {
        m_Handle->release();
      }
    }
  };
}  // namespace bf

#endif /* BF_BASE_ASSET_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

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
