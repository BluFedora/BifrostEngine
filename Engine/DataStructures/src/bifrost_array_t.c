/******************************************************************************/
/*!
 * @file   bf_array_t.c
 * @author Shareef Abdoul-Raheem
 * @brief
 *   Part of the "BluFedora Data Structures C Library"
 *   This is a generic dynamic array class.
 * 
 *   No dependencies besides the C Standard Library.
 *   Random Access - O(1)
 *   Pop           - O(1)
 *   Push, Emplace - O(1) best O(n) worst (when we need to grow)
 *   Clear         - O(1)
 * 
 *   The memory layout:
 *     [ArrayHeader (capacity | size | stride)][alignment-padding][allocation-offset]^[array-data (uint8_t*)]
 *     The '^' symbol indicates the pointer you are handed back from the 'bfArray_new' function.
 * 
 *   Memory Allocation Customization:
 *     To use your own allocator pass an allocation function to the array.
 *     To be compliant you must:
 *       > Act as malloc  when ptr == NULL, size == number of bytes to alloc.
 *       > Act as free    when ptr != NULL, size == number of bytes given back.
 * 
 * @copyright Copyright (c) 2019-2020
*/
/******************************************************************************/
#include "bf/data_structures/bifrost_array_t.h"

#include <assert.h> /* assert             */
#include <stdint.h> /* uint8_t, uintptr_t */
#include <stdlib.h> /* qsort, bsearch     */
#include <string.h> /* memcpy, memcmp     */

typedef struct
{
  size_t      stride;
  const void* key;

} ArrayDefaultCompareData;

typedef uint8_t AllocationOffset_t;

typedef struct
{
  bfArrayAllocator   allocator;
  void*              user_data;
  size_t             size;
  size_t             capacity;
  uint32_t           stride;
  AllocationOffset_t alignment;

} ArrayHeader;

#define bfAssert(arg, msg) assert((arg) && (msg))
#define bfCastByte(arr) ((unsigned char*)(arr))

// Helpers
static void*              alignUpPtr(uintptr_t element_size, uintptr_t element_alignment);
static AllocationOffset_t grabOffset(const void** self);

#if 0
static ArrayHeader* grabHeader(void** self)
{
  const AllocationOffset_t offset       = grabOffset((const void**)self);
  void*                    actual_array = *self;

  return (ArrayHeader*)(bfCastByte(actual_array) - offset - sizeof(ArrayHeader));
}
#else
// NOTE(Shareef): Made this into a macro to improve debug build performance.
#define grabHeader(self_) ((ArrayHeader*)(bfCastByte(*(self_)) - grabOffset((const void**)(self_)) - sizeof(ArrayHeader)))
#endif

// API

void* bfArray_mallocator(void* user_data, void* ptr, size_t size)
{
  (void)user_data;

  if (ptr)
  {
    free(ptr);
    ptr = NULL;
  }
  else
  {
    ptr = malloc(size);
  }

  return ptr;
}

void* bfArray_new_(bfArrayAllocator allocator, size_t element_size, size_t element_alignment, void* allocator_user_data)
{
  if (!allocator)
  {
    allocator = &bfArray_mallocator;
  }

  bfAssert(allocator, "bfArray_new_:: Must pass in a valid allocator.");
  bfAssert(element_size, "bfArray_new_:: The struct size must be greater than 0.");
  bfAssert(element_alignment, "bfArray_new_:: The struct alignment must be greater than 0.");
  bfAssert(element_alignment < UINT8_MAX, "bfArray_new_:: The struct alignment must be less than 256.");
  bfAssert(element_size < UINT32_MAX, "bfArray_new_:: The struct alignment must be less than UINT32_MAX.");
  bfAssert((element_alignment & (element_alignment - 1)) == 0, "bfArray_new_:: The struct alignment must be a power of two.");

  const size_t header_size     = sizeof(ArrayHeader) + sizeof(AllocationOffset_t);
  const size_t allocation_size = header_size + (element_alignment - 1);

  ArrayHeader* header = allocator(allocator_user_data, NULL, allocation_size);
  header->allocator   = allocator;
  header->user_data   = allocator_user_data;
  header->size        = 0;
  header->capacity    = 0;
  header->stride      = (uint32_t)element_size;
  header->alignment   = (AllocationOffset_t)element_alignment;

  void* data_start = alignUpPtr((uintptr_t)header + header_size, header->alignment);

  const AllocationOffset_t new_offset = (AllocationOffset_t)((uintptr_t)data_start - (uintptr_t)header - sizeof(ArrayHeader));

  ((AllocationOffset_t*)data_start)[-1] = new_offset;

  return data_start;
}

