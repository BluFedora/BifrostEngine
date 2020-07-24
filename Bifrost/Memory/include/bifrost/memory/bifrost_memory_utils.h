/******************************************************************************/
/*!
 * @file   bifrost_memory_utils.h
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
#ifndef BIFROST_MEMORY_UTILS_HPP
#define BIFROST_MEMORY_UTILS_HPP

#include <stddef.h> /* size_t                                                                   */
#include <stdint.h> /* uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t */

#if __cplusplus
extern "C" {
#endif

// clang-format off
#define bfBytes(n)     (n)
#define bfKilobytes(n) (bfBytes(n) * 1024)
#define bfMegabytes(n) (bfKilobytes(n) * 1024)
#define bfGigabytes(n) (bfMegabytes(n) * 1024)
// clang-format on

/*!
 * @brief 
 *   Aligns size to required_alignment.
 *
 * @param size
 *   The potentially unaligned size of an object.
 * 
 * @param required_alignment 
 *   Must be a non zero power of two.
 *
 * @return size_t 
 *   The size of the object for the required alignment,
 */
size_t bfAlignUpSize(size_t size, size_t required_alignment);

void* bfAlignUpPointer(const void* ptr, size_t required_alignment);

/* Implements "std::align" but in C. */
void* bfStdAlign(size_t alignment, size_t size, void** ptr, size_t* space);

/*
  (Little / Big) Endian Byte Helpers
    The signed versions of these functions assume 2s compliment.
*/

uint8_t  bfBytesReadUint8LE(const char* bytes);
uint16_t bfBytesReadUint16LE(const char* bytes);
uint32_t bfBytesReadUint32LE(const char* bytes);
uint64_t bfBytesReadUint64LE(const char* bytes);
uint8_t  bfBytesReadUint8BE(const char* bytes);
uint16_t bfBytesReadUint16BE(const char* bytes);
uint32_t bfBytesReadUint32BE(const char* bytes);
uint64_t bfBytesReadUint64BE(const char* bytes);
int8_t   bfBytesReadInt8LE(const char* bytes);
int16_t  bfBytesReadInt16LE(const char* bytes);
int32_t  bfBytesReadInt32LE(const char* bytes);
int64_t  bfBytesReadInt64LE(const char* bytes);
int8_t   bfBytesReadInt8BE(const char* bytes);
int16_t  bfBytesReadInt16BE(const char* bytes);
int32_t  bfBytesReadInt32BE(const char* bytes);
int64_t  bfBytesReadInt64BE(const char* bytes);

void bfBytesWriteUint8LE(char* bytes, uint8_t value);
void bfBytesWriteUint16LE(char* bytes, uint16_t value);
void bfBytesWriteUint32LE(char* bytes, uint32_t value);
void bfBytesWriteUint64LE(char* bytes, uint64_t value);
void bfBytesWriteUint8BE(char* bytes, uint8_t value);
void bfBytesWriteUint16BE(char* bytes, uint16_t value);
void bfBytesWriteUint32BE(char* bytes, uint32_t value);
void bfBytesWriteUint64BE(char* bytes, uint64_t value);
void bfBytesWriteInt8LE(char* bytes, int8_t value);
void bfBytesWriteInt16LE(char* bytes, int16_t value);
void bfBytesWriteInt32LE(char* bytes, int32_t value);
void bfBytesWriteInt64LE(char* bytes, int64_t value);
void bfBytesWriteInt8BE(char* bytes, int8_t value);
void bfBytesWriteInt16BE(char* bytes, int16_t value);
void bfBytesWriteInt32BE(char* bytes, int32_t value);
void bfBytesWriteInt64BE(char* bytes, int64_t value);

#if __cplusplus
}
#endif

#endif /* BIFROST_MEMORY_UTILS_HPP */
