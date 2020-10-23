#include <assert.h>
#include <setjmp.h> /* jmp_buf */
#include <stdio.h>  // getchar
#include <stdlib.h>

// Test Json

#include "bifrost/utility/bifrost_json.h"

static void test_JsonParser(void);

// Test Coroutine

static void test_Coroutine(void);

struct BifrostCoroutine_t;

typedef void (*bfCoroutineFn)(struct BifrostCoroutine_t* context);

typedef struct BifrostCoroutine_t
{
  jmp_buf       yield_buffer;     /*!< */
  bfCoroutineFn callback;         /*!< */
  char          stack_space[512]; /*!< */
  int           state;            /*!< */
  int           current_state;    /*!< */
  int           is_done;          /*!< boolean */

} BifrostCoroutine;

BifrostCoroutine* bfCoroutine_new(bfCoroutineFn callback);
void              bfCoroutine_call(BifrostCoroutine* self);
int               bfCoroutine_stateImpl(BifrostCoroutine* self, int line);
void              bfCoroutine_yield(BifrostCoroutine* self);
void              bfCoroutine_return(BifrostCoroutine* self);
void              bfCoroutine_delete(BifrostCoroutine* self);
#define bfCoroutine_state(self) bfCoroutine_stateImpl((self), __LINE__)

static void myCoroutine(BifrostCoroutine* ctx);

int main()
{
  test_JsonParser();
  test_Coroutine();
  (void)getchar();
  return 0;
}

#define bfQuoteStr(...) #__VA_ARGS__

typedef struct JsonUserContext_t
{
  int         root_object;
  int         current_object;
  const char* current_key;

} JsonUserContext;

static void myJsonEventHandler(bfJsonParserContext* ctx, bfJsonEvent event, void* user_data)
{
  printf("%p -> %p: ", bfJsonParser_parentUserStorage(ctx), bfJsonParser_userStorage(ctx));

  switch (event)
  {
    case BF_JSON_EVENT_BEGIN_ARRAY:
    {
      printf("[\n");
      break;
    }
    case BF_JSON_EVENT_END_ARRAY:
    {
      printf("]\n");
      break;
    }
    case BF_JSON_EVENT_BEGIN_OBJECT:
    {
      printf("{\n");
      break;
    }
    case BF_JSON_EVENT_END_OBJECT:
    {
      printf("}\n");
      break;
    }
    case BF_JSON_EVENT_PARSE_ERROR:
    {
      printf("Error: %s\n", bfJsonParser_errorMessage(ctx));
      break;
    }
    case BF_JSON_EVENT_BEGIN_DOCUMENT:
    {
      printf("DOCUMENT BEGIN\n");

      break;
    }
    case BF_JSON_EVENT_KEY:
    {
      const bfJsonString key = bfJsonParser_asString(ctx);

      printf("Key(%.*s)\n", (int)key.length, key.string);
      break;
    }
    case BF_JSON_EVENT_VALUE:
    {
      switch (bfJsonParser_valueType(ctx))
      {
        case BF_JSON_VALUE_STRING:
        {
          const bfJsonString value = bfJsonParser_asString(ctx);

          printf("Value(%.*s)\n", (int)value.length, value.string);
          break;
        }
        case BF_JSON_VALUE_NUMBER:
        {
          printf("Value(%f)\n", bfJsonParser_asNumber(ctx));
          break;
        }
        case BF_JSON_VALUE_BOOLEAN:
        {
          printf("Value(%s)\n", bfJsonParser_asBoolean(ctx) ? "true" : "false");
          break;
        }
        case BF_JSON_VALUE_NULL:
        {
          printf("Value(null)\n");
          break;
        }
        default:;
      }

      break;
    }
    case BF_JSON_EVENT_END_DOCUMENT:
    {
      printf("DOCUMENT END\n");
      break;
    }
    default:
    {
      break;
    }
  }
}

