/******************************************************************************/
/*!
* @file   bf_json.h
* @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
* @brief
*   Basic Json parser with an Event (SAX) API.
*   Has some extensions to make wrting json easier.
*   Just search for '@JsonSpecExtension' in the source file.
*
* @version 0.0.1
* @date    2020-03-14
*
* @copyright Copyright (c) 2020-2021
*/
/******************************************************************************/
#ifndef BF_JSON_H
#define BF_JSON_H

#include <stdbool.h> /* bool, true, false */
#include <stddef.h>  /* size_t            */
#include <stdint.h>  /* uint32_t          */

#if __cplusplus
extern "C" {
#endif

/* Customizable Constants */

enum
{
  k_bfJsonStringBlockSize = 256,
  k_bfErrorBufferSize     = 128,
};

/* String View */

typedef struct bfJsonString
{
  const char* string;
  size_t      length;

} bfJsonString;

/* Reader API (String -> Object Event Stream) */

typedef enum bfJsonEvent
{
  BF_JSON_EVENT_BEGIN_DOCUMENT,
  BF_JSON_EVENT_BEGIN_ARRAY,
  BF_JSON_EVENT_END_ARRAY,
  BF_JSON_EVENT_BEGIN_OBJECT,
  BF_JSON_EVENT_END_OBJECT,
  BF_JSON_EVENT_KEY_VALUE,
  BF_JSON_EVENT_END_DOCUMENT,
  BF_JSON_EVENT_PARSE_ERROR,

} bfJsonEvent;

typedef enum bfJsonValueType
{
  BF_JSON_VALUE_STRING,
  BF_JSON_VALUE_NUMBER,
  BF_JSON_VALUE_BOOLEAN,
  BF_JSON_VALUE_NULL,

} bfJsonValueType;

typedef struct bfJsonParserContext bfJsonParserContext;
typedef void (*bfJsonFn)(bfJsonParserContext* ctx, bfJsonEvent event, void* user_data);

void            bfJsonParser_fromString(char* source, size_t source_length, bfJsonFn callback, void* user_data);
bfJsonString    bfJsonParser_errorMessage(const bfJsonParserContext* ctx);
bfJsonString    bfJsonParser_key(const bfJsonParserContext* ctx);
bfJsonValueType bfJsonParser_valueType(const bfJsonParserContext* ctx);
bool            bfJsonParser_valueIs(const bfJsonParserContext* ctx, bfJsonValueType type);
bfJsonString    bfJsonParser_valAsString(const bfJsonParserContext* ctx);
double          bfJsonParser_valAsNumber(const bfJsonParserContext* ctx);
bool            bfJsonParser_valAsBoolean(const bfJsonParserContext* ctx);

/* Writer API (Object -> String) */

typedef struct bfJsonWriter      bfJsonWriter;
typedef struct bfJsonStringBlock bfJsonStringBlock;

typedef void* (*bfJsonAllocFn)(size_t size, void* user_data);
typedef void (*bfJsonFreeFn)(void* ptr, void* user_data);
typedef void (*bfJsonWriterForEachFn)(const bfJsonStringBlock* block, void* user_data);

bfJsonWriter* bfJsonWriter_new(bfJsonAllocFn alloc_fn, void* user_data);
bfJsonWriter* bfJsonWriter_newCRTAlloc(void);
size_t        bfJsonWriter_length(const bfJsonWriter* self);
void          bfJsonWriter_beginArray(bfJsonWriter* self);
void          bfJsonWriter_endArray(bfJsonWriter* self);
void          bfJsonWriter_beginObject(bfJsonWriter* self);
void          bfJsonWriter_key(bfJsonWriter* self, bfJsonString key);
void          bfJsonWriter_valueString(bfJsonWriter* self, bfJsonString value);
void          bfJsonWriter_valueNumber(bfJsonWriter* self, double value);
void          bfJsonWriter_valueBoolean(bfJsonWriter* self, bool value);
void          bfJsonWriter_valueNull(bfJsonWriter* self);
void          bfJsonWriter_next(bfJsonWriter* self);
void          bfJsonWriter_indent(bfJsonWriter* self, int num_spaces);
void          bfJsonWriter_write(bfJsonWriter* self, const char* str, size_t length);
void          bfJsonWriter_endObject(bfJsonWriter* self);
void          bfJsonWriter_forEachBlock(const bfJsonWriter* self, bfJsonWriterForEachFn fn, void* user_data);
void          bfJsonWriter_delete(bfJsonWriter* self, bfJsonFreeFn free_fn);
void          bfJsonWriter_deleteCRT(bfJsonWriter* self);
bfJsonString  bfJsonStringBlock_string(const bfJsonStringBlock* block);

#if __cplusplus
}
#endif

#endif /* BF_JSON_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

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
