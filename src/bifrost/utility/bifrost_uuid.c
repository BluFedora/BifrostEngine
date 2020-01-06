#include "bifrost/utility/bifrost_uuid.h"

#include "bifrost/platform/bifrost_platform.h" /* BIFROST_PLATFORM_* */

#include <stdio.h>  /* sprintf        */
#include <string.h> /* strcpy, memcpy */

#if BIFROST_PLATFORM_WINDOWS
#include <Objbase.h> /* GUID, CoCreateGuid, IIDFromString */
#elif BIFROST_PLATFORM_LINUX
#include <uuid/uuid.h> /* uuid_generate_random, uuid_unparse, uuid_parse */
#else
#error "Unsupported platform for generating guids."
#endif

static const BifrostUUID s_EmptyUUID =
 {
  "0000000000000000",
  "00000000-0000-0000-0000-000000000000",
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
    memcpy(self.as_number, &out, sizeof(self.as_number));

    sprintf(self.as_string,
            "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            out.Data1,
            out.Data2,
            out.Data3,
            out.Data4[0],
            out.Data4[1],
            out.Data4[2],
            out.Data4[3],
            out.Data4[4],
            out.Data4[5],
            out.Data4[6],
            out.Data4[7]);

    return self;
  }

  return bfUUID_makeEmpty();
#elif BIFROST_PLATFORM_LINUX
  BifrostUUID self;

  uuid_t out;
  uuid_generate_random(out);

  memcpy(self.as_number, out, sizeof(self.as_number));

  uuid_unparse(out, self.as_string);

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

  mbstowcs(source_wide + 1, source, 36);  // <cstdlib>

  if (IIDFromString(source_wide, &out) == S_OK)
  {
    BifrostUUID self;
    memcpy(self.as_number, &out, sizeof(self.as_number));

    strcpy(self.as_string, source);

    return self;
  }

  return bfUUID_makeEmpty();
#elif BIFROST_PLATFORM_LINUX
  uuid_t out;

  if (uuid_parse((char*)source, &out) != -1)
  {
    BifrostUUID self;
    memcpy(self.as_number, out, sizeof(self.as_number));
    strcpy(self.as_string, source);

    return self;
  }

  return bfUUID_makeEmpty();

#else
#error "Unsupported platform for generating guids."
#endif
}

int bfUUID_isEqual(const BifrostUUID* lhs, const BifrostUUID* rhs)
{
  for (int i = 0; i < 16; ++i)
  {
    if (lhs->as_number[i] != rhs->as_number[i])
    {
      return 0;
    }
  }

  return 1;
}

int bfUUID_isEmpty(const BifrostUUID* self)
{
  return bfUUID_isEqual(self, &s_EmptyUUID);
}
