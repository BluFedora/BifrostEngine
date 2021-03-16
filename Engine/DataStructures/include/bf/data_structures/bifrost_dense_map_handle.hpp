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
  struct SelectUIntXBits
  {
    //
    // A similar problem if you wanted to check smallest int to for a given value
    // (this was not on this just was interesting to read)
    // [https://stackoverflow.com/questions/7038797/automatically-pick-a-variable-type-big-enough-to-hold-a-specified-number]
    //

    template<std::size_t k_NumBits2>
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
      static_assert(kValue <= Size, "The number of bits will not fit in any integer.");
      return Size;
    }

    template<std::size_t kValue, std::size_t Size, std::size_t FirstRestSizes, std::size_t... RestSizes>
    static constexpr std::size_t smallestSize()
    {
      return kValue <= Size ? Size : smallestSize<kValue, FirstRestSizes, RestSizes...>();
    }

    using type = typename SmallestUIntToHoldBitsImpl<smallestSize<k_NumBits, 8, 16, 32, 64>()>::type;
  };

  template<
   typename TObject_,
   std::size_t k_NumGenerationBits,
   std::size_t k_NumIndexBits>
  union DenseMapHandle
  {
    static constexpr std::size_t NumGenerationBits = k_NumGenerationBits;
    static constexpr std::size_t NumIndexBits      = k_NumIndexBits;

    using TObject     = TObject_;
    using THandleType = typename SelectUIntXBits<NumGenerationBits + NumIndexBits>::type;
    using TIndexType  = typename SelectUIntXBits<NumIndexBits>::type;

    static constexpr THandleType MaxObjects   = (1ull << NumIndexBits);
    static constexpr TIndexType  InvalidIndex = MaxObjects - 1u;  // Also acts as mask to get the index.

    struct
    {
      THandleType generation : NumGenerationBits; //!< Tries to detect use after free errors.
      THandleType index : NumIndexBits; //!< Indexes into sparse table.
    };
    THandleType handle;  //!< This contains the unique id, index into the sparse array table.

    DenseMapHandle(const THandleType idx = InvalidIndex) :
      generation(0),
      index{idx}
    {
    }

    [[nodiscard]] bool operator==(const DenseMapHandle& rhs) const { return handle == rhs.handle; }
    [[nodiscard]] bool operator!=(const DenseMapHandle& rhs) const { return handle != rhs.handle; }
    [[nodiscard]] bool isValid() const { return index != InvalidIndex; }
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
