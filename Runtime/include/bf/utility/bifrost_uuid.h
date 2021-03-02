/******************************************************************************/
/*!
 * @file   bf_uuid.h
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Platform abstraction for generating globally unique identifiers.
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_UUID_H
#define BF_UUID_H

#include <stdint.h> /* uint64_t, uint32_t, uint16_t, uint8_t */

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  k_bfUUIDStringLength   = 36,
  k_bfUUIDStringCapacity = k_bfUUIDStringLength + 1, /*!< The +1 is for nul terminator. */
};

typedef struct bfUUIDNumber
{
  union
  {
    char     bytes[16];
    uint8_t  bytes8[16];
    uint16_t bytes16[8];
    uint32_t bytes32[4];
    uint64_t bytes64[2];
  };

} bfUUIDNumber;

typedef struct bfUUIDString
{
  char data[k_bfUUIDStringCapacity]; /*!< Nul terminated string. 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0' */

} bfUUIDString;

typedef struct bfUUID
{
  bfUUIDNumber as_number; /*!< The number as a string.                                         */
  bfUUIDString as_string; /*!< Nul terminated string. 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0' */

} bfUUID;

bfUUID bfUUID_makeEmpty(void);
bfUUID bfUUID_generate(void);
bfUUID bfUUID_fromString(const char source[k_bfUUIDStringCapacity]);  // Expects a string in the format: 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' (Note lack of curly braces)
int    bfUUID_isEqual(const bfUUID* lhs, const bfUUID* rhs);
int    bfUUID_isEmpty(const bfUUID* self);
void   bfUUID_numberToString(const char number[16], char out_string[k_bfUUIDStringCapacity]);
int    bfUUID_numberCmp(const bfUUIDNumber* lhs, const bfUUIDNumber* rhs);
int    bfUUID_numberIsEmpty(const bfUUIDNumber* lhs);
int    bfUUID_stringCmp(const bfUUIDString* lhs, const bfUUIDString* rhs);

#ifdef __cplusplus
}
#endif

#endif /* BF_UUID_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2019-2021 Shareef Abdoul-Raheem

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
