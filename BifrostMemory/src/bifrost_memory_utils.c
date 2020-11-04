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

#include <stdint.h> /* uintptr_t */
#include <assert.h> /* assert    */

#define bfCast(e, T) ((T)(e))

size_t bfAlignUpSize(size_t size, size_t required_alignment)
{
  assert(required_alignment > 0 && (required_alignment & (required_alignment - 1)) == 0 && "bfAlignUpSize:: The alignment must be a non-zero power of two.");

  const size_t required_alignment_minus_one = required_alignment - 1;

  return size + required_alignment_minus_one & ~required_alignment_minus_one;
}

void* bfAlignUpPointer(const void* ptr, size_t required_alignment)
{
  const size_t required_alignment_minus_one = required_alignment - 1;

  return bfCast(bfCast(ptr, uintptr_t) + required_alignment_minus_one & ~required_alignment_minus_one, void*);
}


/* Good read on the various implementations and performance. [https://github.com/KabukiStarship/KabukiToolkit/wiki/Fastest-Method-to-Align-Pointers#21-proof-by-example] */
void* bfStdAlign(size_t alignment, size_t size, void** ptr, size_t* space)
{
  assert(alignment > 0 && (alignment & (alignment - 1)) == 0 && "bfStdAlign:: The alignment must be a non-zero power of two.");
  assert(ptr && "Passed in pointer must not be null");
  assert(space && "Passed in space must not be null");

  void* const     aligned_ptr = bfAlignUpPointer(*ptr, alignment);
  const uintptr_t offset      = bfCast(aligned_ptr, char*) - bfCast(*ptr, char*);

  if (*space >= (size + offset))
  {
    *ptr = aligned_ptr;
    *space -= offset;

    return aligned_ptr;
  }

  return NULL;
}
