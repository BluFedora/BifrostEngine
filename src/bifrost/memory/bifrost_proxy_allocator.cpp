/******************************************************************************/
/*!
  @file   bifrost_proxy_allocator.cpp
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

  void* ProxyAllocator::allocate(std::size_t size)
  {
    return m_Impl.allocate(size);
  }

  void ProxyAllocator::deallocate(void* ptr)
  {
    m_Impl.deallocate(ptr);
  }

  NoFreeAllocator::NoFreeAllocator(IMemoryManager& real_allocator) :
    m_Impl(real_allocator)
  {
  }

  void* NoFreeAllocator::allocate(const std::size_t size)
  {
    return m_Impl.allocate(size);
  }

  void NoFreeAllocator::deallocate(void* ptr)
  {
    (void)ptr;
    // m_Impl.dealloc(ptr);
  }
}
