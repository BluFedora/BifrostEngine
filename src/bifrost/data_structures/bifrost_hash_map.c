#include "bifrost/data_structures/bifrost_hash_map.h"

// #include "bifrost_std.h" /* bfStructHack */

#ifndef bfStructHack
  #if __STDC_VERSION__ >= 199901L
    // In C99, a flexible array member is just "[]".
    #define bfStructHack 
  #else
    // Elsewhere, use a zero-sized array. It's technically undefined behavior,
    // but works reliably in most known compilers.
    #define bfStructHack 0
  #endif
#endif

#include <assert.h> /* assert             */
#include <stdlib.h> /* malloc, free, NULL */
#include <string.h> /* strcmp             */

struct bfHashNode_t
{
  const void* key;
  bfHashNode* next;
  char        value[bfStructHack];
};

static bfHashNode* bfHashMap_newNode(const void *key, size_t value_size, void *value, bfHashNode* next);
static bfHashNode* bfHashMap_getNode(const BifrostHashMap* self, const void *key, unsigned hash);
static void        bfHashMap_deleteNode(const BifrostHashMap* self, bfHashNode* node);
static void        bfHashMap_defaultDtor(void* key, void* value);
static unsigned    bfHashMap_defaultHash(const void* key);
static int         bfHashMap_defaultCmp(const void* lhs, const void* rhs);

void bfHashMapParams_init(BifrostHashMapParams* self)
{
  self->dtor       = bfHashMap_defaultDtor;
  self->hash       = bfHashMap_defaultHash;
  self->cmp        = bfHashMap_defaultCmp;
  self->value_size = sizeof(void*);
}

BifrostHashMap* bfHashMap_new(const BifrostHashMapParams* params)
{
  BifrostHashMap* self = malloc(sizeof(BifrostHashMap));

  if (self)
  {
    bfHashMap_ctor(self, params);
  }

  return self;
}

void bfHashMap_ctor(BifrostHashMap* self, const BifrostHashMapParams* params)
{
  self->params      = *params;
  self->num_buckets = BIFROST_HASH_MAP_BUCKET_SIZE;

  for (unsigned i = 0; i < self->num_buckets; ++i)
  {
    self->buckets[i] = NULL;
  }
}

void bfHashMap_set(BifrostHashMap* self, const void* key, void* value)
{
  unsigned hash    = self->params.hash(key) % self->num_buckets;
  bfHashNode* node = bfHashMap_getNode(self, key, hash);

  if (node)
  {
    self->params.dtor((void*)node->key, node->value);
    node->key = key;
    memcpy(node->value, value, self->params.value_size);
  }
  else
  {
    self->buckets[hash] = bfHashMap_newNode(key, self->params.value_size, value, self->buckets[hash]);
  }
}

int bfHashMap_has(const BifrostHashMap* self, const void* key)
{
  unsigned hash = self->params.hash(key) % self->num_buckets;
  return bfHashMap_getNode(self, key, hash) != NULL;
}

void* bfHashMap_get(BifrostHashMap* self, const void* key)
{
  unsigned hash    = self->params.hash(key) % self->num_buckets;
  bfHashNode* node = bfHashMap_getNode(self, key, hash);

  return node ? node->value : NULL;
}

void bfHashMap_remove(BifrostHashMap* self, const void* key)
{
  unsigned hash      = self->params.hash(key) % self->num_buckets;
  bfHashNode* cursor = self->buckets[hash];
  bfHashNode* prev   = NULL;

  while (cursor)
  {
    if (self->params.cmp(key, cursor->key))
    {
      if (prev)
      {
        prev->next = cursor->next;
      }
      else
      {
        self->buckets[hash] = cursor->next;
      }
      
      bfHashMap_deleteNode(self, cursor);
      break;
    }

    prev   = cursor;
    cursor = cursor->next;
  }
}

bfHashMapIter bfHashMap_itBegin(const BifrostHashMap* self)
{
  bfHashMapIter begin_it = { NULL, NULL, -1, NULL };

  for (int i = 0; i < (int)self->num_buckets; ++i)
  {
    bfHashNode* cursor = self->buckets[i];

    if (cursor)
    {
      begin_it.key   = cursor->key;
      begin_it.value = cursor->value;
      begin_it.index = i;
      begin_it.next  = cursor->next;
      break;
    }
  }

  return begin_it;
}

int bfHashMap_itHasNext(const bfHashMapIter* it)
{
  return it->index != -1 && it->key != NULL;
}

void bfHashMap_itGetNext(const BifrostHashMap* self, bfHashMapIter* it)
{
  if (it->next)
  {
    it->key   = it->next->key;
    it->value = it->next->value;
    it->next  = it->next->next;
  }
  else
  {
    for (int i = (it->index + 1); i < (int)self->num_buckets; ++i)
    {
      bfHashNode* cursor = self->buckets[i];

      if (cursor)
      {
        it->key   = cursor->key;
        it->value = cursor->value;
        it->index = i;
        it->next  = cursor->next;
        return;
      }
    }
    it->key   = NULL;
    it->index = -1;
  }
}

void bfHashMap_clear(BifrostHashMap* self)
{
  for (unsigned i = 0; i < self->num_buckets; ++i)
  {
    bfHashNode* cursor = self->buckets[i];

    while (cursor)
    {
      bfHashNode* next = cursor->next;
      bfHashMap_deleteNode(self, cursor);
      cursor = next;
    }

    self->buckets[i] = NULL;
  }
}

void bfHashMap_dtor(BifrostHashMap* self)
{
  bfHashMap_clear(self);
}

void bfHashMap_delete(BifrostHashMap* self)
{
  bfHashMap_dtor(self);
  free(self);
}

static bfHashNode* bfHashMap_newNode(const void *key, size_t value_size, void *value, bfHashNode* next)
{
  bfHashNode* node = (bfHashNode *)malloc(sizeof(bfHashNode) + value_size);

  if (node)
  {
    node->key   = key;
    node->next  = next;
    memcpy(node->value, value, value_size);
  }
  else
  {
    assert(!"Failed to alloc memory for a bfHashNode.");
  }
  
  return node;
}

static bfHashNode* bfHashMap_getNode(const BifrostHashMap* self, const void *key, unsigned hash)
{
  bfHashNode* cursor = self->buckets[hash];

  while (cursor)
  {
    if (self->params.cmp(key, cursor->key))
    {
      break;
    }

    cursor = cursor->next;
  }

  return cursor;
}

static void bfHashMap_deleteNode(const BifrostHashMap* self, bfHashNode* node)
{
  self->params.dtor((void*)node->key, node->value);
  free(node);
}

static void bfHashMap_defaultDtor(void* key, void* value)
{
  (void) key;
  (void) value;
}

static unsigned bfHashMap_defaultHash(const void* key)
{
  const char* cp = (const char *)key;
  unsigned    hash = 0x811c9dc5;

  while (*cp) {
      hash ^= (unsigned char) *cp++;
      hash *= 0x01000193;
  }

  return hash;
}

static int bfHashMap_defaultCmp(const void* lhs, const void* rhs)
{
  const char* str1 = (const char*)lhs;
  const char* str2 = (const char*)rhs;

  return strcmp(str1, str2) == 0;
}
