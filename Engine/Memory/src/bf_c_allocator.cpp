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
#include "bifrost/memory/bifrost_c_allocator.hpp"

#include <cstdlib> /* calloc, malloc, free */

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
}  // namespace bifrost
