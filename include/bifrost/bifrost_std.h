#ifndef BIFROST_STD_H
#define BIFROST_STD_H

#include <stddef.h> /* size_t             */
#include <stdint.h> /* uint32_t, uint64_t */

/*!
 * @brief
 *  NOTE(Shareef):
 *    This is a compiler intrinsic that tells the compiler that
 *    the default case cannot be reached thus letting the optimizer
 *    do less checks to see if the expression is not in the
 *    set of valid cases.
 */
#define bfInvalidDefaultCase() \
  default:                     \
    __debugbreak();            \
    __assume(0)

#if __cplusplus
#undef bfCArraySize
template<typename T, size_t N>
static constexpr size_t bfCArraySize(const T((&)[N]))
{
  return N;
}

#define bfSizeOfField(T, member) (sizeof(T::member))

#if 0
template<typename T>
T bfClamp(T min_value, T value, T max_value)
{
  return (value < min_value) ? min_value : (value > max_value ? max_value : value);
}
#endif

#else
#define bfCArraySize(arr) ((sizeof(arr) / sizeof(0 [arr])) / ((size_t)(!(sizeof(arr) % sizeof(0 [arr])))))  //(sizeof((arr)) / sizeof((arr)[0]))
#define bfSizeOfField(T, member) (sizeof(((T*)0)->member))
#endif

#if __cplusplus
extern "C" {
#endif
#define bfBit(index) (1ULL << (index))
typedef uint32_t bfBool32;
typedef float    bfFloat32;
typedef double   bfFloat64;
#define bfTrue 1
#define bfFalse 0

typedef struct bfStringRange_t
{
  const char* bgn;
  const char* end;

} bfStringRange;

static bfStringRange bfMakeStringRangeLen(const char* bgn, size_t length)
{
#if __cplusplus
  return {
   bgn,
   bgn + length,
  };
#else
  return (bfStringRange){
   .bgn = bgn,
   .end = bgn + length,
  };
#endif
}

static bfStringRange bfMakeStringRangeC(const char* str)
{
  const char* end = str;

  while (end[0])
  {
    ++end;
  }

  return bfMakeStringRangeLen(str, end - str);
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
