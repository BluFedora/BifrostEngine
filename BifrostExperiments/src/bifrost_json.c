/******************************************************************************/
/*!
* @file   bifrost_json.c
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Basic Json parser with an Event (SAX) API.
*   Has some extensions to make wrting json easier.
*   Just search for '@JsonSpecExtention' in the source file.
*
* @version 0.0.1
* @date    2020-03-14
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bifrost/utility/bifrost_json.h"

#include <ctype.h>  /* isspace, isalpha, isdigit, tolower, isxdigit  */
#include <setjmp.h> /* jmp_buf, setjmp, longjmp                      */
#include <stdio.h>  /* snprintf                                      */
#include <stdlib.h> /* strtol, strtod                                */

/*
  Reader API
*/

typedef enum
{
  BIFROST_JSON_L_CURLY   = '{',
  BIFROST_JSON_R_CURLY   = '}',
  BIFROST_JSON_L_SQR_BOI = '[',
  BIFROST_JSON_R_SQR_BOI = ']',
  BIFROST_JSON_COMMA     = ',',
  BIFROST_JSON_QUOTE     = '"',
  BIFROST_JSON_COLON     = ':',
  BIFROST_JSON_TRUE      = 't',
  BIFROST_JSON_FALSE     = 'f',
  BIFROST_JSON_NULL      = 'n',
  BIFROST_JSON_NUMBER    = '#',
  BIFROST_JSON_EOF       = '!',

} bfJsonTokenType;

typedef struct bfJsonUserStorage_t
{
  char                        storage[BIFROST_JSON_USER_STORAGE_SIZE];
  struct bfJsonUserStorage_t* parent;

} bfJsonUserStorage;

typedef struct
{
  bfJsonTokenType type;
  const char*     source_bgn;
  const char*     source_end;

} bfJsonObject;

struct bfJsonParserContext_t
{
  char*              source_bgn;
  const char*        source_end;
  bfJsonObject       current_object;
  char*              current_location;
  size_t             line_no;
  bfJsonFn           callback;
  void*              user_data;
  bfJsonUserStorage  document_storage;
  bfJsonUserStorage* current_user_storage;
  char               error_message[256];
  jmp_buf            error_handling;
};

static bfBool32 bfJsonIsSpace(char character);
static bfBool32 bfJsonIsSameCharacterI(char a, char b);
static bfBool32 bfJsonIsSameStringI(const char* a, const char* b, size_t n);
static bfBool32 bfJsonIs(bfJsonParserContext* ctx, bfJsonTokenType type);
static char     bfJsonCurrentChar(const bfJsonParserContext* ctx);
static void     bfJsonIncrement(bfJsonParserContext* ctx);
static void     bfJsonSkipSpace(bfJsonParserContext* ctx);
static bfBool32 bfJsonIsKeyword(const bfJsonParserContext* ctx);
static void     bfJsonSkipKeyword(bfJsonParserContext* ctx);
static bfBool32 bfJsonIsDigit(const bfJsonParserContext* ctx);
static bfBool32 bfJsonIsNumber(const bfJsonParserContext* ctx);
static void     bfJsonSkipNumber(bfJsonParserContext* ctx);
static void     bfJsonSkipString(bfJsonParserContext* ctx);
static char     bfJsonUnescapeStringHelper(char character, const char** inc);
static size_t   bfJsonUnescapeString(char* str);
static void     bfJsonSetToken(bfJsonParserContext* ctx, bfJsonTokenType type, const char* bgn, const char* end);
static void     bfJsonNextToken(bfJsonParserContext* ctx);
static bfBool32 bfJsonEat(bfJsonParserContext* ctx, bfJsonTokenType type, bfBool32 optional);
static void     bfJsonInterpret(bfJsonParserContext* ctx);

