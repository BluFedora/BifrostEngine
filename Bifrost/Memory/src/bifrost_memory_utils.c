/******************************************************************************/
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
/******************************************************************************/
#include "bifrost/memory/bifrost_memory_utils.h"

#include <assert.h> /* assert */
#include <string.h> /* memcpy */

#define bfCast(e, T) ((T)(e))

size_t bfAlignUpSize(size_t size, size_t required_alignment)
{
  assert(required_alignment > 0 && (required_alignment & (required_alignment - 1)) == 0 && "bfAlignUpSize:: The alignment must be a non-zero power of two.");

  const size_t required_alignment_minus_one = required_alignment - 1;

  return size + required_alignment_minus_one & ~required_alignment_minus_one;
}

void* bfAlignUpPointer(const void* ptr, size_t required_alignment)
{
  assert(required_alignment > 0 && (required_alignment & (required_alignment - 1)) == 0 && "bfAlignUpPointer:: The alignment must be a non-zero power of two.");

  const size_t required_alignment_minus_one = required_alignment - 1;

  return bfCast(bfCast(ptr, uintptr_t) + required_alignment_minus_one & ~required_alignment_minus_one, void*);
}

/* Good read on the various implementations and performance. 
  [https://github.com/KabukiStarship/KabukiToolkit/wiki/Fastest-Method-to-Align-Pointers#21-proof-by-example] 
*/
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

/*
  Small read on Endianess:
    [https://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html]
*/

uint8_t bfBytesReadUint8LE(const char* bytes)
{
  return bfCast(bytes[0], uint8_t);
}

uint16_t bfBytesReadUint16LE(const char* bytes)
{
  return (bfCast(bytes[0], uint16_t) << 0) | (bfCast(bytes[1], uint16_t) << 8);
}

uint32_t bfBytesReadUint32LE(const char* bytes)
{
  const uint32_t b0     = (bfCast(bytes[0], uint32_t) & 0xFF) << 0;
  const uint32_t b1     = (bfCast(bytes[1], uint32_t) & 0xFF) << 8;
  const uint32_t b2     = (bfCast(bytes[2], uint32_t) & 0xFF) << 16;
  const uint32_t b3     = (bfCast(bytes[3], uint32_t) & 0xFF) << 24;
  const uint32_t result = b0 | b1 | b2 | b3;

  return result;
}

uint64_t bfBytesReadUint64LE(const char* bytes)
{
  return (bfCast(bytes[0], uint64_t) << 0) | (bfCast(bytes[1], uint64_t) << 8) |
         (bfCast(bytes[2], uint64_t) << 16) | (bfCast(bytes[3], uint64_t) << 24) |
         (bfCast(bytes[4], uint64_t) << 32) | (bfCast(bytes[5], uint64_t) << 40) |
         (bfCast(bytes[6], uint64_t) << 48) | (bfCast(bytes[7], uint64_t) << 56);
}

uint8_t bfBytesReadUint8BE(const char* bytes)
{
  return bfCast(bytes[0], uint8_t);
}

uint16_t bfBytesReadUint16BE(const char* bytes)
{
  return (bfCast(bytes[1], uint16_t) << 0) | (bfCast(bytes[0], uint16_t) << 8);
}

uint32_t bfBytesReadUint32BE(const char* bytes)
{
  return (bfCast(bytes[3], uint32_t) << 0) | (bfCast(bytes[2], uint32_t) << 8) |
         (bfCast(bytes[1], uint32_t) << 16) | (bfCast(bytes[0], uint32_t) << 24);
}

uint64_t bfBytesReadUint64BE(const char* bytes)
{
  return (bfCast(bytes[7], uint64_t) << 0) | (bfCast(bytes[6], uint64_t) << 8) |
         (bfCast(bytes[5], uint64_t) << 16) | (bfCast(bytes[4], uint64_t) << 24) |
         (bfCast(bytes[3], uint64_t) << 32) | (bfCast(bytes[2], uint64_t) << 40) |
         (bfCast(bytes[1], uint64_t) << 48) | (bfCast(bytes[0], uint64_t) << 56);
}

#define bfBytesReadIntX(utype, itype, func) \
  utype uvalue = func(bytes);               \
  itype ivalue;                             \
                                            \
  memcpy(&ivalue, &uvalue, sizeof(ivalue)); \
                                            \
  return ivalue;

