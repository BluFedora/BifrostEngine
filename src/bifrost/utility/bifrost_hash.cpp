#include "bifrost/utility/bifrost_hash.hpp"

namespace
{
  // This is a very simple hash (same one that Java uses for String.hashCode())
  // It is not cryptographically secure but is fairly fast.
  template<typename TPredicate>
  bifrost::hash::Hash_t simple_hash_base(const char* p, TPredicate&& f)
  {
    static constexpr std::size_t PRIME  = 31;
    bifrost::hash::Hash_t        result = 0;

    while (f(p))
    {
      result = *p + result * PRIME;
      ++p;
    }

    return result;
  }
}  // namespace

namespace bifrost::hash
{
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
      return (self * 0x100000001B3ull) ^ u32;
    }

    if constexpr (sizeof(Hash_t) == 8)
    {
      return combine(self, u32);
    }

    static_assert(sizeof(uintptr_t) == 4 || sizeof(uintptr_t) == 8, "Unsupported pointer size.");

    return self;
  }

  Hash_t addS32(Hash_t self, std::int32_t s32)
  {
    return addU32(self, std::uint32_t(s32));
  }

  Hash_t addU64(Hash_t self, std::uint64_t u64)
  {
    return addU32(addU32(self, std::uint32_t(u64 & 0xFFFFFFFF)), std::uint32_t((u64 >> 32) & 0xFFFFFFFF));
  }

  Hash_t addS64(Hash_t self, std::int64_t s64)
  {
    return addU64(self, std::uint64_t(s64));
  }

  Hash_t addF32(Hash_t self, float f32)
  {
    union
    {
      float         f32;
      std::uint32_t u32;
    } u;

    u.f32 = f32;

    return addU32(self, u.u32);
  }

  Hash_t addPointer(Hash_t self, const void* ptr)
  {
    if constexpr (sizeof(uintptr_t) == 4)
    {
      return addU32(self, reinterpret_cast<uintptr_t>(ptr));
    }

    if constexpr (sizeof(uintptr_t) == 8)
    {
      return addU64(self, reinterpret_cast<uintptr_t>(ptr));
    }

    static_assert(sizeof(uintptr_t) == 4 || sizeof(uintptr_t) == 8, "Unsupported pointer size.");
    return self;
  }
}  // namespace bifrost::hash