void bfJsonParser_fromString(char* source, size_t source_length, bfJsonFn callback, void* user_data)
{
  bfJsonParserContext ctx;
  ctx.source_bgn                = source;
  ctx.source_end                = source + source_length;
  ctx.current_location          = source;
  ctx.line_no                   = 1;
  ctx.callback                  = callback;
  ctx.current_object.type       = BIFROST_JSON_EOF;
  ctx.current_object.source_bgn = NULL;
  ctx.current_object.source_end = NULL;
  ctx.user_data                 = user_data;
  ctx.document_storage.parent   = NULL;
  ctx.current_user_storage      = &ctx.document_storage;

  if (setjmp(ctx.error_handling) == 0)
  {
    bfJsonEat(&ctx, BIFROST_JSON_EOF, bfFalse);
    callback(&ctx, BIFROST_JSON_EVENT_BEGIN_DOCUMENT, user_data);
    bfJsonInterpret(&ctx);
    callback(&ctx, BIFROST_JSON_EVENT_END_DOCUMENT, user_data);
  }
}

const char* bfJsonParser_errorMessage(const bfJsonParserContext* ctx)
{
  return ctx->error_message;
}

bfJsonType bfJsonParser_valueType(const bfJsonParserContext* ctx)
{
  switch (ctx->current_object.type)
  {
    case BIFROST_JSON_QUOTE:
      return BIFROST_JSON_VALUE_STRING;
    case BIFROST_JSON_TRUE:
      return BIFROST_JSON_VALUE_BOOLEAN;
    case BIFROST_JSON_FALSE:
      return BIFROST_JSON_VALUE_BOOLEAN;
    case BIFROST_JSON_NULL:
      return BIFROST_JSON_VALUE_NULL;
    case BIFROST_JSON_NUMBER:
      return BIFROST_JSON_VALUE_NUMBER;
    case BIFROST_JSON_L_CURLY:
    case BIFROST_JSON_L_SQR_BOI:
    case BIFROST_JSON_R_CURLY:
    case BIFROST_JSON_R_SQR_BOI:
    case BIFROST_JSON_COMMA:
    case BIFROST_JSON_COLON:
    case BIFROST_JSON_EOF:
    default:
      return BIFROST_JSON_VALUE_STRING;
  }
}

bfBool32 bfJsonParser_valueIs(const bfJsonParserContext* ctx, bfJsonType type)
{
  return bfJsonParser_valueType(ctx) == type;
}

bfStringRange bfJsonParser_asString(const bfJsonParserContext* ctx)
{
  return bfMakeStringRangeLen(
   ctx->current_object.source_bgn,
   ctx->current_object.source_end - ctx->current_object.source_bgn);
}

double bfJsonParser_asNumber(const bfJsonParserContext* ctx)
{
  return strtod(ctx->current_object.source_bgn, NULL);
}

bfBool32 bfJsonParser_asBoolean(const bfJsonParserContext* ctx)
{
  return ctx->current_object.type != BIFROST_JSON_FALSE;
}

void* bfJsonParser_userStorage(const bfJsonParserContext* ctx)
{
  return ctx->current_user_storage->storage;
}

void* bfJsonParser_parentUserStorage(const bfJsonParserContext* ctx)
{
  return ctx->current_user_storage->parent->storage;
}

/*
  Writer API
*/

struct bfJsonStringBlock_t
{
  char                        string[BIFROST_JSON_STRING_BLOCK_SIZE];
  size_t                      string_length;
  struct bfJsonStringBlock_t* next;
};

typedef struct bfJsonWriter_t
{
  bfJsonAllocFn      alloc_fn;
  bfJsonStringBlock  base_block;
  bfJsonStringBlock* current_block;
  size_t             string_size;

} BifrostJsonWriter;

BifrostJsonWriter* bfJsonWriter_new(bfJsonAllocFn alloc_fn)
{
  BifrostJsonWriter* self = alloc_fn(sizeof(BifrostJsonWriter));

  self->alloc_fn                 = alloc_fn;
  self->base_block.string_length = 0;
  self->base_block.next          = NULL;
  self->current_block            = &self->base_block;
  self->string_size              = 0;

  return self;
}

size_t bfJsonWriter_length(const BifrostJsonWriter* self)
{
  return self->string_size;
}

