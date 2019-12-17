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

#ifndef BIFROST_MALLOC
#define BIFROST_MALLOC(size, align) malloc((size))
#define BIFROST_REALLOC(ptr, new_size, align) realloc((ptr), new_size)
#define BIFROST_FREE(ptr) free((ptr))
#endif

#ifndef BIFROST_MALLOC
#include "bifrost/memory/bifrost_allocator.h"
#define BIFROST_MALLOC(size, align) Bifrost_alloc((size), (align))
#define BIFROST_REALLOC(ptr, new_size, align) Bifrost_realloc((ptr), (new_size), (align))
#define BIFROST_FREE(ptr) Bifrost_free((ptr))
#endif

#include <stdint.h> /* uint8_t        */
#include <stdlib.h> /* qsort, bsearch */
#include <string.h> /* memcpy         */
#ifdef _DEBUG
#include <assert.h> /* assert         */
#define PRISM_ASSERT(arg, msg) assert((arg) && (msg))
#else
#define PRISM_ASSERT(arg, msg) /* NO-OP */
#endif

#define SELF_CAST(s) ((BifrostArray*)(s))

typedef struct BifrostArrayHeader_t
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
    bfAlignof(BifrostArrayHeader));

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
    /* NOTE(Shareef):
           The growth pattern is:  0, 4, 8, 16, 25, 35, 46, 58, 72, 88, etc...
           Used By Python and they seem pretty alright.
      */
    size_t new_capacity = (header->capacity >> 3) + (header->capacity < 9 ? 3 : 6) + header->capacity;

    if (new_capacity < num_elements)
    {
      new_capacity = num_elements;
    }

    BifrostArrayHeader* new_header = (BifrostArrayHeader*)BIFROST_REALLOC(header, sizeof(BifrostArrayHeader) + new_capacity * header->stride, 1);

    if (new_header)
    {
      new_header->capacity = new_capacity;
      *SELF_CAST(self)     = (char*)new_header + sizeof(BifrostArrayHeader);
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
