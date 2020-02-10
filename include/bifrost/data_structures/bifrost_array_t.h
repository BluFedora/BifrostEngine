/******************************************************************************/
/*!
  @file   bifrost_array_t.h
  @author Shareef Abdoul-Raheem
  @brief
    Part of the "Bifrost Data Structures C Library"
    This is a generic dynamic array class.

    No dependencies besides the C Standard Library.
    Random Access - O(1)
    Pop           - O(1)
    Push, Emplace - O(1) best O(n) worst (when we need to grow)
    Clear         - O(1)

    The memory layout:
      [ArrayHeader (capacity | size | stride)][alignment-padding][allocation-offset]^[array-data (uint8_t*)]
      '^' indicates the pointer you are handed back from the 'bfArray_new' function.

    Memory Allocation Customization:
      To use your own allocator pass an allocation function to the array.
      To be compliant you must:
        > Act as malloc  when ptr == NULL, size == number of bytes to alloc.
        > Act as free    when ptr != NULL, size == number of bytes given back.
*/
/******************************************************************************/
#ifndef BIFROST_DATA_STRUCTURES_ARRAY_H
#define BIFROST_DATA_STRUCTURES_ARRAY_H

#include <stddef.h> /* size_t */

// Old API with default allocator
#define OLD_bfArray_newT(T, initial_size) (T*)_ArrayT_new(sizeof(T), (initial_size))
#if __cplusplus
#include <type_traits> /* remove_reference_t */
#define OLD_bfArray_newA(arr, initial_size) (typename std::remove_reference_t<decltype((arr)[0])>*)_ArrayT_new(sizeof((arr)[0]), (initial_size))
#else
#define OLD_bfArray_newA(arr, initial_size) _ArrayT_new(sizeof((arr)[0]), (initial_size))
#endif
#define Array_new(T, initial_size) OLD_bfArray_newT(T, initial_size)
#if __cplusplus
extern "C" {
#endif
typedef unsigned char* BifrostArray;
typedef int(__cdecl* ArraySortCompare)(const void*, const void*);
typedef int(__cdecl* ArrayFindCompare)(const void*, const void*);
void*  _ArrayT_new(const size_t stride, const size_t initial_size);
void*  Array_begin(const void* const self);
void*  Array_end(const void* const self);
size_t Array_size(const void* const self);
size_t Array_capacity(const void* const self);
void   Array_copy(const void* const self, BifrostArray* dst);
void   Array_clear(void* const self);
void   Array_reserve(void* const self, const size_t num_elements);
void   Array_resize(void* const self, const size_t size);
void   Array_push(void* const self, const void* const data);
void*  Array_emplace(void* const self);
void*  Array_emplaceN(void* const self, const size_t num_elements);
void*  Array_findFromSorted(const void* const self, const void* const key, const size_t index, const size_t size, ArrayFindCompare compare);
void*  Array_findSorted(const void* const self, const void* const key, ArrayFindCompare compare);
size_t Array_find(const void* const self, const void* key, ArrayFindCompare compare);
void*  Array_at(const void* const self, const size_t index);
void*  Array_pop(void* const self);
void*  Array_back(const void* const self);
void   Array_sort(void* const self, ArraySortCompare compare);
void   Array_delete(void* const self);
#if __cplusplus
}
#endif

/* New allocator aware API */

#if __cplusplus
extern "C" {
#endif
#define BIFROST_ARRAY_INVALID_INDEX ((size_t)(-1))

typedef void* (*bfArrayAllocator)(void* user_data, void* ptr, size_t size);

/* a  < b : < 0 */
/* a == b :   0 */
/* a  > b : > 0 */
typedef int(__cdecl* bfArraySortCompare)(const void*, const void*);
typedef int(__cdecl* bfArrayFindCompare)(const void*, const void*);

#if __cplusplus
#include <type_traits> /* remove_reference_t */
#define bfArray_new(T, alloc, user_data) (T*)bfArray_new_(alloc, sizeof(T), alignof(T), (user_data))
#define bfArray_newA(arr, alloc, user_data) bfArray_new(typename std::remove_reference_t<decltype((arr)[0])>, alloc, user_data)
#else
#define bfArray_new(T, alloc, user_data) (T*)bfArray_new_(alloc, sizeof(T), _Alignof(T), (user_data))
#define bfArray_newA(arr, alloc, user_data) bfArray_new((arr)[0], alloc, user_data)
#endif

// A 'BifrostArray' can be used anywhere a 'normal' C array can be used.
// For all functions pass a pointer to the array.

void*  bfArray_mallocator(void* user_data, void* ptr, size_t size);  // Default allocator uses C-Libs 'malloc' and 'free'.
void*  bfArray_new_(bfArrayAllocator allocator, size_t element_size, size_t element_alignment, void* allocator_user_data);
void*  bfArray_userData(void** self);
void*  bfArray_begin(void** self);
void*  bfArray_end(void** self);
void*  bfArray_back(void** self);
size_t bfArray_size(void** self);
size_t bfArray_capacity(void** self);
void   bfArray_copy(void** self, void** src, size_t num_elements);
void   bfArray_clear(void** self);
void   bfArray_reserve(void** self, size_t num_elements);
void   bfArray_resize(void** self, size_t num_elements);
void   bfArray_push(void** self, const void* element);
void*  bfArray_emplace(void** self);
void*  bfArray_emplaceN(void** self, size_t num_elements);
void*  bfArray_at(void** self, size_t index);
void*  bfArray_binarySearchRange(void** self, size_t bgn, size_t end, const void* key, bfArrayFindCompare compare); /* [bgn, end) */
void*  bfArray_binarySearch(void** self, const void* key, bfArrayFindCompare compare);
/* If [compare] is NULL then the default impl uses a memcmp with the sizeof a single element. */
/* [key] is always the first parameter for each comparison.                                   */
/* Returns BIFROST_ARRAY_INVALID_INDEX on failure to find element.                            */
size_t bfArray_findInRange(void** self, size_t bgn, size_t end, const void* key, ArrayFindCompare compare);
size_t bfArray_find(void** self, const void* key, ArrayFindCompare compare);
void   bfArray_removeAt(void** self, size_t index);
void   bfArray_swapAndPopAt(void** self, size_t index);
void*  bfArray_pop(void** self);
void   bfArray_sortRange(void** self, size_t bgn, size_t end, ArraySortCompare compare); /* [bgn, end) */
void   bfArray_sort(void** self, ArraySortCompare compare);
void   bfArray_delete(void** self);
#if __cplusplus
}
#endif

#endif /* BIFROST_DATA_STRUCTURES_ARRAY_H */
