//----------------------------------------------------------------------------//
//
// File Name:   bifrost_dynamic_string.c
// Author(s):   Shareef Raheem
// Project:     SR Utilities
//
// Copyright (C) 2018 Shareef Raheem.
//----------------------------------------------------------------------------//
#include "bifrost/data_structures/bifrost_dynamic_string.h"

#ifndef BIFROST_MALLOC
#define BIFROST_MALLOC(size, align) malloc((size))
#define BIFROST_REALLOC(ptr, new_size, align) realloc((ptr), new_size)
#define BIFROST_FREE(ptr) free((ptr))
#endif

#ifndef bfAssert
#define bfAssert(cond, msg) assert((cond) && (msg))
#endif

#include <assert.h> /* assert                             */
#include <stdarg.h> /* va_list, va_start, va_copy, va_end */
#include <stdio.h>  /* snprintf                           */
#include <stdlib.h> /* exit                               */
#include <string.h> /* strlen, strcpy, strncmp, memmove   */

#if 1
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#ifndef _DEBUG
#define _DEBUG 1
#endif

#if STR_FAST_CMP
int fast_strncmp(const char* s1, const char* s2, size_t length)
{
#define is_different(a, b) ((a) ^ (b)) /* XOR, 0 if same */
#define char_diff(a, b) (int)((unsigned char)((a)) - (unsigned char)((b)))

  const size_t  long_size    = length / sizeof(size_t);
  const size_t* ls1          = (const size_t*)s1;
  const size_t* ls2          = (const size_t*)s2;
  size_t        short_walker = long_size * sizeof(size_t) + (long_size != 0);
  size_t        long_walker  = 0;

  while (long_walker < long_size)
  {
    const size_t* lcur1 = &ls1[long_walker];
    const size_t* lcur2 = &ls2[long_walker];

    if (is_different(*lcur1, *lcur2))
    {
      const char* cur1 = (const char*)lcur1;
      const char* cur2 = (const char*)lcur2;

      while (*cur1 == *cur2 && *cur1 && *cur2)
      {
        ++cur1;
        ++cur2;
      }

      return char_diff(*cur1, *cur2);
    }

    ++long_walker;
  }

  while (short_walker < length)
  {
    const char c1 = s1[short_walker];
    const char c2 = s2[short_walker];

    if (is_different(c1, c2) || c1 == '\0' || c2 == '\0')
    {
      return char_diff(c1, c2);
    }

    ++short_walker;
  }

#undef is_different
#undef char_diff

  return 0;
}
#endif /* STR_FAST_CMP */

typedef struct BifrostStringHeader_t
{
  size_t capacity;
  size_t length;

} BifrostStringHeader;

static BifrostStringHeader* String_getHeader(ConstBifrostString self);
static void                 string_append(BifrostString* self, size_t appended_size, const char* str);
static void                 string_insert(BifrostString* self, size_t index, size_t stringLength, const char* str);

BifrostString String_new(const char* initial_data)
{
  return String_newLen(initial_data, strlen(initial_data));
}

BifrostString String_newLen(const char* initial_data, size_t string_length)
{
  const size_t str_capacity = string_length + 1;
  const size_t total_size   = sizeof(BifrostStringHeader) + str_capacity;

  BifrostStringHeader* const self = BIFROST_MALLOC(total_size, sizeof(char));

  if (self)
  {
    self->capacity   = str_capacity;
    self->length     = string_length;
    char* const data = (char*)self + sizeof(BifrostStringHeader);

    /*
     // NOTE(Shareef):
     //   According to the standard memcpy cannot take in a NULL
     //   pointer and "string_length" must be non-zero.
    */
    if (initial_data && string_length)
    {
      memcpy(data, initial_data, string_length);
    }

    data[string_length] = '\0';

    return data;
  }

  return NULL;
}

BifrostString String_clone(BifrostString cloned)
{
  return String_newLen(String_cstr(cloned), String_length(cloned));
}

void String_reserve(BifrostString* self, size_t new_capacity)
{
  BifrostStringHeader* header = String_getHeader(*self);

  if (new_capacity > header->capacity)
  {
    while (header->capacity < new_capacity)
    {
      header->capacity *= 2;
    }

    BifrostStringHeader* old_header = header;

    header = (BifrostStringHeader*)BIFROST_REALLOC(old_header, sizeof(BifrostStringHeader) + header->capacity, 1);

    if (header)
    {
      *self = (char*)header + sizeof(BifrostStringHeader);
    }
    else
    {
      String_delete(*self);
      *self = NULL;
    }
  }
}

void String_resize(BifrostString* self, size_t new_size)
{
  String_reserve(self, new_size + 1u);
  BifrostStringHeader* header = String_getHeader(*self);
  header->length              = new_size;
  (*self)[new_size]           = '\0';
}

size_t String_length(ConstBifrostString self)
{
  return String_getHeader(self)->length;
}

size_t String_capacity(ConstBifrostString self)
{
  return String_getHeader(self)->capacity;
}

const char* String_cstr(ConstBifrostString self)
{
  return self;
}

void String_cset(BifrostString* self, const char* str)
{
  String_getHeader(*self)->length = 0;
  String_cappend(self, str);
}

void String_cappend(BifrostString* self, const char* str)
{
  String_cappendLen(self, str, strlen(str));
}

void String_cappendLen(BifrostString* self, const char* str, size_t len)
{
  string_append(self, len, str);
}

void String_append(BifrostString* self, ConstBifrostString str)
{
  string_append(self, String_length(str), String_cstr(str));
}

