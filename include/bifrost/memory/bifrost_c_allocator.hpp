/******************************************************************************/
/*!
  @file   bifrost_c_allocator.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is a wrapper around the built in memory allocator.
      Implemented using "malloc / calloc" and "free".
*/
/******************************************************************************/
#ifndef CALLOCATOR_HPP
#define CALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp"

namespace bifrost
{
  class CAllocator : public IMemoryManager
  {
    public:
      CAllocator(void);

    public:
      void* alloc(const std::size_t size, const std::size_t alignment)  override;
      void  dealloc(void* ptr)                                          override;

    public:
      static constexpr std::size_t header_size = 0u;

  };
}

#endif // CALLOCATOR_HPP