void bfJsonWriter_beginArray(BifrostJsonWriter* self)
{
  bfJsonWriter_write(self, "[", 1);
}

void bfJsonWriter_endArray(BifrostJsonWriter* self)
{
  bfJsonWriter_write(self, "]", 1);
}

void bfJsonWriter_beginObject(BifrostJsonWriter* self)
{
  bfJsonWriter_write(self, "{", 1);
}

void bfJsonWriter_key(BifrostJsonWriter* self, bfStringRange key)
{
  bfJsonWriter_valueString(self, key);
  bfJsonWriter_write(self, " : ", 3);
}

void bfJsonWriter_valueString(BifrostJsonWriter* self, bfStringRange value)
{
  bfJsonWriter_write(self, "\"", 1);

  for (const char* it = value.bgn; it != value.end; ++it)
  {
    const char* write;
    size_t      write_length;

    switch (it[0])
    {
      case '"':
      {
        write        = "\\\"";
        write_length = 2;
        break;
      }
      case '\'':
      {
        write        = "\\\'";
        write_length = 2;
        break;
      }
      case '\n':
      {
        write        = "\\n";
        write_length = 2;
        break;
      }
      case '\r':
      {
        write        = "\\r";
        write_length = 2;
        break;
      }
      case '\t':
      {
        write        = "\\t";
        write_length = 2;
        break;
      }
      case '\\':
      {
        write        = "\\\\";
        write_length = 2;
        break;
      }
      default:
      {
        write        = it;
        write_length = 1;
        break;
      }
    }

    bfJsonWriter_write(self, write, write_length);
  }

  bfJsonWriter_write(self, "\"", 1);
}

void bfJsonWriter_valueNumber(BifrostJsonWriter* self, double value)
{
  char         number_buffer[22];
  const size_t number_buffer_length = (size_t)snprintf(number_buffer, sizeof(number_buffer), "%g", value);

  bfJsonWriter_write(self, number_buffer, number_buffer_length);
}

void bfJsonWriter_valueBoolean(BifrostJsonWriter* self, bfBool32 value)
{
  if (value)
  {
    bfJsonWriter_write(self, "true", 4);
  }
  else
  {
    bfJsonWriter_write(self, "false", 5);
  }
}

void bfJsonWriter_valueNull(BifrostJsonWriter* self)
{
  bfJsonWriter_write(self, "null", 4);
}

void bfJsonWriter_next(BifrostJsonWriter* self)
{
  bfJsonWriter_write(self, ",", 1);
}

void bfJsonWriter_indent(BifrostJsonWriter* self, int num_spaces)
{
  for (int i = 0; i < num_spaces; ++i)
  {
    bfJsonWriter_write(self, " ", 1);
  }
}

void bfJsonWriter_write(BifrostJsonWriter* self, const char* str, size_t length)
{
  while (length > 0)
  {
    const size_t bytes_left = bfSizeOfField(bfJsonStringBlock, string) - self->current_block->string_length;

    if (bytes_left < length)
    {
      for (size_t i = 0; i < bytes_left; ++i)
      {
        self->current_block->string[self->current_block->string_length++] = str[i];
      }

      bfJsonStringBlock* const new_block = self->alloc_fn(sizeof(bfJsonStringBlock));
      new_block->next                    = NULL;
      new_block->string_length           = 0;

      self->current_block->next = new_block;
      self->current_block       = new_block;

      str += bytes_left;
      length -= bytes_left;
      self->string_size += bytes_left;
      continue;
    }

    for (size_t i = 0; i < length; ++i)
    {
      self->current_block->string[self->current_block->string_length++] = str[i];
    }

    self->string_size += length;
    break;
  }
}

void bfJsonWriter_endObject(BifrostJsonWriter* self)
{
  bfJsonWriter_write(self, "}", 1);
}

void bfJsonWriter_forEachBlock(const BifrostJsonWriter* self, bfJsonWriterForEachFn fn, void* user_data)
{
  const bfJsonStringBlock* current = &self->base_block;

  while (current)
  {
    const bfJsonStringBlock* next = current->next;
    fn(current, user_data);
    current = next;
  }
}

