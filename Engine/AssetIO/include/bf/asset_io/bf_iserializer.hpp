/******************************************************************************/
/*!
 * @file   bf_iserializer.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Generic interface for serializing and reflecting over various data types.
 *
 * @version 0.0.1
 * @date    2020-12-20
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_ISERIALIZER_HPP
#define BF_ISERIALIZER_HPP

#include "bf/Quaternion.h"                   // Quaternionf
#include "bf/StringRange.hpp"                // StringRange
#include "bf/bifrost_math.h"                 // bfColor4f, bfColor4u
#include "bf/math/bifrost_rect2.hpp"         // Math Types
#include "bf/meta/bifrost_meta_variant.hpp"  // MetaVariant
#include "bf/utility/bifrost_uuid.h"         // bfUUID, bfUUIDNumber

namespace bf
{
  // Forward Declarations

  namespace meta
  {
    struct MetaObject;
  }

  class IBaseObject;
  class EntityRef;
  class IARCHandle;

  enum class SerializerMode
  {
    LOADING,
    SAVING,
    INSPECTING,
  };

  class ISerializer
  {
   private:
    SerializerMode m_Mode;

   protected:
    explicit ISerializer(SerializerMode mode) :
      m_Mode{mode}
    {
    }

   public:
    SerializerMode mode() const { return m_Mode; }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // API / Implementation Notes:
    //   * If you are within an Array all 'StringRange key' parameters are ignored,
    //     as a result of this condition you may pass in nullptr.
    //       > An implementation is allowed to do something special with the key
    //         if it is not nullptr though.
    //
    //   * The return value in pushArray's "std::size_t& size" is only useful for
    //     SerializerMode::LOADING. Otherwise you are going to receive 0.
    //
    //   * Scopes for 'pushObject' and 'pushArray' are only valid if they return true.
    //     Only call 'popObject' and 'popArray' only if 'pushXXX' returned true.
    //
    //   * Only begin reading the document if 'beginDocument' returns true.
    //
    ////////////////////////////////////////////////////////////////////////////////

    virtual bool beginDocument(bool is_array = false) = 0;
    virtual bool hasKey(StringRange key);
    virtual bool pushObject(StringRange key)                   = 0;
    virtual bool pushArray(StringRange key, std::size_t& size) = 0;
    virtual void serialize(StringRange key, std::byte& value) { serialize(key, reinterpret_cast<std::uint8_t&>(value)); }
    virtual void serialize(StringRange key, bool& value)          = 0;
    virtual void serialize(StringRange key, std::int8_t& value)   = 0;
    virtual void serialize(StringRange key, std::uint8_t& value)  = 0;
    virtual void serialize(StringRange key, std::int16_t& value)  = 0;
    virtual void serialize(StringRange key, std::uint16_t& value) = 0;
    virtual void serialize(StringRange key, std::int32_t& value)  = 0;
    virtual void serialize(StringRange key, std::uint32_t& value) = 0;
    virtual void serialize(StringRange key, std::int64_t& value)  = 0;
    virtual void serialize(StringRange key, std::uint64_t& value) = 0;
    virtual void serialize(StringRange key, float& value)         = 0;
    virtual void serialize(StringRange key, double& value)        = 0;
    virtual void serialize(StringRange key, long double& value)   = 0;
    virtual void serialize(StringRange key, Vec2f& value);
    virtual void serialize(StringRange key, Vec3f& value);
    virtual void serialize(StringRange key, Quaternionf& value);
    virtual void serialize(StringRange key, bfColor4f& value);
    virtual void serialize(StringRange key, bfColor4u& value);
    virtual void serialize(StringRange key, Rect2f& value);
    virtual void serialize(StringRange key, String& value) = 0;
    virtual void serialize(StringRange key, bfUUIDNumber& value);
    virtual void serialize(StringRange key, bfUUID& value);
    virtual void serialize(StringRange key, IARCHandle& value) = 0;
    virtual void serialize(StringRange key, EntityRef& value)  = 0;
    virtual void serialize(StringRange key, IBaseObject& value);
    virtual void serialize(IBaseObject& value);
    virtual void serialize(StringRange key, meta::MetaObject& value);
    virtual void serialize(meta::MetaObject& value);
    virtual void serialize(StringRange key, meta::MetaVariant& value);
    virtual void serialize(meta::MetaVariant& value);
    virtual void popObject()   = 0;
    virtual void popArray()    = 0;
    virtual void endDocument() = 0;

    // Helpers

    void serialize(StringRange key, Vector2f& value);
    void serialize(StringRange key, Vector3f& value);

    template<typename T>
    void serializeT(StringRange key, T* value)
    {
      if (pushObject(key))
      {
        serializeT(value);

        popObject();
      }
    }

    template<typename T>
    void serializeT(T* value)
    {
      auto variant = meta::makeVariant(value);

      serialize(variant);
    }

    virtual ~ISerializer() = default;
  };
}  // namespace bf

#endif /* BF_ISERIALIZER_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020 Shareef Abdoul-Raheem

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
