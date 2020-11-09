/******************************************************************************/
/*!
 * @file   bf_uuid.c
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

#include <stdio.h>  /* sprintf        */
#include <string.h> /* strcpy, memcpy */

#if _WIN32
#include <Objbase.h> /* GUID, CoCreateGuid, IIDFromString */
#include <wchar.h>   /* mbsrtowcs                         */
#elif __linux || __APPLE__
#include <uuid/uuid.h> /* uuid_generate_random, uuid_unparse, uuid_parse */
#else
#error "Unsupported platform for generating guids."
#endif

// TODO(SR): Apple should use CFUUIDCreate 
// TODO(SR): Support for Android.

static const bfUUID s_EmptyUUID =
 {
  {"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"},
  {"00000000-0000-0000-0000-000000000000"},
};

bfUUID bfUUID_makeEmpty(void)
{
  return s_EmptyUUID;
}

bfUUID bfUUID_generate(void)
{
#if _WIN32
  GUID out;

  if (CoCreateGuid(&out) == S_OK)
  {
    bfUUID self;
    memcpy(self.as_number.data, &out, sizeof(self.as_number.data));

    bfUUID_numberToString(self.as_number.data, self.as_string.data);

    return self;
  }

  return bfUUID_makeEmpty();
#elif __linux || __APPLE__
  bfUUID self;

  uuid_t out;
  uuid_generate_random(out);

  memcpy(self.as_number.data, out, sizeof(self.as_number.data));

  bfUUID_numberToString(self.as_number.data, self.as_string.data);

  return self;
#else
#error "Unsupported platform for generating guids."
#endif
}

bfUUID bfUUID_fromString(const char source[k_bfUUIDStringCapacity])
{
#if _WIN32
  GUID out;

  WCHAR source_wide[k_bfUUIDStringCapacity + 2];

  source_wide[0]  = L'{';
  source_wide[k_bfUUIDStringCapacity + 0] = L'}';
  source_wide[k_bfUUIDStringCapacity + 1] = L'\0';

  // NOTE(SR):
  //   Using `mbsrtowcs` over `mbstowcs` since `mbstowcs` is not 
  //   thread safe due to accessing and manipulating a static "mbstate_t".
  //
  //   Making a UUID should be a thread safe operation.

  mbstate_t state;
  ZeroMemory(&state, sizeof state);

  const char* source_cpy = source; // mbsrtowcs modifies the source argument for some god awful reason.

  mbsrtowcs(source_wide + 1, &source, k_bfUUIDStringCapacity - 1, &state);

  if (IIDFromString(source_wide, &out) == S_OK)
  {
    bfUUID self;
    memcpy(self.as_number.data, &out, sizeof(self.as_number));

    strncpy(self.as_string.data, source_cpy, k_bfUUIDStringCapacity);

    return self;
  }
#elif __linux || __APPLE__
  uuid_t out;

  if (uuid_parse((char*)source, out) != -1)
  {
    bfUUID self;
    memcpy(self.as_number.data, out, sizeof(self.as_number.data));
    strcpy(self.as_string.data, source);

    return self;
  }
#else
#error "Unsupported platform for generating guids."
#endif
  return bfUUID_makeEmpty();
}

int bfUUID_isEqual(const bfUUID* lhs, const bfUUID* rhs)
{
  return bfUUID_numberCmp(&lhs->as_number, &rhs->as_number);
}

int bfUUID_isEmpty(const bfUUID* self)
{
  return bfUUID_isEqual(self, &s_EmptyUUID);
}

void bfUUID_numberToString(const char number[16], char out_string[37])
{
#if _WIN32
  GUID native_guid;
#elif __linux || __APPLE__
  uuid_t native_guid;
#else
#error "Unsupported platform for generating guids."
#endif

  memcpy(&native_guid, number, sizeof(native_guid));

#if _WIN32
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
#elif __linux || __APPLE__
  uuid_unparse(native_guid, out_string);
#else
#error "Unsupported platform for generating guids."
#endif
}

int bfUUID_numberCmp(const bfUUIDNumber* lhs, const bfUUIDNumber* rhs)
{
  const int num_bytes_to_cmp = (int)sizeof(bfUUIDNumber);

  for (int i = 0; i < num_bytes_to_cmp; ++i)
  {
    if (lhs->data[i] != rhs->data[i])
    {
      return 0;
    }
  }

  return 1;
}

int bfUUID_numberIsEmpty(const bfUUIDNumber* lhs)
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