void bfJsonWriter_delete(BifrostJsonWriter* self, bfJsonFreeFn free_fn)
{
  bfJsonStringBlock* current = self->base_block.next;

  while (current)
  {
    bfJsonStringBlock* next = current->next;
    free_fn(current);
    current = next;
  }

  free_fn(self);
}

bfStringRange bfJsonStringBlock_string(const bfJsonStringBlock* block)
{
  return bfMakeStringRangeLen(block->string, block->string_length);
}

/*
  Private Helpers
*/

/* Reader */
static bfBool32 bfJsonIsSpace(char character)
{
  return isspace(character);
}

static bfBool32 bfJsonIsSameCharacterI(char a, char b)
{
  return tolower(a) == tolower(b);
}

static bfBool32 bfJsonIsSameStringI(const char* a, const char* b, size_t n)
{
  for (size_t i = 0; i < n; ++i)
  {
    if (!bfJsonIsSameCharacterI(a[i], b[i]))
    {
      return bfFalse;
    }
  }

  return bfTrue;
}

static bfBool32 bfJsonIs(bfJsonParserContext* ctx, bfJsonTokenType type)
{
  return ctx->current_object.type == type;
}

static char bfJsonCurrentChar(const bfJsonParserContext* ctx)
{
  return ctx->current_location[0];
}

static void bfJsonIncrement(bfJsonParserContext* ctx)
{
  if (ctx->current_location < ctx->source_end)
  {
    ++ctx->current_location;

    if (bfJsonCurrentChar(ctx) == '\n' || bfJsonCurrentChar(ctx) == '\r')
    {
      ++ctx->line_no;
    }
  }
}

static void bfJsonSkipSpace(bfJsonParserContext* ctx)
{
  while (ctx->current_location < ctx->source_end && bfJsonIsSpace(bfJsonCurrentChar(ctx)))
  {
    bfJsonIncrement(ctx);
  }
}

static bfBool32 bfJsonIsKeyword(const bfJsonParserContext* ctx)
{
  const char character = bfJsonCurrentChar(ctx);

  return character == '(' || character == ')' || character == '_' || isalpha(character);
}

static void bfJsonSkipKeyword(bfJsonParserContext* ctx)
{
  while (ctx->current_location < ctx->source_end && bfJsonIsKeyword(ctx))
  {
    bfJsonIncrement(ctx);
  }
}

static bfBool32 bfJsonIsDigit(const bfJsonParserContext* ctx)
{
  const char character = bfJsonCurrentChar(ctx);

  return isdigit(character) ||
         character == '-' ||
         character == '+';
}

static bfBool32 bfJsonIsNumber(const bfJsonParserContext* ctx)
{
  const char character = bfJsonCurrentChar(ctx);

  /*
    @JsonSpecExtention
      Added support for other types of numbers such as
      hexadecimal and a trailing 'f' / 'F'.

      Supports anything that can be passed into 'strtod'.
  */

  return bfJsonIsDigit(ctx) ||
         character == '.' ||
         character == 'P' ||
         character == 'p' ||
         character == 'X' ||
         character == 'x' ||
         isxdigit(character); /* 0-9, a-f, A-F */
}

static void bfJsonSkipNumber(bfJsonParserContext* ctx)
{
  while (ctx->current_location < ctx->source_end && bfJsonIsNumber(ctx))
  {
    bfJsonIncrement(ctx);
  }
}

static void bfJsonSkipString(bfJsonParserContext* ctx)
{
  while (ctx->current_location < ctx->source_end && bfJsonCurrentChar(ctx) != BIFROST_JSON_QUOTE)
  {
    const char prev = bfJsonCurrentChar(ctx);
    bfJsonIncrement(ctx);

    if (prev == '\\')
    {
      bfJsonIncrement(ctx);
    }
    else if (bfJsonCurrentChar(ctx) == BIFROST_JSON_QUOTE)
    {
      break;
    }
  }
}

