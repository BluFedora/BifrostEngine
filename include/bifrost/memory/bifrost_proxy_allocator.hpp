/******************************************************************************/
/*!
  @file   proxy_allocator.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is not really an allocator at all but allows for more
      debugging opportunities if the occasion arises.
*/
/******************************************************************************/
#ifndef PROXY_ALLOCATOR_HPP
#define PROXY_ALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp" /* IMemoryManager */

namespace bifrost
{
  class ProxyAllocator : public IMemoryManager
  {
    private:
      IMemoryManager& m_Impl;

    public:
      ProxyAllocator(IMemoryManager& real_allocator);
      ~ProxyAllocator() = default;

    public:
      void* alloc(const std::size_t size, const std::size_t alignment)  override;
      void  dealloc(void* ptr)                                          override;

    public:
      static constexpr std::size_t header_size = 0u;

  };
}

#endif /* PROXY_ALLOCATOR_HPP */
