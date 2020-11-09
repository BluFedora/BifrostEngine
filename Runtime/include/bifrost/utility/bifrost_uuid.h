/******************************************************************************/
/*!
 * @file   bf_uuid.h
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
#ifndef BF_UUID_H
#define BF_UUID_H

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  k_bfUUIDStringCapacity = 37,
};

typedef struct
{
  char data[16]; /*!< The number as a string. */

} bfUUIDNumber;

typedef struct BifrostUUIDString_t
{
  char data[k_bfUUIDStringCapacity]; /*!< Nul terminated string. 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0' */

} BifrostUUIDString;

typedef struct
{
  bfUUIDNumber      as_number; /*!< The number as a string.                                         */
  BifrostUUIDString as_string; /*!< Nul terminated string. 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0' */

} bfUUID;

bfUUID bfUUID_makeEmpty(void);
bfUUID bfUUID_generate(void);
bfUUID bfUUID_fromString(const char source[k_bfUUIDStringCapacity]);  // Expects a string in the format: 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' (Note lack of curly braces)
int    bfUUID_isEqual(const bfUUID* lhs, const bfUUID* rhs);
int    bfUUID_isEmpty(const bfUUID* self);
void   bfUUID_numberToString(const char number[16], char out_string[37]);
int    bfUUID_numberCmp(const bfUUIDNumber* lhs, const bfUUIDNumber* rhs);
int    bfUUID_numberIsEmpty(const bfUUIDNumber* lhs);
int    bfUUID_stringCmp(const BifrostUUIDString* lhs, const BifrostUUIDString* rhs);

#ifdef __cplusplus
}
#endif

#endif /* BF_UUID_H */
