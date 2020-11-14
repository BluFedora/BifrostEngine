/******************************************************************************/
/*!
 * @file   bifrost_uuid.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   C++ Helpers for data-structures for the Bifrost UUID module.
 *
 * @version 0.0.1
 * @date    2020-06-24
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BIFROST_UUID_HPP
#define BIFROST_UUID_HPP

#include "bf/data_structures/bifrost_dynamic_string.h" /* bfString_hashN, bfString_hashN64 */
#include "bifrost_uuid.h"                              /* Biforst UUID API                 */

#include <cstddef> /* size_t */

namespace bf
{
  struct UUIDHasher final
  {
    // ReSharper disable CppUnreachableCode
    std::size_t operator()(const bfUUID& uuid) const
    {
      if constexpr (sizeof(std::size_t) == 4)
      {
        return bfString_hashN(uuid.as_number.data, sizeof(uuid.as_number));
      }
      else
      {
        return bfString_hashN64(uuid.as_number.data, sizeof(uuid.as_number));
      }
    }

    std::size_t operator()(const bfUUIDNumber& as_number) const
    {
      if constexpr (sizeof(std::size_t) == 4)
      {
        return bfString_hashN(as_number.data, sizeof(as_number));
      }
      else
      {
        return bfString_hashN64(as_number.data, sizeof(as_number));
      }
    }

    std::size_t operator()(const BifrostUUIDString& as_string) const
    {
      if constexpr (sizeof(std::size_t) == 4)
      {
        return bfString_hashN(as_string.data, sizeof(as_string) - 1);
      }
      else
      {
        return bfString_hashN64(as_string.data, sizeof(as_string) - 1);
      }
    }
    // ReSharper restore CppUnreachableCode
  };

  struct UUIDEqual final
  {
    bool operator()(const bfUUID& lhs, const bfUUID& rhs) const
    {
      return bfUUID_isEqual(&lhs, &rhs) != 0;
    }

    bool operator()(const bfUUIDNumber& lhs, const bfUUIDNumber& rhs) const
    {
      return bfUUID_numberCmp(&lhs, &rhs) != 0;
    }

    bool operator()(const BifrostUUIDString& lhs, const BifrostUUIDString& rhs) const
    {
      return bfUUID_stringCmp(&lhs, &rhs) != 0;
    }
  };
}  // namespace bf

#endif /* BIFROST_UUID_HPP */
