/******************************************************************************/
/*!
   @file   bifrost_array_t.c
   @author Shareef Abdoul-Raheem
   @par    email: shareef.a\@digipen.edu
   @brief
     Part of the "Bifrost Data Structures C Library"
     This is a generic dynamic array class the growing algorithm is based
     off of the one used with Python.
     No dependencies besides the C Standard Library.
     Random Access - O(1)
     Pop           - O(1)
     Push, Emplace - O(1) best O(n) worst (when we need to grow)
     Clear         - O(1)
 */
/******************************************************************************/
#include "bifrost/data_structures/bifrost_array_t.h"

#include <assert.h> /* assert         */
#include <stdint.h> /* uint8_t        */
#include <stdlib.h> /* qsort, bsearch */
#include <string.h> /* memcpy         */

#define PRISM_ASSERT(arg, msg) assert((arg) && (msg))
#define SELF_CAST(s) ((BifrostArray*)(s))
#define BIFROST_MALLOC(size, align) malloc((size))
#define BIFROST_REALLOC(ptr, new_size, align) realloc((ptr), new_size)
#define BIFROST_FREE(ptr) free((ptr))
#define PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS 1  // Disable for a faster 'Array_at'.

typedef struct
{
  size_t capacity;
  size_t size;
  size_t stride;

} BifrostArrayHeader;

static BifrostArrayHeader* Array_getHeader(BifrostArray self)
{
  return (BifrostArrayHeader*)(self - sizeof(BifrostArrayHeader));
}

void* _ArrayT_new(const size_t stride, const size_t initial_size)
{
  PRISM_ASSERT(stride, "_ArrayT_new:: The struct must be greater than 0.");
  PRISM_ASSERT(initial_size * stride, "_ArrayT_new:: Please initialize the Array with a size greater than 0");

  BifrostArrayHeader* const self = (BifrostArrayHeader*)
   BIFROST_MALLOC(
    sizeof(BifrostArrayHeader) + stride * initial_size,
    0);

  PRISM_ASSERT(self, "Array_new:: The Dynamic Array could not be allocated");

  if (!self)
  {
    return NULL;
  }

  self->capacity = initial_size;
  self->size     = 0;
  self->stride   = stride;

  return (uint8_t*)self + sizeof(BifrostArrayHeader);
}

void* Array_begin(const void* const self)
{
  return (*SELF_CAST(self));
}

void* Array_end(const void* const self)
{
  BifrostArrayHeader* const header = Array_getHeader(*SELF_CAST(self));
  return *(char**)self + (header->size * header->stride);
}

size_t Array_size(const void* const self)
{
  return Array_getHeader(*SELF_CAST(self))->size;
}

size_t Array_capacity(const void* const self)
{
  return Array_getHeader(*(BifrostArray*)self)->capacity;
}

void Array_copy(const void* const self, BifrostArray* dst)
{
  const size_t size   = Array_size(SELF_CAST(self));
  const size_t stride = Array_getHeader(*SELF_CAST(self))->stride;

  Array_getHeader(*dst)->stride = stride;
  Array_getHeader(*dst)->size   = size;
  Array_reserve(dst, size);
  memcpy((*dst), (*SELF_CAST(self)), size * stride);
}

void Array_clear(void* const self)
{
  Array_getHeader(*SELF_CAST(self))->size = 0;
}

void Array_reserve(void* const self, const size_t num_elements)
{
  BifrostArrayHeader* header = Array_getHeader(*SELF_CAST(self));

  if (header->capacity < num_elements)
  {
    size_t new_capacity = (header->capacity >> 3) + (header->capacity < 9 ? 3 : 6) + header->capacity;

    if (new_capacity < num_elements)
    {
      new_capacity = num_elements;
    }

    BifrostArrayHeader* new_header = (BifrostArrayHeader*)BIFROST_REALLOC(header, sizeof(BifrostArrayHeader) + new_capacity * header->stride, 1);

    if (new_header)
    {
      new_header->capacity = new_capacity;
      *SELF_CAST(self)     = (unsigned char*)new_header + sizeof(BifrostArrayHeader);
    }
    else
    {
      Array_delete(self);
      *SELF_CAST(self) = NULL;
    }
  }
}

void Array_resize(void* const self, const size_t size)
{
  Array_reserve(self, size);
  Array_getHeader(*SELF_CAST(self))->size = size;
}

void Array_push(void* const self, const void* const data)
{
  const size_t stride = Array_getHeader(*SELF_CAST(self))->stride;

  Array_reserve(self, Array_size(self) + 1);

  memcpy(Array_end(self), data, stride);
  ++Array_getHeader(*SELF_CAST(self))->size;
}

