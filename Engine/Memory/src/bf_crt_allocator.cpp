/******************************************************************************/
/*!
* @file   bifrost_c_allocator.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*  This allocator is a wrapper around the built in memory allocator.
*  Implemented using "malloc / calloc" and "free".
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019
*/
/******************************************************************************/
#include "bf/memory/bf_crt_allocator.hpp"

#include <cstdlib> /* calloc, malloc, free */

#if 0
#include <new>

void* ::operator new(std::size_t size)
{
  void* const ptr = malloc(size);

  if (!ptr)
  {
    throw std::bad_alloc{};
  }

  return ptr;
}

void* ::operator new(std::size_t size, const std::nothrow_t&) noexcept
{
  try
  {
    return malloc(size);
  }
  catch (...)
  {
  }

  return nullptr;
}

void* ::operator new[](std::size_t size)
{
  return malloc(size);
}

void* ::operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
  try
  {
    return malloc(size);
  }
  catch (...)
  {
  }

  return nullptr;
}

void ::operator delete(void* ptr)
{
  free(ptr);
}

void ::operator delete(void* ptr, const std::nothrow_t&) noexcept
{
  free(ptr);
}

void ::operator delete[](void* ptr)
{
  free(ptr);
}

void ::operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
  free(ptr);
}
#endif

namespace bf
{
  CAllocator::CAllocator() :
    IMemoryManager()
  {
  }

  void* CAllocator::allocate(std::size_t size)
  {
#if 0
    return std::calloc(1, size);
#else
    return std::malloc(size);
#endif
  }

  void CAllocator::deallocate(void* ptr, std::size_t /* num_bytes */)
  {
    std::free(ptr);
  }
}  // namespace bf
