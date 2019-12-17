/******************************************************************************/
/*!
  @file   c_allocator.cpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is a wrapper around the built in memory allocator.
      Implemented using "malloc / calloc" and "free" respectively.
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_c_allocator.hpp"

#include <cstdlib> /* calloc, malloc, free */

namespace bifrost
{
  CAllocator::CAllocator(void) :
    IMemoryManager()
  {
  }

  void* CAllocator::alloc(const std::size_t size, const std::size_t alignment)
  {
    (void) alignment;
#ifdef DEBUG_MEMORY
    return std::calloc(1, size);
#else
    return std::malloc(size);
#endif
  }

  void CAllocator::dealloc(void* ptr)
  {
    std::free(ptr);
  }
}
