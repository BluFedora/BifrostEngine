#ifndef BIFROST_STD_H
#define BIFROST_STD_H

#include <stdint.h> /* uint32_t, uint64_t */

#if __cplusplus
extern "C" {
#endif
#define bfCArraySize(arr) ((sizeof(arr) / sizeof(0 [arr])) / ((size_t)(!(sizeof(arr) % sizeof(0 [arr])))))  //(sizeof((arr)) / sizeof((arr)[0]))
#define bfBit(index) (1ULL << (index))
#define BIFROST_C_ARRAY_SIZE(arr) bfCArraySize(arr)
typedef uint32_t bfBool32;
typedef float    bfFloat32;
typedef double   bfFloat64;
#define bfTrue 1
#define bfFalse 0

typedef struct
{
  const char* bgn;
  const char* end;

} bfStringRange;

static inline bfStringRange bfMakeStringRangeC(const char* str)
{
  const char* end = str;

  while (end[0])
  {
    ++end;
  }

#if __cplusplus
  return {
   str,
   end,
  };
#else
  return (bfStringRange){
   .bgn = str,
   .end = end,
  };
#endif
}

// This is used to clearly mark flexible-sized arrays that appear at the end of
// some dynamically-allocated structs, known as the "struct hack".
#if __STDC_VERSION__ >= 199901L
// In C99, a flexible array member is just "[]".
#define bfStructHack
#else
// Elsewhere, use a zero-sized array. It's technically undefined behavior,
// but works reliably in most known compilers.
#define bfStructHack 0
#endif
#if __cplusplus
}
#endif

#endif /* BIFROST_STD_H */