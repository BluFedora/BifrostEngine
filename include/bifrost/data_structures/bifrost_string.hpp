/*!
 * @file   bifrost_string.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   C++ Utilities for manipulating strings.
 *
 * @version 0.0.1
 * @date 2019-12-22
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_STRING_HPP
#define BIFROST_STRING_HPP

#include "bifrost/bifrost_std.h"    /* bfStringRange      */
#include "bifrost_dynamic_string.h" /* ConstBifrostString */

#include <cstddef>   /* size_t        */
#include <stdexcept> /* runtime_error */

namespace bifrost
{
  class IMemoryManager;

  struct StringRange : public bfStringRange
  {
    StringRange(const bfStringRange& rhs) :
      bfStringRange{rhs}
    {
    }

    StringRange(const char* bgn, const char* end) :
      bfStringRange{bgn, end}
    {
    }

    [[nodiscard]] std::size_t length() const
    {
      return end - bgn;
    }
  };

  class String final
  {
   private:
    BifrostString m_Handle;

   public:
    String() :
      m_Handle{nullptr}
    {
    }

    String(const char* data) :
      m_Handle{String_new(data)}
    {
    }

    String(const char* bgn, const char* end) :
      m_Handle{String_newLen(bgn, end - bgn)}
    {
    }

    String(const char* bgn, std::size_t length) :
      m_Handle{String_newLen(bgn, length)}
    {
    }

    String(const String& rhs) :
      m_Handle{rhs.m_Handle ? String_clone(rhs.m_Handle) : nullptr}
    {
    }

    String(String&& rhs) noexcept :
      m_Handle{rhs.m_Handle}
    {
      rhs.m_Handle = nullptr;
    }

    String& operator=(const String& rhs)
    {
      if (this != &rhs)
      {
        deleteHandle();
        m_Handle = rhs.m_Handle ? String_clone(rhs.m_Handle) : nullptr;
      }

      return *this;
    }

    String& operator=(String&& rhs) noexcept
    {
      if (this != &rhs)
      {
        deleteHandle();
        m_Handle     = rhs.m_Handle;
        rhs.m_Handle = nullptr;
      }

      return *this;
    }

    [[nodiscard]] bool operator==(const char* rhs) const
    {
      return m_Handle && String_ccmp(m_Handle, rhs) == 0;
    }

    [[nodiscard]] bool operator==(const String& rhs) const
    {
      return m_Handle && rhs.m_Handle && ::String_cmp(m_Handle, rhs.m_Handle) == 0;
    }

    [[nodiscard]] bool operator!=(const char* rhs) const
    {
      return !(*this == rhs);
    }

    [[nodiscard]] bool operator!=(const String& rhs) const
    {
      return !(*this == rhs);
    }

    operator StringRange() const
    {
      return {cstr(), cstr() + length()};
    }

    [[nodiscard]] BifrostString handle() const { return m_Handle; }

    void reserve(const std::size_t new_capacity)
    {
      if (!m_Handle)
      {
        m_Handle = String_new("");
      }

      ::String_reserve(&m_Handle, new_capacity);
    }

    void resize(const std::size_t new_size)
    {
      if (!m_Handle)
      {
        m_Handle = String_new("");
      }

      ::String_resize(&m_Handle, new_size);
    }

    [[nodiscard]] std::size_t length() const
    {
      return ::String_length(m_Handle);
    }

    [[nodiscard]] std::size_t capacity() const
    {
      return ::String_capacity(m_Handle);
    }

    [[nodiscard]] const char* cstr() const
    {
      return String_cstr(m_Handle);
    }

    void set(const char* str)
    {
      if (m_Handle)
      {
        String_cset(&m_Handle, str);
      }
      else
      {
        m_Handle = String_new(str);
      }
    }

    void append(const char character)
    {
      const char null_terminated_character[] = {character, '\0'};
      append(null_terminated_character, 1);
    }

    void append(const char* str)
    {
      if (m_Handle)
      {
        String_cappend(&m_Handle, str);
      }
      else
      {
        m_Handle = String_new(str);
      }
    }

    void append(const String& str)
    {
      append(str.cstr(), str.length());
    }

    void append(const char* str, const std::size_t len)
    {
      if (m_Handle)
      {
        String_cappendLen(&m_Handle, str, len);
      }
      else
      {
        m_Handle = String_newLen(str, len);
      }
    }

    void insert(const std::size_t index, const char* str)
    {
      if (m_Handle)
      {
        String_cinsert(&m_Handle, index, str);
      }
      else if (index == 0)
      {
        m_Handle = String_new(str);
      }
      else
      {
        throw std::runtime_error("Inserting into an empty string.");
      }
    }

    void unescape()
    {
      String_unescape(m_Handle);
    }

    [[nodiscard]] std::size_t hash() const
    {
      if constexpr (sizeof(std::size_t) == 4)
      {
        return bfString_hash(cstr());
      }
      else
      {
        return bfString_hash64(cstr());
      }
    }

    void clear()
    {
      ::String_clear(&m_Handle);
    }

    ~String()
    {
      deleteHandle();
    }

   private:
    void deleteHandle() const
    {
      if (m_Handle)
      {
        String_delete(m_Handle);
      }
    }
  };

  inline String operator+(const String& lhs, const char* rhs)
  {
    String result = lhs;
    result.append(rhs);
    return result;
  }

  inline String operator+(const StringRange& lhs, const String& rhs)
  {
    String result = {lhs.bgn, lhs.end};
    result.append(rhs);
    return result;
  }
}  // namespace bifrost

namespace std
{
  template<>
  struct hash<bifrost::String>
  {
    std::size_t operator()(bifrost::String const& s) const noexcept
    {
      return s.hash();
    }
  };
}  // namespace std

namespace bifrost::string_utils
{
  struct StringHasher final
  {
    std::size_t operator()(const ConstBifrostString input) const
    {
      return bfString_hash(input);
    }
  };

  struct StringComparator final
  {
    bool operator()(const ConstBifrostString lhs, const char* const rhs) const
    {
      return String_ccmp(lhs, rhs) == 0;
    }
  };

  // NOTE(Shareef):
  //   The caller is responsible for freeing any memory this allocates.
  //   Use 'IMemoryManager::deallcoate' or 'alloc_fmt_free'
  char* alloc_fmt(IMemoryManager& allocator, std::size_t* out, const char* fmt, ...);
  void  free_fmt(IMemoryManager& allocator, char* ptr);
}  // namespace bifrost::string_utils

#endif /* BIFROST_STRING_HPP */