void* bfArray_userData(void** self)
{
  return grabHeader(self)->user_data;
}

void* bfArray_begin(void** self)
{
  return *self;
}

static void* bfArray_endImpl(const ArrayHeader* header, void** self)
{
  return bfCastByte(*self) + header->size * header->stride;
}

void* bfArray_end(void** self)
{
  // NOTE(Shareef): This function doesn't use 'bfArray_at' since we check the index at size is technically out of bounds.

  const ArrayHeader* const header = grabHeader(self);

  return bfArray_endImpl(header, self);
}

void* bfArray_back(void** self)
{
  return bfArray_at(self, grabHeader(self)->size - 1);
}

size_t bfArray_size(void** self)
{
  return grabHeader(self)->size;
}

size_t bfArray_capacity(void** self)
{
  return grabHeader(self)->capacity;
}

void bfArray_copy(void** self, void** src, size_t num_elements)
{
  ArrayHeader* header_src = grabHeader(src);

  bfAssert(num_elements <= header_src->size, "bfArray_copy:: num_elements must be less than or equal to the source array's size.");

  bfArray_reserve(self, num_elements);

  ArrayHeader* header_self = grabHeader(self);
  header_self->size        = num_elements;

  bfAssert(header_self->stride == header_src->stride, "bfArray_copy:: The source and destination arrays must be of similar types.");

  memcpy(*self, *src, num_elements * header_self->stride);
}

void bfArray_clear(void** self)
{
  grabHeader(self)->size = 0;
}

void bfArray_reserve(void** self, size_t num_elements)
{
  ArrayHeader* header = grabHeader(self);

  if (header->capacity < num_elements)
  {
    //
    // Diagram of Memory:
    //  [ArrayHeader][alignment-padding][allocation-offset][array-data]
    //

    const size_t      header_size_  = sizeof(ArrayHeader) + sizeof(AllocationOffset_t);
    const size_t      header_size   = header_size_ + (header->alignment - 1);
    const size_t      new_data_size = num_elements * header->stride;
    const size_t      old_data_size = header->size * header->stride;
    const ArrayHeader header_state  = *header;

    void* new_allocation = header_state.allocator(header->user_data, NULL, header_size + new_data_size);

    if (new_allocation)
    {
      void* data_start = alignUpPtr((uintptr_t)new_allocation + header_size_, header->alignment);

      memcpy(data_start, *self, old_data_size);

      header_state.allocator(header->user_data, header, header_size + old_data_size);

      const AllocationOffset_t new_offset = (AllocationOffset_t)((uintptr_t)data_start - (uintptr_t)new_allocation - sizeof(ArrayHeader));

      ((AllocationOffset_t*)data_start)[-1] = new_offset;

      header            = (ArrayHeader*)new_allocation;
      header->allocator = header_state.allocator;
      header->user_data = header_state.user_data;
      header->size      = header_state.size;
      header->capacity  = num_elements;
      header->stride    = header_state.stride;
      header->alignment = header_state.alignment;

      *self = data_start;
    }
    else
    {
      header_state.allocator(header->user_data, header, 0);
      *self = NULL;
    }
  }
}

void bfArray_resize(void** self, size_t num_elements)
{
  bfArray_reserve(self, num_elements);
  grabHeader(self)->size = num_elements;
}

void bfArray_push(void** self, const void* element)
{
  void*        destination_element = bfArray_emplace(self);
  ArrayHeader* header              = grabHeader(self);
  memcpy(destination_element, element, header->stride);
}

void bfArray_insert(void** self, size_t index, const void* element)
{
  void*        dst    = bfArray_insertEmplace(self, index);
  ArrayHeader* header = grabHeader(self);
  const size_t stride = header->stride;

  memcpy(dst, element, stride);
}

void* bfArray_insertEmplace(void** self, size_t index)
{
  ArrayHeader* const header = grabHeader(self);
  const size_t       size   = header->size;
  const size_t       stride = header->stride;

  bfAssert(index <= size, "bfArray_insert:: index must be less than or equal to the size.");

  bfArray_resize(self, size + 1);

  void* const move_dst = bfArray_at(self, index + 1);
  void* const move_src = bfArray_at(self, index);

  memmove(move_dst, move_src, stride * (size - index));

  return move_src;
}

void* bfArray_emplace(void** self)
{
  return bfArray_emplaceN(self, 1);
}

