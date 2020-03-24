/*!
 * @file   bifrost_memory_utils.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   Contains functions useful for low level memory manipulations.   
 *
 * @version 0.0.1
 * @date    2020-03-22
 * 
 * @copyright Copyright (c) 2020
 */
#include "bifrost/memory/bifrost_memory_utils.h"

#include <assert.h> /* assert */

size_t alignedUpSize(size_t size, size_t required_alignment)
{
  assert(required_alignment > 0 && (required_alignment & (required_alignment - 1)) == 0 && "alignedUpSize:: The alignment must be a non-zero power of two.");

  return (size + required_alignment - 1) & ~(required_alignment - 1);
}
