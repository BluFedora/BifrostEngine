/******************************************************************************/
/*!
 * @file   bifrost_uuid.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Platform abstraction for generating globally unique identifiers.
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019
 */
/******************************************************************************/
#ifndef BIFROST_UUID_H
#define BIFROST_UUID_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BifrostUUIDNumber_t
{
  char data[16]; /*!< The number as a string. */

} BifrostUUIDNumber;

typedef struct BifrostUUIDString_t
{
  char data[37]; /*!< Nul terminated string. '00000000-0000-0000-0000-000000000000\0' */

} BifrostUUIDString;

typedef struct BifrostUUID_t
{
  char as_number[16]; /*!< The number as a string.                                         */
  char as_string[37]; /*!< Nul terminated string. '00000000-0000-0000-0000-000000000000\0' */

} BifrostUUID;

BifrostUUID bfUUID_makeEmpty(void);
BifrostUUID bfUUID_generate(void);
BifrostUUID bfUUID_fromString(const char source[37]);  // Expects a string in the format: '00000000-0000-0000-0000-000000000000' (Note lack of curly braces)
int         bfUUID_isEqual(const BifrostUUID* lhs, const BifrostUUID* rhs);
int         bfUUID_isEmpty(const BifrostUUID* self);
void        bfUUID_numberToString(const char number[16], char out_string[37]);

#ifdef __cplusplus
}
#endif

#endif /* BIFROST_UUID_H */