void* bfArray_emplaceN(void** self, size_t num_elements)
{
  ArrayHeader* header = grabHeader(self);

  if (header->size + num_elements > header->capacity)
  {
    bfArray_reserve(self, (header->size + num_elements) * 3 / 2);
    header = grabHeader(self);
  }

  void* elements_start = bfArray_endImpl(header, self);

  header->size += num_elements;
  return elements_start;
}

void* bfArray_at(void** self, size_t index)
{
  ArrayHeader* const header = grabHeader(self);

  bfAssert(index < header->size, "bfArray_at:: index must be less than the size.");

  return bfCastByte(*self) + index * header->stride;
}

void* bfArray_binarySearchRange(void** self, size_t bgn, size_t end, const void* key, bfArrayFindCompare compare)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(bgn < header->size, "bfArray_binarySearchRange:: bgn must be less than the size.");
  bfAssert(end <= header->size, "bfArray_binarySearchRange:: end must be less than or equal to the size.");
  bfAssert(end > bgn, "bfArray_binarySearchRange:: end must be greater than bgn.");

  return bsearch(key, bfCastByte(bfArray_begin(self)) + bgn * header->stride, end - bgn, header->stride, compare);
}

void* bfArray_binarySearch(void** self, const void* key, bfArrayFindCompare compare)
{
  return bfArray_binarySearchRange(self, 0, grabHeader(self)->size, key, compare);
}

static int bfArray__find(const void* lhs, const void* rhs)
{
  ArrayDefaultCompareData* data = (ArrayDefaultCompareData*)lhs;
  return memcmp(data->key, rhs, data->stride) == 0;
}

size_t bfArray_findInRange(void** self, size_t bgn, size_t end, const void* key, bfArrayFindCompare compare)
{
  bfAssert(end > bgn, "bfArray_findInRange:: end must be greater than bgn.");

  ArrayDefaultCompareData default_key;

  if (!compare)
  {
    compare            = bfArray__find;
    default_key.key    = key;
    default_key.stride = grabHeader(self)->stride;
    key                = &default_key;
  }

  for (size_t i = bgn; i < end; ++i)
  {
    if (compare(key, bfArray_at(self, i)))
    {
      return i;
    }
  }

  return k_bfArrayInvalidIndex;
}

size_t bfArray_find(void** self, const void* key, bfArrayFindCompare compare)
{
  return bfArray_findInRange(self, 0, bfArray_size(self), key, compare);
}

void bfArray_removeAt(void** self, size_t index)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(index < header->size, "bfArray_removeAt:: index must be less than the size.");

  const size_t num_elemnts_to_move = header->size - index - 1;

  if (num_elemnts_to_move)
  {
    memmove(bfArray_at(self, index), bfArray_at(self, index + 1), num_elemnts_to_move * header->stride);
  }

  --header->size;
}

void bfArray_swapAndPopAt(void** self, size_t index)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(index < header->size, "bfArray_swapAndPopAt:: index must be less than the size.");

  if (header->size)
  {
    memcpy(bfArray_at(self, index), bfArray_back(self), header->stride);
  }

  --header->size;
}

void* bfArray_pop(void** self)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(header->size, "bfArray_pop:: attempt to pop empty array.");

  void* last_element = bfArray_back(self);
  --header->size;
  return last_element;
}

void bfArray_sortRange(void** self, size_t bgn, size_t end, bfArraySortCompare compare)
{
  ArrayHeader* const header = grabHeader(self);

  bfAssert(bgn < header->size, "bfArray_sortRange:: bgn must be less than the size.");
  bfAssert(end <= header->size, "bfArray_sortRange:: end must be less than or equal to the size.");
  bfAssert(end > bgn, "bfArray_sortRange:: end must be greater than bgn.");

  const size_t size = end - bgn;

  if (size >= 2)
  {
    qsort(bfArray_at(self, bgn), size, header->stride, compare);
  }
}

void bfArray_sort(void** self, bfArraySortCompare compare)
{
  bfArray_sortRange(self, 0, grabHeader(self)->size, compare);
}

void bfArray_delete(void** self)
{
  ArrayHeader* const header       = grabHeader(self);
  const size_t       header_size_ = sizeof(ArrayHeader) + sizeof(AllocationOffset_t);
  const size_t       header_size  = header_size_ + (header->alignment - 1);
  const size_t       data_size    = header->size * header->stride;

  header->allocator(header->user_data, header, header_size + data_size);
}

static void* alignUpPtr(uintptr_t element_size, uintptr_t element_alignment)
{
  return (void*)(element_size + (element_alignment - 1) & ~(element_alignment - 1));
}

static AllocationOffset_t grabOffset(const void** self)
{
  return ((const AllocationOffset_t*)*self)[-1];
}
