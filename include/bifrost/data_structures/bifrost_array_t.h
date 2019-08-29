/******************************************************************************/
/*!
  @file   bifrost_array_t.h
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

    The memory layout: [BifrostArrayHeader (capacity | size | stride) | BifrostArray (uint8_t*)]

    Memory Allocation Customization:
      To use your own allocation then define these three macros
      with the same name and arguments.
      You are also passed in the wanted alignment but ignoring
      it is generally fine assuming you don't care about the 
      alignment of the elements in the array.

      EX:
        #define BIFROST_MALLOC(size, align)           malloc((size))
        #define BIFROST_REALLOC(ptr, new_size, align) realloc((ptr), new_size)
        #define BIFROST_FREE(ptr)                     free((ptr))
*/
/******************************************************************************/
#ifndef BIFROST_DATA_STRUCTURES_ARRAY_H
#define BIFROST_DATA_STRUCTURES_ARRAY_H

#include <stddef.h> /* size_t */

#define bfArray_newT(T, initial_size) (T*)_ArrayT_new(sizeof(T), (initial_size))

#if __cplusplus
#include <type_traits> /* remove_reference_t */
#define bfArray_newA(arr, initial_size) (typename std::remove_reference_t<decltype((arr)[0])>*)_ArrayT_new(sizeof((arr)[0]), (initial_size))
#else
#define bfArray_newA(arr, initial_size) _ArrayT_new(sizeof((arr)[0]), (initial_size))
#endif

// Old Stuff
#define PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS 1  // Disable for a faster 'Array_at'.
#define Array_new(T, initial_size) bfArray_newT(T, initial_size)

#if __cplusplus
extern "C" {
#endif
#define BIFROST_ARRAY_INVALID_INDEX ((size_t)(-1))

// NOTE(Shareef):
//   
//   A 'BifrostArray' can be used anywhere a 'normal' C array can be used.
typedef unsigned char* BifrostArray;

// a  < b : < 0 //
// a == b :   0 //
// a  > b : > 0 //
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
// If [compare] is NULL then the default impl uses a memcmp with the sizeof a single element.
// [key] is always the first parameter for each comparison.
size_t Array_find(const void* const self, const void* key, ArrayFindCompare compare);
void*  Array_at(const void* const self, const size_t index);
void*  Array_pop(void* const self);
void*  Array_back(const void* const self);
void   Array_sort(void* const self, ArraySortCompare compare);
void   Array_delete(void* const self);
#if __cplusplus
}
#endif

#endif /* BIFROST_DATA_STRUCTURES_ARRAY_H */