static void printBlock(const bfJsonStringBlock* block, void* ud)
{
  bfJsonString string = bfJsonStringBlock_string(block);

  printf("  Block(%.*s)\n", (int)string.length, string.string);
}

static void test_JsonParser(void)
{
  printf("Json Tests\n");

  char json_source[] = bfQuoteStr(
   {
     "MyKey" : 0.5,
     "Another Key" : [
       "Array Element",
       7.0
     ]
   });
  const size_t json_source_size = sizeof(json_source) - 1;

  JsonUserContext user_ctx;

  bfJsonParser_fromString(json_source, json_source_size, &myJsonEventHandler, &user_ctx);

  printf("Json Writer\n");

  bfJsonWriter* json_writer = bfJsonWriter_newCRTAlloc();
  {
    bfJsonWriter_beginArray(json_writer);
    {
      bfJsonWriter_beginObject(json_writer);
      {
        bfJsonWriter_key(json_writer, (bfJsonString){"Test Key", sizeof("Test Key") - 1});
        bfJsonWriter_valueNumber(json_writer, 75.43);
      }
      bfJsonWriter_endObject(json_writer);

      bfJsonWriter_next(json_writer);

      for (int i = 0; i < 200; ++i)
      {
        bfJsonWriter_valueNumber(json_writer, i * 3.2);
        bfJsonWriter_next(json_writer);
      }
    }
    bfJsonWriter_endArray(json_writer);

    bfJsonWriter_forEachBlock(json_writer, &printBlock, NULL);
  }
  bfJsonWriter_deleteCRT(json_writer);
}

#pragma region Coroutine

static void test_Coroutine(void)
{
  printf("Coroutine Tests\n");

  BifrostCoroutine* const coro = bfCoroutine_new(&myCoroutine);

  bfCoroutine_call(coro);
  printf("WILL THIS WORK 0\n");
  bfCoroutine_call(coro);
  printf("WILL THIS WORK 1\n");
  bfCoroutine_call(coro);
  printf("WILL THIS WORK 2\n");
  bfCoroutine_call(coro);

  bfCoroutine_delete(coro);
}

static void myCoroutine(BifrostCoroutine* ctx)
{
  if (bfCoroutine_state(ctx))
  {
    printf("Step 1\n");

    bfCoroutine_yield(ctx);
  }

  if (bfCoroutine_state(ctx))
  {
    printf("Step 2\n");

    bfCoroutine_yield(ctx);
  }

  if (bfCoroutine_state(ctx))
  {
    printf("Step 3\n");

    bfCoroutine_yield(ctx);
  }

  bfCoroutine_return(ctx);
}

#include <stdlib.h> /* malloc, free */
#include <string.h> /* memset       */

BifrostCoroutine* bfCoroutine_new(bfCoroutineFn callback)
{
  BifrostCoroutine* const self = malloc(sizeof(BifrostCoroutine));

  if (self)
  {
    memset(self, 0x0, sizeof(*self));
    self->callback = callback;
  }

  return self;
}

void bfCoroutine_call(BifrostCoroutine* self)
{
  if (self->is_done)
  {
    assert(!"bfCoroutine_call called on a coroutine that has finished running.");
    return;
  }
  switch (setjmp(self->yield_buffer))
  {
    case 0:
    {
      self->callback(self);
      break;
    }
    case 1:
    {
      self->is_done = 0;
      break;
    }
    case 2:
    {
      self->is_done = 1;
      break;
    }
    default:
    {
      break;
    }
  }
}

int bfCoroutine_stateImpl(BifrostCoroutine* self, int line)
{
  self->current_state = line;
  return self->state < line;
}

void bfCoroutine_yield(BifrostCoroutine* self)
{
  self->state = self->current_state;
  longjmp(self->yield_buffer, 1);
}

void bfCoroutine_return(BifrostCoroutine* self)
{
  longjmp(self->yield_buffer, 2);
}

void bfCoroutine_delete(BifrostCoroutine* self)
{
  free(self);
}

#pragma endregion
