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

enum
{
  k_bfUUIDStringCapacity = 37,
};

typedef struct BifrostUUIDNumber_t
{
  char data[16]; /*!< The number as a string. */

} BifrostUUIDNumber;

typedef struct BifrostUUIDString_t
{
  char data[k_bfUUIDStringCapacity]; /*!< Nul terminated string. 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0' */

} BifrostUUIDString;

typedef struct BifrostUUID_t
{
  BifrostUUIDNumber as_number; /*!< The number as a string.                                         */
  BifrostUUIDString as_string; /*!< Nul terminated string. 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0' */

} BifrostUUID;

BifrostUUID bfUUID_makeEmpty(void);
BifrostUUID bfUUID_generate(void);
BifrostUUID bfUUID_fromString(const char source[k_bfUUIDStringCapacity]);  // Expects a string in the format: 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx' (Note lack of curly braces)
int         bfUUID_isEqual(const BifrostUUID* lhs, const BifrostUUID* rhs);
int         bfUUID_isEmpty(const BifrostUUID* self);
void        bfUUID_numberToString(const char number[16], char out_string[37]);
int         bfUUID_numberCmp(const BifrostUUIDNumber* lhs, const BifrostUUIDNumber* rhs);
int         bfUUID_numberIsEmpty(const BifrostUUIDNumber* lhs);
int         bfUUID_stringCmp(const BifrostUUIDString* lhs, const BifrostUUIDString* rhs);

#ifdef __cplusplus
}
#endif

#endif /* BIFROST_UUID_H */
