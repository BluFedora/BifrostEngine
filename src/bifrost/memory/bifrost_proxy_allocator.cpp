/******************************************************************************/
/*!
  @file   proxy_allocator.cpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is not really an allocator at all but allows for more
      debugging opportunities if the occasion arises.
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_proxy_allocator.hpp"

namespace bifrost
{
  ProxyAllocator::ProxyAllocator(IMemoryManager& real_allocator) :
    m_Impl(real_allocator)
  {
  }

  void* ProxyAllocator::alloc(const std::size_t size, const std::size_t alignment)
  {
    return m_Impl.alloc(size, alignment);
  }

  void ProxyAllocator::dealloc(void* ptr)
  {
    m_Impl.dealloc(ptr);
  }

  NoFreeAllocator::NoFreeAllocator(IMemoryManager& real_allocator) :
    m_Impl(real_allocator)
  {
  }

  void* NoFreeAllocator::alloc(const std::size_t size, const std::size_t alignment)
  {
    return m_Impl.alloc(size, alignment);
  }

  void NoFreeAllocator::dealloc(void* ptr)
  {
    (void)ptr;
    // m_Impl.dealloc(ptr);
  }
}
