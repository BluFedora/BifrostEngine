//----------------------------------------------------------------------------//
//
// File Name:   bifrost_dynamic_string.h
// Author(s):   Shareef Raheem
// Project:     Bifrost Data Structures C Library
//
// Copyright (C) 2018 Shareef Raheem.
//----------------------------------------------------------------------------//
#ifndef BIFROST_DYNAMIC_STRING_H
#define BIFROST_DYNAMIC_STRING_H

#include <stddef.h> /* size_t   */
#include <stdint.h> /* uint32_t */

#define STR_FAST_CMP 1

  // NOTE(Shareef):
  //   The memory layout: [BifrostStringHeader (capacity | length) | BifrostString (char*)]
  //   A 'BifrostString' can be used anywhere a 'normal' C string can be used.
typedef char*       BifrostString;
typedef const char* ConstBifrostString;

  // DOCS:
  //   Parameter Sematics:
  //     Paramters with "BifrostString" or "ConstBifrostString" MUST be created with 
  //     "String_new" or "String_newLen" or "String_clone" otherwise if
  //     "const char*" is the parameter type then either a normal C string 
  //     or a BifrostString can be passed in.

  // NOTE(Shareef): Creates a new String on the Heap
BifrostString     String_new(const char* initial_data);
BifrostString     String_newLen(const char* initial_data, size_t string_length);

  // NOTE(Shareef): Creates a new String from the other string
BifrostString     String_clone(BifrostString cloned);

  // NOTE(Shareef): This is mostly a private function but can be used
  //   if you know how many bytes need to be reserved
  //   before you do any manipulation (used for optimization)
void              String_reserve(BifrostString* self, size_t newCapacity);

  // NOTE(Shareef): Gets the length of the string.
size_t            String_length(ConstBifrostString self);

  // NOTE(Shareef): Gets the capacity of the string.
size_t            String_capacity(ConstBifrostString self);

  // NOTE(Shareef): Gets the "Real" c string
const char*       String_cstr(ConstBifrostString self);

  // NOTE(Shareef): Sets this string's data from the cString
void              String_cset(BifrostString* self, const char *str);

  // NOTE(Shareef): Appends a string into this
void              String_cappend(BifrostString* self, const char* str);
void              String_cappendLen(BifrostString* self, const char* str, size_t len);

  // NOTE(Shareef): Appends a dyanmic string to this string
void              String_append(BifrostString* self, ConstBifrostString str);

  // NOTE(Shareef): Inserts the text at a certain index in this string
void              String_cinsert(BifrostString* self, size_t index, const char *str);

  // NOTE(Shareef): Inserts the text at a certain index in this string
void              String_insert(BifrostString* self, size_t index, ConstBifrostString str);

  // NOTE(Shareef): Processes escape characters of the string
  //   EX: \n, \t, \" etc
void              String_unescape(BifrostString self);

  // NOTE(Shareef): Processes escape characters of the c string
  //   returns the new length of the string
  //   EX: \n, \t, \" etc
size_t            CString_unescape(char* str);

uint32_t          bfString_hash(const char* str);
uint32_t          bfString_hashN(const char* str, size_t length);

  // NOTE(Shareef): returns 0 if the strings are the same
  //   works the same as strncmp (but potentially more optimized)
int               String_cmp(ConstBifrostString self, ConstBifrostString other);

  // NOTE(Shareef): returns 0 if the strings are the same
  //   works the same as strcmp
int               String_ccmp(ConstBifrostString self, const char* other);
int               String_ccmpn(ConstBifrostString self, const char* other, size_t length);

  // NOTE(Shareef): Just like sprintf but with memory management
void              String_sprintf(BifrostString* self, const char* format, ...);

  // NOTE(Shareef): clears the string's data
void              String_clear(BifrostString* self);

  // NOTE(Shareef): Deletes the heap allocated string
void              String_delete(BifrostString self);

#endif /* BIFROST_DYNAMIC_STRING_H */