#define bfBytesReadIntXDef(size, type)                                           \
  int##size##_t bfBytesReadInt##size##type(const char* bytes)                    \
  {                                                                              \
    bfBytesReadIntX(uint##size##_t, int##size##_t, bfBytesReadUint##size##type); \
  }

bfBytesReadIntXDef(8, LE);
bfBytesReadIntXDef(16, LE);
bfBytesReadIntXDef(32, LE);
bfBytesReadIntXDef(64, LE);
bfBytesReadIntXDef(8, BE);
bfBytesReadIntXDef(16, BE);
bfBytesReadIntXDef(32, BE);
bfBytesReadIntXDef(64, BE);

void bfBytesWriteUint8LE(char* bytes, const uint8_t value)
{
  bytes[0] = bfCast(value >> 0 & 0xFFu, uint8_t);
}

void bfBytesWriteUint16LE(char* bytes, const uint16_t value)
{
  bytes[0] = bfCast(value >> 0 & 0xFFu, uint8_t);
  bytes[1] = bfCast(value >> 8 & 0xFFu, uint8_t);
}

void bfBytesWriteUint32LE(char* bytes, const uint32_t value)
{
  bytes[0] = bfCast(value >> 0 & 0xFFu, uint8_t);
  bytes[1] = bfCast(value >> 8 & 0xFFu, uint8_t);
  bytes[2] = bfCast(value >> 16 & 0xFFu, uint8_t);
  bytes[3] = bfCast(value >> 24 & 0xFFu, uint8_t);
}

void bfBytesWriteUint64LE(char* bytes, const uint64_t value)
{
  bytes[0] = bfCast(value >> 0 & 0xFFu, uint8_t);
  bytes[1] = bfCast(value >> 8 & 0xFFu, uint8_t);
  bytes[2] = bfCast(value >> 16 & 0xFFu, uint8_t);
  bytes[3] = bfCast(value >> 24 & 0xFFu, uint8_t);
  bytes[4] = bfCast(value >> 32 & 0xFFu, uint8_t);
  bytes[5] = bfCast(value >> 40 & 0xFFu, uint8_t);
  bytes[6] = bfCast(value >> 48 & 0xFFu, uint8_t);
  bytes[7] = bfCast(value >> 56 & 0xFFu, uint8_t);
}

void bfBytesWriteUint8BE(char* bytes, const uint8_t value)
{
  bytes[0] = bfCast(value >> 0 & 0xFFu, uint8_t);
}

void bfBytesWriteUint16BE(char* bytes, const uint16_t value)
{
  bytes[0] = bfCast(value >> 8 & 0xFFu, uint8_t);
  bytes[1] = bfCast(value >> 0 & 0xFFu, uint8_t);
}

void bfBytesWriteUint32BE(char* bytes, const uint32_t value)
{
  bytes[0] = bfCast(value >> 24 & 0xFFu, uint8_t);
  bytes[1] = bfCast(value >> 16 & 0xFFu, uint8_t);
  bytes[2] = bfCast(value >> 8 & 0xFFu, uint8_t);
  bytes[3] = bfCast(value >> 0 & 0xFFu, uint8_t);
}

void bfBytesWriteUint64BE(char* bytes, const uint64_t value)
{
  bytes[0] = bfCast(value >> 56 & 0xFFu, uint8_t);
  bytes[1] = bfCast(value >> 48 & 0xFFu, uint8_t);
  bytes[2] = bfCast(value >> 40 & 0xFFu, uint8_t);
  bytes[3] = bfCast(value >> 32 & 0xFFu, uint8_t);
  bytes[4] = bfCast(value >> 24 & 0xFFu, uint8_t);
  bytes[5] = bfCast(value >> 16 & 0xFFu, uint8_t);
  bytes[6] = bfCast(value >> 8 & 0xFFu, uint8_t);
  bytes[7] = bfCast(value >> 0 & 0xFFu, uint8_t);
}

#define bfBytesWriteIntX(utype, itype, func) \
  utype uvalue;                              \
                                             \
  memcpy(&uvalue, &value, sizeof(value));    \
                                             \
  func(bytes, uvalue)

#define bfBytesWriteIntXDef(size, type)                                            \
  void bfBytesWriteInt##size##type(char* bytes, const int##size##_t value)         \
  {                                                                                \
    bfBytesWriteIntX(uint##size##_t, int##size##_t, bfBytesWriteUint##size##type); \
  }

bfBytesWriteIntXDef(8, LE);
bfBytesWriteIntXDef(16, LE);
bfBytesWriteIntXDef(32, LE);
bfBytesWriteIntXDef(64, LE);
bfBytesWriteIntXDef(8, BE);
bfBytesWriteIntXDef(16, BE);
bfBytesWriteIntXDef(32, BE);
bfBytesWriteIntXDef(64, BE);
