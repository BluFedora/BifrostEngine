/******************************************************************************/
/*!
 * @file   bf_hash.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Some hashing utilities for various data types.
 *
 * @version 0.0.1
 * @date 2019-12-28
 *
 * @copyright Copyright (c) 2019
 */
/******************************************************************************/
#include "bf/bf_hash.hpp"

#include <cstring>     // memcpy
#include <functional>  // hash

namespace
{
  // This is a very simple hash (same one that Java uses for String.hashCode())
  // It is NOT cryptographically secure but is fairly fast.
  template<typename TPredicate>
  bf::hash::Hash_t simple_hash_base(const char* p, TPredicate&& f)
  {
    static constexpr std::size_t PRIME  = 31;
    bf::hash::Hash_t             result = 0;

    while (f(p))
    {
      result = *p + result * PRIME;
      ++p;
    }

    return result;
  }
}  // namespace

namespace bf::hash
{
  //
  // NOTE(SR):
  //   Using GodBolt I have found that the compiler
  //   optimized the mod out and does the equivalent
  //   using other methods. (Dec 27th, 2020)
  //

  template<>
  std::uint8_t reducePointer<std::uint8_t>(const void* ptr)
  {
    using namespace LargestPrimeLessThanPo2;

    return std::uint8_t(std::uintptr_t(ptr)) % k_8Bit;
  }

  template<>
  std::uint16_t reducePointer<std::uint16_t>(const void* ptr)
  {
    using namespace LargestPrimeLessThanPo2;

    return std::uint16_t(std::uintptr_t(ptr)) % k_16Bit;
  }

  template<>
  std::uint32_t reducePointer<std::uint32_t>(const void* ptr)
  {
    using namespace LargestPrimeLessThanPo2;

    return std::uint32_t(std::uintptr_t(ptr)) % k_32Bit;
  }

  template<>
  std::uint64_t reducePointer<std::uint64_t>(const void* ptr)
  {
    using namespace LargestPrimeLessThanPo2;

    return std::uint64_t(std::uintptr_t(ptr)) % k_64Bit;
  }

  Hash_t simple(const char* p, std::size_t size)
  {
    return simple_hash_base(p, [end = p + size](const char* p) { return p != end; });
  }

  // (h * 0x100000001b3ull) ^ value

  Hash_t simple(const char* p)
  {
    return simple_hash_base(p, [](const char* p) { return *p != '\0'; });
  }

  // This is what boost::hash_combine does
  Hash_t combine(Hash_t lhs, Hash_t hashed_value)
  {
    // https://github.com/HowardHinnant/hash_append/issues/7
    if constexpr (sizeof(Hash_t) == 1)  // 8-bit
    {
      lhs ^= hashed_value + 0x9E + (lhs << 6) + (lhs >> 2);
    }

    if constexpr (sizeof(Hash_t) == 2)  // 16-bit
    {
      lhs ^= hashed_value + 0x9E37 + (lhs << 6) + (lhs >> 2);
    }

    if constexpr (sizeof(Hash_t) == 4)  // 32-bit
    {
      lhs ^= hashed_value + 0x9E3779B9 + (lhs << 6) + (lhs >> 2);
    }

    if constexpr (sizeof(Hash_t) == 8)  // 64-bit
    {
      lhs ^= hashed_value + 0x9E3779B97f4A7C15 + (lhs << 6) + (lhs >> 2);
    }

    return lhs;
  }

  Hash_t addString(Hash_t self, const char* p)
  {
    return combine(addU32(self, 0xFF), simple(p));
  }

  Hash_t addString(Hash_t self, const char* p, std::size_t size)
  {
    return addBytes(addU32(self, 0xFF), p, size);
  }

  Hash_t addBytes(Hash_t self, const char* p, std::size_t size)
  {
    return combine(self, simple(p, size));
  }

  Hash_t addU32(Hash_t self, std::uint32_t u32)
  {
    if constexpr (sizeof(Hash_t) == 4)
    {
      // TODO: Fix Me
      return (self * 0x100000001B3ull) ^ u32;
    }

    if constexpr (sizeof(Hash_t) == 8)
    {
      return combine(self, std::hash<std::uint32_t>()(u32));
    }

    // return self;
  }

  Hash_t addS32(Hash_t self, std::int32_t s32)
  {
    return addU32(self, std::uint32_t(s32));
  }

  Hash_t addU64(Hash_t self, std::uint64_t u64)
  {
    return combine(self, std::hash<std::uint64_t>()(u64));
  }

  Hash_t addS64(Hash_t self, std::int64_t s64)
  {
    return addU64(self, std::uint64_t(s64));
  }

  Hash_t addF32(Hash_t self, float f32)
  {
    static_assert(sizeof(float) == sizeof(std::uint32_t), "The sizes should match.");

    std::uint32_t u32;
    std::memcpy(&u32, &f32, sizeof(float));

    return addU32(self, u32);
  }

  Hash_t addPointer(Hash_t self, const void* ptr)
  {
    static constexpr std::hash<const void*> s_Hasher;

    if constexpr (sizeof(uintptr_t) == 4)
    {
      // NOTE: the cast is to appease the warning gods on 64bit compiles.
      // TODO: Fix Me
      return addU32(self, std::uint32_t(reinterpret_cast<uintptr_t>(ptr)));
    }

    if constexpr (sizeof(uintptr_t) == 8)
    {
      return combine(self, s_Hasher(ptr));
    }

    static_assert(sizeof(uintptr_t) == 4 || sizeof(uintptr_t) == 8, "Unsupported pointer size.");
    return self;
  }
}  // namespace bf::hash