static char bfJsonUnescapeStringHelper(char character, const char** inc)
{
  /*
    @JsonSpecExtention
      Added support for some more escape characters.
  */

  const char* hex_start = *inc + 1;

  *inc += character == 'u' ? 4 : 1;

  switch (character)
  {
    case 'a': return '\a';  /* EXT  */
    case 'b': return '\b';  /* SPEC */
    case 'f': return '\f';  /* SPEC */
    case 'n': return '\n';  /* SPEC */
    case 'r': return '\r';  /* SPEC */
    case 't': return '\t';  /* SPEC */
    case 'v': return '\v';  /* EXT  */
    case '\\': return '\\'; /* SPEC */
    case '\'': return '\''; /* EXT  */
    case '\"': return '\"'; /* SPEC */
    case '/': return '/';   /* SPEC */
    case '?': return '\?';  /* EXT  */
    case 'u':               /* SPEC */
    {
      const char hex_buffer[] =
       {
        hex_start[0],
        hex_start[1],
        hex_start[2],
        hex_start[3],
        '\0',
       };

      return (char)strtoul(hex_buffer, NULL, 16);
    }
    default: return character;
  }
}

static size_t bfJsonUnescapeString(char* str)
{
  const char* old_str = str;
  char*       new_str = str;

  while (*old_str)
  {
    char c = old_str[0];
    ++old_str;

    if (c == '\\')
    {
      c = bfJsonUnescapeStringHelper(old_str[0], &old_str);

      if (c == '\0')
      {
        break;
      }
    }

    *new_str++ = c;
  }

  new_str[0] = '\0';

  return new_str - str;
}

static void bfJsonSetToken(bfJsonParserContext* ctx, bfJsonTokenType type, const char* bgn, const char* end)
{
  ctx->current_object.type       = type;
  ctx->current_object.source_bgn = bgn;
  ctx->current_object.source_end = end;
}

static void bfJsonNextToken(bfJsonParserContext* ctx)
{
  bfJsonSkipSpace(ctx);

  if (ctx->current_location < ctx->source_end)
  {
    if (bfJsonIsKeyword(ctx))
    {
      const char* token_bgn = ctx->current_location;
      bfJsonSkipKeyword(ctx);
      const char* token_end = ctx->current_location;

      /*
        @JsonSpecExtention
          Added support for "inf", "infinity", and "nan" (case insensitive).
      */
      if (token_end - token_bgn >= 3 && (bfJsonIsSameStringI(token_bgn, "INF", 3) || bfJsonIsSameStringI(token_bgn, "NAN", 3)))
      {
        bfJsonSetToken(ctx, BIFROST_JSON_NUMBER, token_bgn, token_end);
      }
      else
      {
        bfJsonSetToken(ctx, token_bgn[0], token_bgn, token_end);
      }
    }
    else if (bfJsonIsDigit(ctx))
    {
      const char* token_bgn = ctx->current_location;
      bfJsonSkipNumber(ctx);
      const char* token_end = ctx->current_location;

      bfJsonSetToken(ctx, BIFROST_JSON_NUMBER, token_bgn, token_end);
    }
    else if (bfJsonCurrentChar(ctx) == BIFROST_JSON_QUOTE)
    {
      bfJsonIncrement(ctx); /* '"' */
      char* token_bgn = ctx->current_location;
      bfJsonSkipString(ctx);
      char* token_end = ctx->current_location;
      token_end[0]    = '\0';
      token_end       = token_bgn + bfJsonUnescapeString(token_bgn);
      bfJsonIncrement(ctx); /* '"' (really '\0') */

      bfJsonSetToken(ctx, BIFROST_JSON_QUOTE, token_bgn, token_end);
    }
    else
    {
      bfJsonSetToken(ctx, ctx->current_location[0], NULL, NULL);
      bfJsonIncrement(ctx);
    }
  }
  else
  {
    bfJsonSetToken(ctx, BIFROST_JSON_EOF, NULL, NULL);
  }
}

