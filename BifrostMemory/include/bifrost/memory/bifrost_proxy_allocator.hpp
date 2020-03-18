/******************************************************************************/
/*!
* @file   bifrost_proxy_allocator.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   This allocator is not really an allocator at all but allows for more
*   debugging opportunities when the occasions arise.
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019
*/
/******************************************************************************/
#ifndef BIFROST_PROXY_ALLOCATOR_HPP
#define BIFROST_PROXY_ALLOCATOR_HPP

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
    void* allocate(std::size_t size) override;
    void  deallocate(void* ptr) override;

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
    void* allocate(std::size_t size) override;
    void  deallocate(void* ptr) override;

   public:
    static constexpr std::size_t header_size = 0u;
  };
}  // namespace bifrost

#endif /* BIFROST_PROXY_ALLOCATOR_HPP */