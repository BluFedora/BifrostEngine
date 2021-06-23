/******************************************************************************/
/*!
* @file   bf_core.h
* @author Shareef Raheem (https://blufedora.github.io)
* @brief
*  Helper base functionality that is generally useful throughout the codebase.
*
* @version 0.0.1
* @date    2020-06-21
*
* @copyright Copyright (c) 2019-2021 Shareef Abdoul-Raheem
*/
/******************************************************************************/
#ifndef BF_CORE_H
#define BF_CORE_H

#include <stddef.h> /* size_t             */
#include <stdint.h> /* uint32_t, uint64_t */

#if _MSC_VER
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

/*!
 * @brief
 *  NOTE(Shareef):
 *    This is a compiler intrinsic that tells the compiler that
 *    the this class will never be instantiated directly and thus vtable
 *    initialization does not need to happen in the base class's ctor.
 */
#define bfPureInterface(T) __declspec(novtable) T

#define bfDebugBreak() __debugbreak()

#else

// TODO(SR):
//   [https://stackoverflow.com/questions/44054078/how-to-guide-gcc-optimizations-based-on-assertions-without-runtime-cost]
#define bfInvalidDefaultCase() \
  default:                     \
    break

#define bfPureInterface(T) T

#define bfDebugBreak() assert(!"TODO(SR): Do real debug break for other compilers.")

#endif

#if __cplusplus
#undef bfCArraySize
#undef bfSizeOfField
#undef bfBit

template<typename T, size_t N>
static constexpr size_t bfCArraySize(const T((&)[N]))
{
  return N;
}

#define bfSizeOfField(T, member) (sizeof(T::member))

template<typename T>
static constexpr T bfBit(T bit_idx)
{
  return T(1) << bit_idx;
}

#else
#define bfCArraySize(arr) ((sizeof(arr) / sizeof(0 [arr])) / ((size_t)(!(sizeof(arr) % sizeof(0 [arr])))))  //(sizeof((arr)) / sizeof((arr)[0]))
#define bfSizeOfField(T, member) (sizeof(((T*)0)->member))
#define bfBit(index) (1ULL << (index))
#endif

/*!
 * @brief
 *   This will retrieve the containing object from one of it's members.
*/
#define bfObjectFromMember(ptr, type, member) ((type*)((char*)(ptr)-offsetof(type, member)))

/*!
 * @brief
 *   Will quote an arbirtary set of characters so that you can use them as
 *   a string literal.
 * 
 *   Ex:
 *     char json_source[] = bfQuoteStr({
 *         "MyKey" : 0.5,
 *         "Another Key" : [
 *           "Array Element",
 *           7.0
 *         ]
 *     });
 */
#define bfQuoteStr(...) #__VA_ARGS__

// This is used to clearly mark flexible-sized arrays that appear at the end of
// some dynamically-allocated structs, known as the "struct hack".
#if __STDC_VERSION__ >= 199901L
// In C99, a flexible array member is just "[]".
#define bfStructHack
#else
// Elsewhere, use a zero-sized array. It's technically undefined behavior,
// but works reliably on most known compilers.
#define bfStructHack 0
#endif

#if __cplusplus
extern "C" {
#endif

typedef unsigned char bfByte;
typedef uint16_t      bfBool16;
typedef uint32_t      bfBool32;
typedef float         bfFloat32;
typedef double        bfFloat64;
#define bfTrue 1
#define bfFalse 0

/**** Core API Types ****/

/*!
 * @brief
 *   A non-owning reference to a string.
*/
typedef struct bfStringRange
{
  const char* str_bgn;
  const char* str_end;

} bfStringRange;

typedef bfStringRange bf_string_range;
typedef bfStringRange string_range;

inline size_t StringRange_length(bfStringRange self)
{
  return self.str_end - self.str_bgn;
}


#if __cplusplus
constexpr
#endif
 static inline bfStringRange
 bfMakeStringRangeLen(const char* bgn, size_t length)
{
#if __cplusplus
  return {
   bgn,
   bgn + length,
  };
#else
  return (bfStringRange){
   .str_bgn = bgn,
   .str_end = bgn + length,
  };
#endif
}

#if __cplusplus
constexpr
#endif
 static inline bfStringRange
 bfMakeStringRangeC(const char* str)
{
  const char* end = str;

  while (end[0]) { ++end; }

  return bfMakeStringRangeLen(str, end - str);
}

/* Color Types */

typedef struct color4f
{
  float r, g, b, a;

} color4f;

typedef struct color4u
{
  uint8_t r, g, b, a;

} color4u;

typedef uint32_t color32h; /*!< Could be in either 0xAABBGGRR or 0xAARRGGBB format. */

/* Parameters ordered from lowest to highest byte. */
inline color32h color32h_make(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  return ((color32h)r << 0) |
         ((color32h)g << 8) |
         ((color32h)b << 16) |
         ((color32h)a << 24);
}

inline color32h color32h_makeBGRA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  return color32h_make(b, g, r, a);
}

inline color32h color32h_makeARGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  return color32h_make(a, r, g, b);
}

/* Math Types */

typedef struct vec2i
{
  int x, y;

} vec2i;

typedef struct vec2f
{
  float x, y;

} vec2f;

typedef struct vec3f
{
  float x, y, z;

} vec3f;

typedef struct vec4f
{
  float x, y, z, w;

} vec4f;

typedef struct rect2i
{
  vec2i min, max;

} rect2i;

typedef struct rect2f
{
  vec2f min, max;

} rect2f;

#if __cplusplus
}
#endif

#endif /* BF_CORE_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
