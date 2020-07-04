/******************************************************************************/
/*!
 * @file   bifrost_uuid.c
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
#include "bifrost/utility/bifrost_uuid.h"

#include "bifrost/platform/bifrost_platform.h" /* BIFROST_PLATFORM_* */

#include <stdio.h>  /* sprintf        */
#include <string.h> /* strcpy, memcpy */

#if BIFROST_PLATFORM_WINDOWS
#include <Objbase.h> /* GUID, CoCreateGuid, IIDFromString */
#elif BIFROST_PLATFORM_LINUX || BIFROST_PLATFORM_MACOS
#include <uuid/uuid.h> /* uuid_generate_random, uuid_unparse, uuid_parse */
#else
#error "Unsupported platform for generating guids."
#endif

static const BifrostUUID s_EmptyUUID =
 {
  {"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"},
  {"00000000-0000-0000-0000-000000000000"},
};

BifrostUUID bfUUID_makeEmpty(void)
{
  return s_EmptyUUID;
}

BifrostUUID bfUUID_generate(void)
{
#if BIFROST_PLATFORM_WINDOWS
  GUID out;

  if (CoCreateGuid(&out) == S_OK)
  {
    BifrostUUID self;
    memcpy(self.as_number.data, &out, sizeof(self.as_number.data));

    bfUUID_numberToString(self.as_number.data, self.as_string.data);

    return self;
  }

  return bfUUID_makeEmpty();
#elif BIFROST_PLATFORM_LINUX || BIFROST_PLATFORM_MACOS
  BifrostUUID self;

  uuid_t out;
  uuid_generate_random(out);

  memcpy(self.as_number.data, out, sizeof(self.as_number.data));

  bfUUID_numberToString(self.as_number.data, self.as_string.data);

  return self;
#else
#error "Unsupported platform for generating guids."
#endif
}

BifrostUUID bfUUID_fromString(const char source[37])
{
#if BIFROST_PLATFORM_WINDOWS
  GUID out;

  WCHAR source_wide[39];

  source_wide[0]  = L'{';
  source_wide[37] = L'}';
  source_wide[38] = L'\0';

  mbstowcs(source_wide + 1, source, 36); /* <cstdlib> */

  if (IIDFromString(source_wide, &out) == S_OK)
  {
    BifrostUUID self;
    memcpy(self.as_number.data, &out, sizeof(self.as_number));

    strcpy(self.as_string.data, source);

    return self;
  }
#elif BIFROST_PLATFORM_LINUX || BIFROST_PLATFORM_MACOS
  uuid_t out;

  if (uuid_parse((char*)source, out) != -1)
  {
    BifrostUUID self;
    memcpy(self.as_number.data, out, sizeof(self.as_number.data));
    strcpy(self.as_string.data, source);

    return self;
  }
#else
#error "Unsupported platform for generating guids."
#endif
  return bfUUID_makeEmpty();
}

int bfUUID_isEqual(const BifrostUUID* lhs, const BifrostUUID* rhs)
{
  return bfUUID_numberCmp(&lhs->as_number, &rhs->as_number);
}

int bfUUID_isEmpty(const BifrostUUID* self)
{
  return bfUUID_isEqual(self, &s_EmptyUUID);
}

void bfUUID_numberToString(const char number[16], char out_string[37])
{
#if BIFROST_PLATFORM_WINDOWS
  GUID native_guid;
#elif BIFROST_PLATFORM_LINUX || BIFROST_PLATFORM_MACOS
  uuid_t native_guid;
#else
#error "Unsupported platform for generating guids."
#endif

  memcpy(&native_guid, number, sizeof(native_guid));

#if BIFROST_PLATFORM_WINDOWS
  sprintf(out_string,
          "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
          native_guid.Data1,
          native_guid.Data2,
          native_guid.Data3,
          native_guid.Data4[0],
          native_guid.Data4[1],
          native_guid.Data4[2],
          native_guid.Data4[3],
          native_guid.Data4[4],
          native_guid.Data4[5],
          native_guid.Data4[6],
          native_guid.Data4[7]);
#elif BIFROST_PLATFORM_LINUX || BIFROST_PLATFORM_MACOS
  uuid_unparse(native_guid, out_string);
#else
#error "Unsupported platform for generating guids."
#endif
}

int bfUUID_numberCmp(const BifrostUUIDNumber* lhs, const BifrostUUIDNumber* rhs)
{
  const int num_bytes_to_cmp = (int)sizeof(BifrostUUIDNumber);

  for (int i = 0; i < num_bytes_to_cmp; ++i)
  {
    if (lhs->data[i] != rhs->data[i])
    {
      return 0;
    }
  }

  return 1;
}

int bfUUID_numberIsEmpty(const BifrostUUIDNumber* lhs)
{
  return bfUUID_numberCmp(lhs, &s_EmptyUUID.as_number);
}

int bfUUID_stringCmp(const BifrostUUIDString* lhs, const BifrostUUIDString* rhs)
{
  const int num_bytes_to_cmp = (int)sizeof(BifrostUUIDString) - 1;

  for (int i = 0; i < num_bytes_to_cmp; ++i)
  {
    if (lhs->data[i] != rhs->data[i])
    {
      return 0;
    }
  }

  return 1;
}