static bfBool32 bfJsonEat(bfJsonParserContext* ctx, bfJsonTokenType type, bfBool32 optional)
{
  if (bfJsonIs(ctx, type))
  {
    bfJsonNextToken(ctx);
    return bfTrue;
  }

  if (!optional)
  {
    snprintf(ctx->error_message,
             sizeof(ctx->error_message),
             "Line(%i): Expected a '%c' but got a '%c'.",
             (int)ctx->line_no,
             (char)type,
             (char)ctx->current_object.type);
    ctx->callback(ctx, BIFROST_JSON_EVENT_PARSE_ERROR, ctx->user_data);
    longjmp(ctx->error_handling, 1);
    /* return bfFalse; */
  }

  return optional;
}

static void bfJsonInterpret(bfJsonParserContext* ctx)
{
  switch (ctx->current_object.type)
  {
    case BIFROST_JSON_L_CURLY:
    {
      bfJsonUserStorage user_storage;

      bfJsonEat(ctx, BIFROST_JSON_L_CURLY, bfFalse);

      user_storage.parent       = ctx->current_user_storage;
      ctx->current_user_storage = &user_storage;

      ctx->callback(ctx, BIFROST_JSON_EVENT_BEGIN_OBJECT, ctx->user_data);

      while (!bfJsonIs(ctx, BIFROST_JSON_R_CURLY))
      {
        // bfStringRange key = bfJsonCurrentString(ctx);

        ctx->callback(ctx, BIFROST_JSON_EVENT_KEY, ctx->user_data);
        bfJsonEat(ctx, BIFROST_JSON_QUOTE, bfFalse);
        bfJsonEat(ctx, BIFROST_JSON_COLON, bfFalse);
        bfJsonInterpret(ctx);

        /*
          @JsonSpecExtention
            Added support for trailing commas (allowed in ES5 though).
            Also commas are optional.
        */
        bfJsonEat(ctx, BIFROST_JSON_COMMA, bfTrue);
      }

      ctx->callback(ctx, BIFROST_JSON_EVENT_END_OBJECT, ctx->user_data);
      bfJsonEat(ctx, BIFROST_JSON_R_CURLY, bfFalse);

      ctx->current_user_storage = ctx->current_user_storage->parent;
      break;
    }
    case BIFROST_JSON_L_SQR_BOI:
    {
      bfJsonUserStorage user_storage;

      user_storage.parent       = ctx->current_user_storage;
      ctx->current_user_storage = &user_storage;

      bfJsonEat(ctx, BIFROST_JSON_L_SQR_BOI, bfFalse);
      ctx->callback(ctx, BIFROST_JSON_EVENT_BEGIN_ARRAY, ctx->user_data);

      while (!bfJsonIs(ctx, BIFROST_JSON_R_SQR_BOI))
      {
        bfJsonInterpret(ctx);

        /*
          @JsonSpecExtention
            Added support for trailing commas (allowed in ES5 though).
            Also commas are optional.
        */
        bfJsonEat(ctx, BIFROST_JSON_COMMA, bfTrue);
      }

      ctx->callback(ctx, BIFROST_JSON_EVENT_END_ARRAY, ctx->user_data);
      bfJsonEat(ctx, BIFROST_JSON_R_SQR_BOI, bfFalse);

      ctx->current_user_storage = ctx->current_user_storage->parent;
      break;
    }
    case BIFROST_JSON_QUOTE:
    case BIFROST_JSON_TRUE:
    case BIFROST_JSON_FALSE:
    case BIFROST_JSON_NULL:
    case BIFROST_JSON_NUMBER:
    {
      ctx->callback(ctx, BIFROST_JSON_EVENT_VALUE, ctx->user_data);
      /* [[fallthough]] */
    }
    case BIFROST_JSON_R_CURLY:
    case BIFROST_JSON_R_SQR_BOI:
    case BIFROST_JSON_COMMA:
    case BIFROST_JSON_COLON:
    case BIFROST_JSON_EOF:
    default:
    {
      bfJsonEat(ctx, ctx->current_object.type, bfFalse);
      break;
    }
  }
}
