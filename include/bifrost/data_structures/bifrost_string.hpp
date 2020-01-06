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

#include "bifrost_dynamic_string.h" /* ConstBifrostString */

#include <cstddef> /* size_t */

namespace bifrost
{
  class IMemoryManager;

  class String
  {
   public:
    String(const char*)
    {
    }
  };
}  // namespace bifrost

namespace bifrost::string_utils
{
  struct BifrostStringHasher final
  {
    std::size_t operator()(ConstBifrostString input) const
    {
      return bfString_hash(input);
    }
  };

  struct BifrostStringComparator final
  {
    bool operator()(ConstBifrostString lhs, ConstBifrostString rhs) const
    {
      return String_cmp(lhs, rhs) == 0;
    }
  };

  // NOTE(Shareef):
  //   The caller is responsible for freeing any memory this allocates.
  char* alloc_fmt(IMemoryManager& allocator, std::size_t* out, const char* fmt, ...);
}  // namespace bifrost::string_utils

#endif  // !BIFROST_STRING_HPP
