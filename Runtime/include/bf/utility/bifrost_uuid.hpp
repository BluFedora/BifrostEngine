/******************************************************************************/
/*!
 * @file   bf_uuid.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   C++ Helpers for data-structures for the UUID module.
 *
 * @version 0.0.1
 * @date    2020-06-24
 *
 * @copyright Copyright (c) 2020-2021
 */
/******************************************************************************/
#ifndef BF_UUID_HPP
#define BF_UUID_HPP

#include "bf/data_structures/bifrost_dynamic_string.h" /* bfString_hashN, bfString_hashN64 */
#include "bifrost_uuid.h"                              /* UUID API                         */

#include <cstddef> /* size_t */

namespace bf
{
  struct UUIDHasher final
  {
    // ReSharper disable CppUnreachableCode

    static inline std::size_t pointerSizedHash(const char* bytes, std::size_t num_bytes)
    {
      if constexpr (sizeof(std::size_t) == 4)
      {
        return bfString_hashN(bytes, num_bytes);
      }
      else
      {
        return bfString_hashN64(bytes, num_bytes);
      }
    }

    // ReSharper restore CppUnreachableCode

    std::size_t operator()(const bfUUIDNumber& as_number) const { return pointerSizedHash(as_number.bytes, k_bfUUIDNumberSize); }
    std::size_t operator()(const bfUUIDString& as_string) const { return pointerSizedHash(as_string.data, k_bfUUIDStringLength); }
    std::size_t operator()(const bfUUID& uuid) const { return (*this)(uuid.as_number); }
  };

  struct UUIDEqual final
  {
    bool operator()(const bfUUIDNumber& lhs, const bfUUIDNumber& rhs) const { return bfUUID_numberCmp(&lhs, &rhs) != 0; }
    bool operator()(const bfUUIDString& lhs, const bfUUIDString& rhs) const { return bfUUID_stringCmp(&lhs, &rhs) != 0; }
    bool operator()(const bfUUID& lhs, const bfUUID& rhs) const { return bfUUID_isEqual(&lhs, &rhs) != 0; }
  };
}  // namespace bf

#endif /* BF_UUID_HPP */

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
