#ifndef BIFROST_HASH_HPP
#define BIFROST_HASH_HPP

#include <cstddef> /* size_t   */
#include <cstdint> /* uint64_t */

namespace bifrost::hash
{
  using Hash_t = std::uint64_t;

  Hash_t simple(const char* p, std::size_t size);
  Hash_t simple(const char* p);
  Hash_t combine(Hash_t lhs, Hash_t hashed_value);

  Hash_t addString(Hash_t self, const char* p);
  Hash_t addString(Hash_t self, const char* p, std::size_t size);
  Hash_t addBytes(Hash_t self, const char* p, std::size_t size);
  Hash_t addU32(Hash_t self, std::uint32_t u32);
  Hash_t addS32(Hash_t self, std::int32_t s32);
  Hash_t addU64(Hash_t self, std::uint64_t u64);
  Hash_t addS64(Hash_t self, std::int64_t s64);
  Hash_t addF32(Hash_t self, float f32);
  Hash_t addPointer(Hash_t self, const void* ptr);
}  // namespace bifrost::hash

#endif /* BIFROST_HASH_HPP */