void* Array_emplace(void* const self)
{
  return Array_emplaceN(self, 1);
}

void* Array_emplaceN(void* const self, const size_t num_elements)
{
  const size_t old_size = Array_size(self);
  Array_reserve(self, old_size + num_elements);
  uint8_t* const      new_element = Array_end(self);
  BifrostArrayHeader* header      = Array_getHeader(*SELF_CAST(self));
  memset(new_element, 0x0, header->stride * num_elements);
  header->size += num_elements;
  return new_element;
}

void* Array_findFromSorted(const void* const self, const void* const key, const size_t index, const size_t size, ArrayFindCompare compare)
{
  const size_t stride = Array_getHeader(*SELF_CAST(self))->size;

  return bsearch(key, (char*)Array_begin(self) + (index * stride), size, stride, compare);
}

void* Array_findSorted(const void* const self, const void* const key, ArrayFindCompare compare)
{
  return Array_findFromSorted(self, key, 0, Array_size(self), compare);
}

typedef struct
{
  size_t      stride;
  const void* key;

} ArrayDefaultCompareData;

static int Array_findDefaultCompare(const void* lhs, const void* rhs)
{
  ArrayDefaultCompareData* data = (ArrayDefaultCompareData*)lhs;
  return memcmp(data->key, rhs, data->stride) == 0;
}

size_t Array_find(const void* const self, const void* key, ArrayFindCompare compare)
{
  const size_t len = Array_size(self);

  if (compare)
  {
    for (size_t i = 0; i < len; ++i)
    {
      if (compare(key, Array_at(self, i)))
      {
        return i;
      }
    }
  }
  else
  {
    const size_t            stride = Array_getHeader(*SELF_CAST(self))->stride;
    ArrayDefaultCompareData data   = {stride, key};

    for (size_t i = 0; i < len; ++i)
    {
      if (Array_findDefaultCompare(&data, Array_at(self, i)))
      {
        return i;
      }
    }
  }

  return BIFROST_ARRAY_INVALID_INDEX;
}

void* Array_at(const void* const self, const size_t index)
{
#if PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS
  const size_t size         = Array_size(self);
  const int    is_in_bounds = index < size;

  if (!is_in_bounds)
  {
    PRISM_ASSERT(is_in_bounds, "Array_at:: index array out of bounds error");
  }
#endif /* PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS */

  return *(char**)self + (Array_getHeader(*SELF_CAST(self))->stride * index);
}

void* Array_pop(void* const self)
{
#if PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS
  PRISM_ASSERT(Array_size(self) != 0, "Array_pop:: attempt to pop empty array");
#endif

  BifrostArrayHeader* const header      = Array_getHeader(*SELF_CAST(self));
  void* const               old_element = Array_at(self, header->size - 1);
  --header->size;

  return old_element;
}

void* Array_back(const void* const self)
{
  const BifrostArrayHeader* const header = Array_getHeader(*SELF_CAST(self));

  return (char*)Array_end(self) - header->stride;
}

void Array_sort(void* const self, ArraySortCompare compare)
{
  BifrostArrayHeader* const header = Array_getHeader(*SELF_CAST(self));

  if (header->size >= 2)
  {
    qsort(Array_begin(self), header->size, header->stride, compare);
  }
}

void Array_delete(void* const self)
{
  BIFROST_FREE(Array_getHeader(*SELF_CAST(self)));
}

#undef SELF_CAST
#undef PRISM_ASSERT

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// CRT Functions Used:
//   assert, memcpy, memcmp, bsearch, qsort
// STD Types Used:
//   uint8_t, size_t, uintptr_t

typedef uint8_t AllocationOffset_t;

typedef struct
{
  bfArrayAllocator   allocator;
  void*              user_data;
  size_t             size;
  size_t             capacity;
  size_t             stride;
  AllocationOffset_t alignment;

} ArrayHeader;

#define bfAssert(arg, msg) assert((arg) && (msg))
#define bfCastByte(arr) ((unsigned char*)(arr))

