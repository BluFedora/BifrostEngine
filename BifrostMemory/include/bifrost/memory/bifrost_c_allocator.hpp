/******************************************************************************/
/*!
* @file   bifrost_c_allocator.hpp
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
#ifndef BIFROST_C_ALLOCATOR_HPP
#define BIFROST_C_ALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp"

namespace bifrost
{
  class CAllocator final : public IMemoryManager
  {
   public:
    CAllocator();

   public:
    void* allocate(std::size_t size) override;
    void  deallocate(void* ptr) override;

    static constexpr std::size_t header_size = 0u;
  };
}  // namespace bifrost

#endif  /* BIFROST_C_ALLOCATOR_HPP */
