/******************************************************************************/
/*!
 * @file   bf_dense_map_handle.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Strongly typed wrapper around an integer handle for use in the DenseMap<T>.
 *   
 *   Inspired By:
 *     [http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html]
 *
 * @version 0.0.1
 * @date    2019-12-27
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_DENSE_MAP_HANDLE_HPP
#define BF_DENSE_MAP_HANDLE_HPP

#include <cstdint> /* uint8_t, uint16_t, uint32_t, uint16_t */

namespace bf
{
  template<std::size_t k_NumBits>
  struct SmallestUIntToHoldBitsImpl; /* = undefined */

  // clang-format off
  template<>
  struct SmallestUIntToHoldBitsImpl<8> { using type = std::uint8_t; };
  template<>
  struct SmallestUIntToHoldBitsImpl<16> { using type = std::uint16_t; };
  template<>
  struct SmallestUIntToHoldBitsImpl<32> { using type = std::uint32_t; };
  template<>
  struct SmallestUIntToHoldBitsImpl<64> { using type = std::uint64_t; };
  // clang-format on

  template<std::size_t kValue, std::size_t Size>
  static constexpr std::size_t smallestSize()
  {
    return Size;
  }

  template<std::size_t kValue, std::size_t Size, std::size_t FirstRestSizes, std::size_t... RestSizes>
  static constexpr std::size_t smallestSize()
  {
    return kValue <= Size ? Size : smallestSize<kValue, FirstRestSizes, RestSizes...>();
  }

  template<std::size_t k_NumBits>
  using SmallestUIntToHoldBits = std::uint32_t;   // typename SmallestUIntToHoldBitsImpl<smallestSize<k_NumBits, 8, 16, 32, 64>()>::type;

  template<
   typename TObject_,
   std::size_t k_NumGenerationBits,  // = 32 - 20,
   std::size_t k_NumIndexBits        // = 20
   >
  union DenseMapHandle
  {
    using TObject     = TObject_;
    using THandleType = SmallestUIntToHoldBits<k_NumGenerationBits + k_NumIndexBits>;
    using TIndexType  = SmallestUIntToHoldBits<k_NumIndexBits>;

    static constexpr std::size_t NumGenerationBits = k_NumGenerationBits;
    static constexpr std::size_t NumIndexBits      = k_NumIndexBits;
    static constexpr THandleType MaxObjects        = (1ull << NumIndexBits);
    static constexpr TIndexType  InvalidIndex      = MaxObjects - 1u;  // Also acts as mask to get the index.

    struct
    {
      THandleType generation : NumGenerationBits;
      THandleType index : NumIndexBits;
    };
    THandleType handle;  //!< This contains the unique id, the bottom bits contain the index into the m_SparseIndices array.

    DenseMapHandle(const THandleType id = InvalidIndex) :
      handle(id)
    {
    }

    [[nodiscard]] bool operator==(const DenseMapHandle& rhs) const { return handle == rhs.handle; }
    [[nodiscard]] bool operator!=(const DenseMapHandle& rhs) const { return handle != rhs.handle; }
    [[nodiscard]] bool isValid() const { return handle != InvalidIndex; }
  };
}  // namespace bf

#endif /* BF_DENSE_MAP_HANDLE_HPP */

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