// Helpers
static void*              alignUpPtr(uintptr_t element_size, uintptr_t element_alignment);
static AllocationOffset_t grabOffset(const void** self);
static ArrayHeader*       grabHeader(void** self);

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

  bfAssert(allocator, "bfArray_new_:: Must pass in  avalid allocator.");
  bfAssert(element_size, "bfArray_new_:: The struct size must be greater than 0.");
  bfAssert(element_alignment, "bfArray_new_:: The struct alignment must be greater than 0.");
  bfAssert(element_alignment < 256, "bfArray_new_:: The struct alignment must be less than 256.");
  bfAssert((element_alignment & (element_alignment - 1)) == 0, "bfArray_new_:: The struct alignment must be a power of two.");

  const size_t header_size     = sizeof(ArrayHeader) + sizeof(AllocationOffset_t);
  const size_t allocation_size = header_size + (element_alignment - 1);

  ArrayHeader* header = allocator(allocator_user_data, NULL, allocation_size);
  header->allocator   = allocator;
  header->user_data   = allocator_user_data;
  header->size        = 0;
  header->capacity    = 0;
  header->stride      = element_size;
  header->alignment   = (AllocationOffset_t)element_alignment;

  void* data_start = alignUpPtr((uintptr_t)header + header_size, header->alignment);

  const AllocationOffset_t new_offset = (AllocationOffset_t)((uintptr_t)data_start - (uintptr_t)header - sizeof(ArrayHeader));

  ((AllocationOffset_t*)data_start)[-1] = new_offset;

  return data_start;
}

void* bfArray_begin(void** self)
{
  return *self;
}

void* bfArray_end(void** self)
{
  // NOTE(Shareef): This function doesn't use 'bfArray_at' since we check the index and @size is technically out of bounds.

  ArrayHeader* header = grabHeader(self);

  return bfCastByte(*self) + header->size * header->stride;
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

  void* elements_start = bfArray_end(self);

  header->size += num_elements;
  return elements_start;
}

void* bfArray_at(void** self, size_t index)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(index < header->size, "bfArray_at:: index must be less than the size.");

  return bfCastByte(*self) + index * header->stride;
}

void* bfArray_binarySearchRange(void** self, size_t bgn, size_t end, const void* key, bfArrayFindCompare compare)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(bgn < header->size, "bfArray_binarySearchRange:: bgn must be less than the size.");
  bfAssert(end <= header->size, "bfArray_binarySearchRange:: end must be less than or equal to the size.");
  bfAssert(end > bgn, "bfArray_binarySearchRange:: end must be greater than bgn.");

  return bsearch(key, bfCastByte(Array_begin(self)) + bgn * header->stride, end - bgn, header->stride, compare);
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

size_t bfArray_findInRange(void** self, size_t bgn, size_t end, const void* key, ArrayFindCompare compare)
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
    if (compare(key, Array_at(self, i)))
    {
      return i;
    }
  }

  return BIFROST_ARRAY_INVALID_INDEX;
}

size_t bfArray_find(void** self, const void* key, ArrayFindCompare compare)
{
  return bfArray_findInRange(self, 0, Array_size(self), key, compare);
}

void bfArray_removeAt(void** self, size_t index)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(index < header->size, "bfArray_removeAt:: index must be less than the size.");

  const size_t num_elemnts_to_move = header->size - index - 1;

  if (num_elemnts_to_move)
  {
    memmove(Array_at(self, index), Array_at(self, index + 1), num_elemnts_to_move * header->stride);
  }

  --header->size;
}

void bfArray_swapAndPopAt(void** self, size_t index)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(index < header->size, "bfArray_swapAndPopAt:: index must be less than the size.");

  if (header->size)
  {
    memcpy(Array_at(self, index), Array_back(self), header->stride);
  }

  --header->size;
}

void* bfArray_pop(void** self)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(header->size, "bfArray_pop:: attempt to pop empty array.");

  void* last_element = Array_back(self);
  --header->size;
  return last_element;
}

void bfArray_sortRange(void** self, size_t bgn, size_t end, ArraySortCompare compare)
{
  ArrayHeader* header = grabHeader(self);

  bfAssert(bgn < header->size, "bfArray_sortRange:: bgn must be less than the size.");
  bfAssert(end <= header->size, "bfArray_sortRange:: end must be less than or equal to the size.");
  bfAssert(end > bgn, "bfArray_sortRange:: end must be greater than bgn.");

  const size_t size = end - bgn;

  if (size >= 2)
  {
    qsort(bfArray_at(self, bgn), size, header->stride, compare);
  }
}

void bfArray_sort(void** self, ArraySortCompare compare)
{
  bfArray_sortRange(self, 0, grabHeader(self)->size, compare);
}

void bfArray_delete(void** self)
{
  ArrayHeader* header       = grabHeader(self);
  const size_t header_size_ = sizeof(ArrayHeader) + sizeof(AllocationOffset_t);
  const size_t header_size  = header_size_ + (header->alignment - 1);
  const size_t data_size    = header->size * header->stride;

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

static ArrayHeader* grabHeader(void** self)
{
  const AllocationOffset_t offset       = grabOffset((const void**)self);
  void*                    actual_array = *self;

  return (ArrayHeader*)(bfCastByte(actual_array) - offset - sizeof(ArrayHeader));
}
