/******************************************************************************/
/*!
  @file   bifrost_proxy_allocator.hpp
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
  class ProxyAllocator final : public IMemoryManager
  {
   private:
    IMemoryManager& m_Impl;

   public:
    ProxyAllocator(IMemoryManager& real_allocator);

   public:
    void* alloc(std::size_t size, std::size_t alignment) override;
    void  dealloc(void* ptr) override;

   public:
    static constexpr std::size_t header_size = 0u;
  };

  class NoFreeAllocator final : public IMemoryManager
  {
   private:
    IMemoryManager& m_Impl;

   public:
    NoFreeAllocator(IMemoryManager& real_allocator);

   public:
    void* alloc(std::size_t size, std::size_t alignment) override;
    void  dealloc(void* ptr) override;

   public:
    static constexpr std::size_t header_size = 0u;
  };
}  // namespace bifrost

#endif /* PROXY_ALLOCATOR_HPP */
