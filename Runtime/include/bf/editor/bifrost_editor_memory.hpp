/******************************************************************************/
/*!
* @file   bifrost_editor_memory.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-07-05
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_EDITOR_MEMORY_HPP
#define BIFROST_EDITOR_MEMORY_HPP

#include "bf/IMemoryManager.hpp"          /* IMemoryManager            */
#include "bf/bf_meta_function_traits.hpp" /*  meta::function_caller<F> */

#include <memory> /* unique_ptr<T> */

namespace bf::editor
{
  IMemoryManager& allocator();

  template<typename T, typename... Args>
  T* make(Args&&... args)
  {
    return allocator().allocateT<T>(std::forward<Args&&>(args)...);
  }

  template<typename T>
  void deallocateT(T* ptr)
  {
    allocator().deallocateT(ptr);
  }

  template<typename T>
  using UniquePtr = std::unique_ptr<T, meta::function_caller<&deallocateT<T>>>;
}  // namespace bf::editor

#endif /* BIFROST_EDITOR_MEMORY_HPP */
