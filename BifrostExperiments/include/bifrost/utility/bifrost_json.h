/******************************************************************************/
/*!
* @file   bifrost_json.h
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Basic Json parser with an Event (SAX) API.
*   Has some extensions to make wrting json easier.
*   Just search for '@JsonSpecExtention'.
*
* @version 0.0.1
* @date    2020-03-14
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_JSON_H
#define BIFROST_JSON_H

#include "bifrost/bifrost_std.h" /* size_t, bfBool32, bfStringRange */

#if __cplusplus
extern "C" {
#endif

#define BIFROST_JSON_USER_STORAGE_SIZE 64
#define BIFROST_JSON_STRING_BLOCK_SIZE 256

typedef enum bfJsonEvent_t
{
  BIFROST_JSON_EVENT_BEGIN_DOCUMENT,
  BIFROST_JSON_EVENT_END_DOCUMENT,
  BIFROST_JSON_EVENT_BEGIN_ARRAY,
  BIFROST_JSON_EVENT_END_ARRAY,
  BIFROST_JSON_EVENT_BEGIN_OBJECT,
  BIFROST_JSON_EVENT_END_OBJECT,
  BIFROST_JSON_EVENT_KEY,
  BIFROST_JSON_EVENT_VALUE,
  BIFROST_JSON_EVENT_PARSE_ERROR,

} bfJsonEvent;

typedef enum bfJsonType_t
{
  BIFROST_JSON_VALUE_STRING,
  BIFROST_JSON_VALUE_NUMBER,
  BIFROST_JSON_VALUE_BOOLEAN,
  BIFROST_JSON_VALUE_NULL,

} bfJsonType;

struct bfJsonParserContext_t;
typedef struct bfJsonParserContext_t bfJsonParserContext;

struct bfJsonStringBlock_t;
typedef struct bfJsonStringBlock_t bfJsonStringBlock;

struct bfJsonWriter_t;
typedef struct bfJsonWriter_t BifrostJsonWriter;

typedef void (*bfJsonFn)(bfJsonParserContext* ctx, bfJsonEvent event, void* user_data);
typedef void* (*bfJsonAllocFn)(size_t size);
typedef void (*bfJsonFreeFn)(void* ptr);
typedef void (*bfJsonWriterForEachFn)(const bfJsonStringBlock* block, void* user_data);

/* Reader API (String -> Object) */
void          bfJsonParser_fromString(char* source, size_t source_length, bfJsonFn callback, void* user_data);
const char*   bfJsonParser_errorMessage(const bfJsonParserContext* ctx);
bfJsonType    bfJsonParser_valueType(const bfJsonParserContext* ctx);
bfBool32      bfJsonParser_valueIs(const bfJsonParserContext* ctx, bfJsonType type);
bfStringRange bfJsonParser_asString(const bfJsonParserContext* ctx);
double        bfJsonParser_asNumber(const bfJsonParserContext* ctx);
bfBool32      bfJsonParser_asBoolean(const bfJsonParserContext* ctx);
void*         bfJsonParser_userStorage(const bfJsonParserContext* ctx);
void*         bfJsonParser_parentUserStorage(const bfJsonParserContext* ctx);
/* Writer API (Object -> String) */
BifrostJsonWriter* bfJsonWriter_new(bfJsonAllocFn alloc_fn);
size_t             bfJsonWriter_length(const BifrostJsonWriter* self);
void               bfJsonWriter_beginArray(BifrostJsonWriter* self);
void               bfJsonWriter_endArray(BifrostJsonWriter* self);
void               bfJsonWriter_beginObject(BifrostJsonWriter* self);
void               bfJsonWriter_key(BifrostJsonWriter* self, bfStringRange key);
void               bfJsonWriter_valueString(BifrostJsonWriter* self, bfStringRange value);
void               bfJsonWriter_valueNumber(BifrostJsonWriter* self, double value);
void               bfJsonWriter_valueBoolean(BifrostJsonWriter* self, bfBool32 value);
void               bfJsonWriter_valueNull(BifrostJsonWriter* self);
void               bfJsonWriter_next(BifrostJsonWriter* self);
void               bfJsonWriter_indent(BifrostJsonWriter* self, int num_spaces);
void               bfJsonWriter_write(BifrostJsonWriter* self, const char* str, size_t length);
void               bfJsonWriter_endObject(BifrostJsonWriter* self);
void               bfJsonWriter_forEachBlock(const BifrostJsonWriter* self, bfJsonWriterForEachFn fn, void* user_data);
void               bfJsonWriter_delete(BifrostJsonWriter* self, bfJsonFreeFn free_fn);
bfStringRange      bfJsonStringBlock_string(const bfJsonStringBlock* block);

#if __cplusplus
}
#endif

#endif /* BIFROST_JSON_H */
