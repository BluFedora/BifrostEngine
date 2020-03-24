// #define cast(o, T)    ((T)(o))
// 
// #if _MSC_VER
// #define max_align_t (_Alignof(long double))
// #endif
// 
// #if !__cplusplus
// #define nullptr NULL
// #endif
// 
// // __alignof() or _Alignof
// 
// /* Returns the number of bytes to add to align the pointer to a boundary */
// // https://github.com/kabuki-starship/kabuki-toolkit/wiki/Fastest-Method-to-Align-Pointers#21-proof-by-example
// static inline uintptr_t memory_align_offset(const void* ptr, const size_t size)
// {
//     // is_aligned: ((unsigned long)p & (ALIGN - 1)) == 0
//   return ((~cast(ptr, uintptr_t)) + 1) & (size - 1);
// }
// 
// /* Aligns the given pointer to an 'alignment boundary */
// static void* std_align(const size_t alignment, const size_t size, void** ptr, size_t* space)
// {
//     // Alignment should be: sizeof(uintptr_t)
//   const uintptr_t offset = memory_align_offset((*ptr), alignment);
// 
//   if ((*space) >= (size + offset))
//   {
//     void* aligned_ptr = cast(cast((*ptr), char*) + offset, void*);
// 
//     (*ptr) = aligned_ptr;
//     (*space) -= offset;
// 
//     return aligned_ptr;
//   }
// 
//   return NULL;
// }
// 
// typedef union {
// 
//   char *__p;
// 
//   double __d;
// 
//   long double __ld;
// 
//   long int __i;
// } max_align_t;
// 
// #ifdef HAVE_TYPEOF
// 
// /*
// * Can use arbitrary expressions
// */
// #define alignof(t) \
// ((sizeof (t) > 1)? offsetof(struct { char c; typeof(t) x; }, x) : 1)
// 
// #else
// 
// /*
// * Can only use types
// */
// #define alignof(t) \
// ((sizeof (t) > 1)? offsetof(struct { char c; t x; }, x) : 1)
// 
// #endif