void String_cinsert(BifrostString* self, size_t index, const char* str)
{
  string_insert(self, index, (size_t)strlen(str), str);
}

void String_insert(BifrostString* self, size_t index, ConstBifrostString str)
{
  string_insert(self, index, String_length(str), str);
}

static unsigned char escape_convert(const unsigned char c)
{
  switch (c)
  {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '\\': return '\\';
    case '\'': return '\'';
    case '\"': return '\"';
    case '?': return '\?';
    default: return '\0';
  }
}

void String_unescape(BifrostString self)
{
  String_getHeader(self)->length = CString_unescape(self);
}

size_t CString_unescape(char* str)
{
  const char* oldStr = str;
  char*       newStr = str;

  while (*oldStr)
  {
    unsigned char c = *(unsigned char*)(oldStr++);

    if (c == '\\')
    {
      c = *(unsigned char*)(oldStr++);
      if (c == '\0') break;
      if (escape_convert(c)) c = escape_convert(c);
    }

    *newStr++ = (char)c;
  }

  *newStr = '\0';

  return (newStr - str);
}

// Fowler-Noll-Vo Hash (FNV1a)
uint32_t bfString_hash(const char* str)
{
  uint32_t hash = 0x811c9dc5;

  while (*str)
  {
    hash ^= (unsigned char)*str++;
    hash *= 0x01000193;
  }

  return hash;
}

uint32_t bfString_hashN(const char* str, size_t length)
{
  uint32_t hash = 0x811c9dc5;

  const char* str_end = str + length;

  while (str != str_end)
  {
    hash ^= (unsigned char)*str;
    hash *= 0x01000193;
    ++str;
  }

  return hash;
}

uint64_t bfString_hash64(const char* str)
{
  uint64_t hash = 14695981039346656037ULL;

  while (*str)
  {
    hash ^= (unsigned char)*str++;
    hash *= 1099511628211ULL;
  }

  return hash;
}

uint64_t bfString_hashN64(const char* str, size_t length)
{
  uint64_t hash = 14695981039346656037ULL;

  const char* str_end = str + length;

  while (str != str_end)
  {
    hash ^= (unsigned char)*str;
    hash *= 1099511628211ULL;
    ++str;
  }

  return hash;
}

int String_cmp(ConstBifrostString self, ConstBifrostString other)
{
  const size_t len1 = String_length(self);
  const size_t len2 = String_length(other);

  if (len1 != len2)
  {
    return -1;
  }

#if STR_FAST_CMP
  return fast_strncmp(self, other, len1);
#else
  return strncmp(self, other, len1);
#endif
}

int String_ccmp(ConstBifrostString self, const char* other)
{
  return strcmp(String_cstr(self), other);
}

int String_ccmpn(ConstBifrostString self, const char* other, size_t length)
{
  if (length > String_length(self))
  {
    return -1;
  }

  return strncmp(String_cstr(self), other, length);
}

// __attribute__((__format__ (__printf__, 2, 0)))
void String_sprintf(BifrostString* self, const char* format, ...)
{
  va_list args, args_cpy;
  va_start(args, format);

  //#pragma clang diagnostic push
  //#pragma clang diagnostic ignored "-Wformat-nonliteral"
  va_copy(args_cpy, args);
  const size_t num_chars = (size_t)vsnprintf(NULL, 0, format, args_cpy);
  va_end(args_cpy);
  //#pragma clang diagnostic pop

  String_reserve(self, num_chars + 2ul);
  vsnprintf(*self, num_chars + 1ul, format, args);
  String_getHeader(*self)->length = num_chars;

  va_end(args);
}

void String_clear(BifrostString* self)
{
  String_resize(self, 0u);
}

void String_delete(BifrostString self)
{
  BIFROST_FREE(String_getHeader(self));
}

/*
 NOTE(Shareef):
  Private Functions
*/
static BifrostStringHeader* String_getHeader(ConstBifrostString self)
{
  return (BifrostStringHeader*)(self - sizeof(BifrostStringHeader));
}

static int doRangesOverlap(const char* x0, size_t x0_length, const char* y0, size_t y0_length)
{
  const char* const x1 = x0 + x0_length;
  const char* const y1 = y0 + y0_length;

  return x0 <= y1 && y0 <= x1;
}

static void string_append(BifrostString* self, size_t appended_size, const char* str)
{
  const size_t self_size = String_length(*self);

  assert(!doRangesOverlap(*self, self_size, str, appended_size) && "Cannot append a string to itself. Maybe the feature could be added in future versions but with how the code is right now it will not work.");

  const size_t new_size  = self_size + appended_size;

  String_reserve(self, new_size + 1);
  strncpy(*self + self_size, str, appended_size);

  String_getHeader(*self)->length = new_size;
  (*self)[new_size]               = '\0';
}

static void string_insert(BifrostString* self, size_t index, size_t stringLength, const char* str)
{
  const size_t selfLength = String_length(*self);
  const size_t newLength  = selfLength + stringLength;

  bfAssert(index < selfLength, "Insertion index must be less than the string length itself.");

  String_reserve(self, newLength + 1);

  char* const insertionPoint = ((*self) + index);
  char* const endPoint       = insertionPoint + stringLength;

  memmove(endPoint, insertionPoint, (selfLength - index));
  memcpy(insertionPoint, str, stringLength);

  (*self)[newLength]              = '\0';
  String_getHeader(*self)->length = newLength;
}
#endif
