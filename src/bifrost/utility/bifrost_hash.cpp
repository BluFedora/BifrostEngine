#include "bifrost/utility/bifrost_hash.hpp"

namespace
{
  // This is a very simple hash (same one that Java uses for String.hashCode())
  // It is not cryptographically secure but is fairly fast.
  template<typename TPredicate>
  std::size_t simple_hash_base(const char* p, TPredicate&& f)
  {
    static constexpr std::size_t PRIME  = 31;
    std::size_t                  result = 0;

    while(f(p))
    {
      result = *p + result * PRIME;
      ++p;
    }

    return result;
  }
}

namespace bifrost::hash
{
  std::size_t simple(const char* p, size_t size)
  {
    return simple_hash_base(p, [end = p + size](const char* p) { return p != end; });
  }

  std::size_t simple(const char* p)
  {
    return simple_hash_base(p, [](const char* p) { return *p != '\0'; });
  }

  // This is what boost::hash_combine does
  std::size_t combine(std::size_t lhs, std::size_t hashed_value)
  {
    // https://github.com/HowardHinnant/hash_append/issues/7
    if constexpr (sizeof(std::size_t) == 1)  // 8-bit
    {
      lhs ^= hashed_value + 0x9E + (lhs << 6) + (lhs >> 2);
    }

    if constexpr (sizeof(std::size_t) == 2)  // 16-bit
    {
      lhs ^= hashed_value + 0x9E37 + (lhs << 6) + (lhs >> 2);
    }

    if constexpr (sizeof(std::size_t) == 4)  // 32-bit
    {
      lhs ^= hashed_value + 0x9E3779B9 + (lhs << 6) + (lhs >> 2);
    }

    if constexpr (sizeof(std::size_t) == 8)  // 64-bit
    {
      lhs ^= hashed_value + 0x9E3779B97f4A7C15 + (lhs << 6) + (lhs >> 2);
    }

    return lhs;
  }
}
