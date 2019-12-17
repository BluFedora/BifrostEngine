/******************************************************************************/
/*!
  @file   imemory_manager.cpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      Implements a basic interface for the various types of memory managers.
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_imemory_manager.hpp"

namespace bifrost
{
  MemoryManager::MemoryManager(char* memory_block, const std::size_t memory_block_size) :
    IMemoryManager(),
    m_MemoryBlockBegin(memory_block),
    m_MemoryBlockEnd(memory_block + memory_block_size)
  {
#if DEBUG_MEMORY
    //std::memset(begin(), DEBUG_MEMORY_SIGNATURE, memory_block_size);
#endif
  }
}
