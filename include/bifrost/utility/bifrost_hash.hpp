#ifndef BIFROST_HASH_HPP
#define BIFROST_HASH_HPP

#include <cstddef> /* size_t */

namespace bifrost::hash
{
  std::size_t simple(const char* p, std::size_t size);
  std::size_t simple(const char* p);
  std::size_t combine(std::size_t lhs, std::size_t hashed_value);
}  // namespace bifrost::hash

#endif /* BIFROST_HASH_HPP */
