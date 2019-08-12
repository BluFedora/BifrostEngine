#ifndef BIFROST_HASH_MAP_H
#define BIFROST_HASH_MAP_H

#include <stddef.h> /* size_t */

#if __cplusplus
extern "C"
{
#endif
  #define BIFROST_HASH_MAP_BUCKET_SIZE 128

  typedef void     (*bfHashMapDtor)(void* key, void* value);
  typedef unsigned (*bfHashMapHash)(const void* key);
  typedef int      (*bfHashMapCmp)(const void* lhs, const void* rhs);

  struct bfHashNode_t;
  typedef struct bfHashNode_t bfHashNode;

  typedef struct
  {
    const void* key;
    void*       value;
    int         index;
    bfHashNode* next;

  } bfHashMapIter;

  typedef struct
  {
    bfHashMapDtor dtor;
    bfHashMapHash hash;
    bfHashMapCmp  cmp;
    size_t        value_size;

  } BifrostHashMapParams;

  /*
    NOTE(Shareef):
      The defaults assume the following.

      dtor - Does nothing and assumes ypu will handle deallcation of
             key and values yourself before you call "bfHashMap_remove"
             or "bfHashMap_clear". Easiest way is through the iterator API.

      hash - Assumes the keys are nul terminated strings. So if you use a
             different datatype you MUST pass in a valid hash function. 

      cmp  - Like [hash] assumes a nul terminated string and will compare each
             character. So if you use a different datatype you MUST pass in a 
             valid compare function.

      value_size - By default is the size of a pointer.
  */
  void bfHashMapParams_init(BifrostHashMapParams* self);

  typedef struct 
  {
    BifrostHashMapParams params;
    bfHashNode*          buckets[BIFROST_HASH_MAP_BUCKET_SIZE];
    unsigned             num_buckets; //!< For if I ever switch to a dynamic bucket size.
    
  } BifrostHashMap;

  BifrostHashMap* bfHashMap_new(const BifrostHashMapParams* params);
  void            bfHashMap_ctor(BifrostHashMap* self, const BifrostHashMapParams* params);
  void            bfHashMap_set(BifrostHashMap* self, const void* key, void* value);
  int             bfHashMap_has(const BifrostHashMap* self, const void* key);
  void*           bfHashMap_get(BifrostHashMap* self, const void* key);
  void            bfHashMap_remove(BifrostHashMap* self, const void* key);
  bfHashMapIter   bfHashMap_itBegin(const BifrostHashMap* self);
  int             bfHashMap_itHasNext(const bfHashMapIter* it);
  void            bfHashMap_itGetNext(const BifrostHashMap* self, bfHashMapIter* it);
  void            bfHashMap_clear(BifrostHashMap* self);
  void            bfHashMap_dtor(BifrostHashMap* self);
  void            bfHashMap_delete(BifrostHashMap* self);

  #define bfHashMapFor(it, map)                            \
    for (bfHashMapIter it = bfHashMap_itBegin(map);        \
                            bfHashMap_itHasNext(&it);      \
                            bfHashMap_itGetNext(map, &it))
#if __cplusplus
}
#endif

#endif /* BIFROST_HASH_MAP_H */