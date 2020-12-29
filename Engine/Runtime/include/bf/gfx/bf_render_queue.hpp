//
// Shareef Abdoul-Raheem
//
#ifndef BF_RENDER_QUEUE_HPP
#define BF_RENDER_QUEUE_HPP

#include <cassert>     /* assert         */
#include <cstdint>     /* uint64_t       */
#include <limits>      /* numeric_limits */
#include <type_traits> /* is_unsigned_v  */

#include <iostream>

// https://www.fluentcpp.com/2016/12/08/strong-types-for-strong-interfaces/

namespace bf
{
  //
  // Tag type for the bit manipulation functions.
  //
  template<std::size_t k_Offset, std::size_t k_NumBits>
  struct BitRange
  {
  };

  namespace Bits
  {
    //
    // All of these functions return the new values and do not manipulate it's input.
    //

    template<typename T, std::size_t k_NumBits>
    constexpr T max_value() noexcept
    {
      constexpr int k_MaxNumBitsInT = std::numeric_limits<T>::digits;

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_NumBits <= k_MaxNumBitsInT, "The num bits overflows the range of T.");

      return T((1 << k_NumBits) - 1);
    }

    template<typename T, std::size_t k_Offset, std::size_t k_NumBits>
    constexpr T mask(BitRange<k_Offset, k_NumBits>) noexcept
    {
      constexpr int         k_MaxNumBitsInT  = std::numeric_limits<T>::digits;
      constexpr std::size_t k_LastBitInRange = k_Offset + k_NumBits;

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_LastBitInRange <= k_MaxNumBitsInT, "The range overflows the range of T.");

      return max_value<T, k_NumBits>() << k_Offset;
    }

    // Clears the bits to 0 in the specified range.
    template<typename T, std::size_t k_Offset, std::size_t k_NumBits>
    T cleared(T bits, BitRange<k_Offset, k_NumBits> range) noexcept
    {
      constexpr int         k_MaxNumBitsInT  = std::numeric_limits<T>::digits;
      constexpr std::size_t k_LastBitInRange = k_Offset + k_NumBits;
      constexpr T           k_Mask           = mask<T>(range);

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_LastBitInRange <= k_MaxNumBitsInT, "The range overflows the range of T.");

      return bits & ~k_Mask;
    }

    // This function does NOT clear the range of what was previously there
    // Use 'Bits::clearAndSet' for that purpose.
    template<typename T, typename U, std::size_t k_Offset, std::size_t k_NumBits>
    T set(T bits, U value, BitRange<k_Offset, k_NumBits> range) noexcept
    {
      constexpr int         k_MaxNumBitsInT  = std::numeric_limits<T>::digits;
      constexpr std::size_t k_LastBitInRange = k_Offset + k_NumBits;
      constexpr T           k_Mask           = mask<T>(range);

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_LastBitInRange <= k_MaxNumBitsInT, "The range overflows the range of T.");

      // NOTE(SR):
      //   This static_assert is annoying to have since by default all integers are signed in C/++.
      // static_assert(std::is_unsigned_v<U>, "U must be an unsigned arithmetic type.");

      assert((T(value) <= max_value<T, k_NumBits>()) && "Value out of range.");

      return bits | ((static_cast<std::size_t>(value) << k_Offset) & k_Mask);
    }

    // Sets the values to the specified range in bits making sure to clear the range prior.
    template<typename T, typename U, std::size_t k_Offset, std::size_t k_NumBits>
    T cleared_set(T bits, U value, BitRange<k_Offset, k_NumBits> range) noexcept
    {
      return set(cleared(bits, range), value, range);
    }

    template<typename To, typename From>
    To cast(const From& from) noexcept
    {
      static_assert(sizeof(From) == sizeof(To), "The two types must but the same size.");
      static_assert(std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>, "memcpy is used to copy.");
      static_assert(std::is_trivially_constructible_v<To>, "'To' must be constructable without side effects.");

      To result;

      std::memcpy(&result, &from, sizeof(To));

      return result;
    }

    inline std::uint32_t hi_bits(float value, int num_hi_bits) noexcept
    {
      constexpr std::uint32_t k_NumBitsInFloat = 32;
      constexpr std::uint32_t k_HiBitIndex     = 31;
      constexpr std::uint32_t k_HiBit          = (1u << k_HiBitIndex);

      assert(std::uint32_t(num_hi_bits) <= k_NumBitsInFloat);

      const std::uint32_t float_bits = cast<std::uint32_t>(value);

      // Handle negative numbers, making it correctly sortable.
      // Adapted from [http://stereopsis.com/radix.html]

      const std::uint32_t float_flip_mask = -std::int32_t(float_bits >> k_HiBitIndex) | k_HiBit;

      return (float_bits ^ float_flip_mask) >> (k_NumBitsInFloat - num_hi_bits);
    }

    //
    // For 64bit pointers only 47 of the bits are used
    // so for sorting we get most of the important bits so this is good enough.
    //
    inline std::uint32_t basic_pointer_hash(const void* value)
    {
      constexpr int k_NumBitsInPtr     = std::numeric_limits<std::uintptr_t>::digits;
      constexpr int k_HalfNumBitsInPtr = k_NumBitsInPtr / 2;

      const std::uintptr_t value_bits = cast<std::uintptr_t>(value);

      return static_cast<std::uint32_t>(value_bits ^ (value_bits >> k_HalfNumBitsInPtr));
    }
  }  // namespace Bits

  struct RenderSortKey
  {
    std::uint64_t value;
  };

  struct RenderItem
  {
    RenderSortKey sort_key;
    RenderItem*   next_in_ground;
  };

  inline void TestStuff()
  {
    std::uint64_t value = 0x0;

    std::cout << std::showbase;

    std::cout << std::hex << value << '\n';

    value = Bits::set(value, 0xF0FF, BitRange<16, 16>{});

    std::cout << std::hex << value << '\n';

    value = Bits::cleared(value, BitRange<16, 16>{});

    std::cout << std::hex << value << '\n';

    value = Bits::cleared_set(value, 0xABEF, BitRange<16, 16>{});

    std::cout << std::hex << value << '\n';

    int* heap_ptr = new int();

    std::cout << "Hashed Stack Pointer: " << &value << " => " << Bits::basic_pointer_hash(&value) << "\n";
    std::cout << "Hashed Heap  Pointer: " << heap_ptr << " => " << Bits::basic_pointer_hash(heap_ptr) << "\n";

    delete heap_ptr;
  }
}  // namespace bf

#endif /* BF_RENDER_QUEUE_HPP */
