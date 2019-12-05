#include "bifrost_vm_gc.h"

#include "bifrost/bifrost_vm.h"                       // BifrostVM_t
#include "bifrost/data_structures/bifrost_array_t.h"  // Array_size
#include "bifrost_vm_function_builder.h"              // BifrostVMFunctionBuilder
#include "bifrost_vm_obj.h"                           // BifrostObj
#include "bifrost_vm_parser.h"                        // BifrostParser
#include "bifrost_vm_value.h"                         // bfVMValue
#include <assert.h>                                   /* assert        */
#include <stdlib.h>                                   /* realloc, free */

/* TODO(SR):
  @Optimization:
    A new trick: in the sweep phase, you don't need to reset the mark.
    You can just use another integer as the "traversed" mark for the next traversal.
    You can increment the mark at each traversal,
    and if you're not comfortable with overflows wrap it around a fixed bound
    that must be higher than the total number of colors you need, plus one.
    This saves you one write per non-freed object, which is nice.
*/

static inline size_t bfGCFinalizePostMark(BifrostVM* self);
static inline void   bfGCMarkValue(bfVMValue value);
static inline void   bfGCMarkValues(bfVMValue* values);
static void          bfGCMarkObj(BifrostObj* obj);
static inline void   bfGCMarkSymbols(BifrostVMSymbol* symbols);
static inline size_t bfGCObjectSize(BifrostObj* obj);
static inline void   bfGCFinalize(BifrostVM* self);

extern bfVMValue     bfVM_getHandleValue(bfValueHandle h);
extern bfValueHandle bfVM_getHandleNext(bfValueHandle h);
extern uint32_t      bfVM_getSymbol(BifrostVM* self, bfStringRange name);

void bfGCMarkObjects(struct BifrostVM_t* self)
{
  const size_t stack_size = Array_size(&self->stack);

  for (size_t i = 0; i < stack_size; ++i)
  {
    bfGCMarkValue(self->stack[i]);
  }

  const size_t frames_size = Array_size(&self->frames);

    // TODO(SR): Is this really needed?
  for (size_t i = 0; i < frames_size; ++i)
  {
    BifrostObjFn* const fn = self->frames[i].fn;

    if (fn)
    {
      bfGCMarkObj(&fn->super);
    }
  }

  bfHashMapFor(it, &self->modules)
  {
    BifrostObjStr* const key = (void*)it.key;

    bfGCMarkObj(&key->super);
    bfGCMarkObj(*(void**)it.value);
  }

  bfValueHandle cursor = self->handles;

  while (cursor)
  {
    bfGCMarkValue(bfVM_getHandleValue(cursor));
    cursor = bfVM_getHandleNext(cursor);
  }

  BifrostParser* parsers = self->parser_stack;

  while (parsers)
  {
    if (parsers->current_module)
    {
      bfGCMarkObj(&parsers->current_module->super);
    }

    if (parsers->current_clz)
    {
      bfGCMarkObj(&parsers->current_clz->super);
    }

    const size_t num_builders = Array_size(&parsers->fn_builder_stack);

    for (size_t i = 0; i < num_builders; ++i)
    {
      BifrostVMFunctionBuilder* builder = parsers->fn_builder_stack + i;

      if (builder->constants)
      {
        bfGCMarkValues(builder->constants);
      }
    }

    parsers = parsers->parent;
  }

  for (uint8_t i = 0; i < self->temp_roots_top; ++i)
  {
    bfGCMarkObj(self->temp_roots[i]);
  }
}

size_t bfGCSweep(struct BifrostVM_t* self)
{
  BifrostObj** cursor       = &self->gc_object_list;
  BifrostObj*  garbage_list = NULL;

  size_t collected_bytes = 0u;

  while (*cursor)
  {
    if (!(*cursor)->gc_mark)
    {
      BifrostObj* garbage = *cursor;
      *cursor             = garbage->next;

      garbage->next = garbage_list;
      garbage_list  = garbage;

      collected_bytes += bfGCObjectSize(garbage);
    }
    else
    {
      (*cursor)->gc_mark = 0;
      cursor             = &(*cursor)->next;
    }
  }

  BifrostObj* g_cursor = garbage_list;

  while (g_cursor)
  {
    BifrostObj* const next = g_cursor->next;

    if (g_cursor->type == BIFROST_VM_OBJ_INSTANCE)
    {
      bfObjFinalize(self, (BifrostObjInstance*)g_cursor);
    }

    g_cursor = next;
  }

  g_cursor = garbage_list;

  //*
  while (g_cursor)
  {
    BifrostObj* const next = g_cursor->next;
#if 1
    if (g_cursor->type == BIFROST_VM_OBJ_INSTANCE)
    {
      BifrostObjInstance* const inst   = (BifrostObjInstance*)g_cursor;
      BifrostObjClass* const    clz    = inst->clz;
      const uint32_t            symbol = bfVM_getSymbol(self, bfMakeStringRangeC("dtor"));

      if (symbol < Array_size(&clz->symbols))
      {
        const bfVMValue value = clz->symbols[symbol].value;

        if (IS_POINTER(value))
        {
          if (bfObjIsFunction(AS_POINTER(value)))
          {
            inst->super.next = &self->finalized->super;
            self->finalized  = inst;

            collected_bytes -= bfGCObjectSize(g_cursor);

            /*
              NOTE(Shareef):
                Don't GC Just Yet.
             */
            g_cursor = next;
            continue;
          }
        }
      }
    }
#endif
    bfVMObject_delete(self, g_cursor);
    g_cursor = next;
  }
  //*/
  return collected_bytes;
}

void bfGCCollect(struct BifrostVM_t* self)
{
  bfGCMarkObjects(self);
  size_t collected_bytes = bfGCFinalizePostMark(self);
  collected_bytes += bfGCSweep(self);

  self->bytes_allocated -= collected_bytes;

  const size_t new_heap_size = self->bytes_allocated + (size_t)(self->bytes_allocated * self->params.heap_growth_factor);
  const size_t min_heap_size = self->params.min_heap_size;
  self->params.heap_size     = new_heap_size > min_heap_size ? new_heap_size : min_heap_size;

  bfGCFinalize(self);
}

#include <string.h>

void* bfGCDefaultAllocator(void* user_data, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
  (void)user_data;
  (void)alignment;
  (void)old_size;

  /*
    NOTE(Shareef):
      "if new_size is zero, the behavior is implementation defined
      (null pointer may be returned
      (in which case the old memory block may or may not be freed),
      or some non-null pointer may be returned that may
      not be used to access storage)."
  */
  if (new_size == 0u)
  {
    memset(ptr, 0xCC, old_size);
    free(ptr);
    ptr = NULL;
  }
  else
  {
    void* const new_ptr = realloc(ptr, new_size);

    if (!new_ptr)
    {
      /*
        NOTE(Shareef):
          As to not leak memory, realloc says:
            "If there is not enough memory,
            the old memory block is not freed and null pointer is returned."
      */
      free(ptr);
    }

    ptr = new_ptr;
  }

  return ptr;
}

void* bfGCAllocMemory(struct BifrostVM_t* self, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
  if (new_size == 0u)
  {
    self->bytes_allocated -= old_size;
  }
  else
  {
    self->bytes_allocated += new_size;

    if (self->bytes_allocated >= self->params.heap_size)
    {
      bfVM_gc(self);
    }
  }

  return (self->params.memory_fn)(self->params.user_data, ptr, old_size, new_size, alignment);
}

void bfGCPushRoot(struct BifrostVM_t* self, struct BifrostObj_t* obj)
{
  assert(self->temp_roots_top < bfCArraySize(self->temp_roots));
  self->temp_roots[self->temp_roots_top++] = obj;
}

void bfGCPopRoot(struct BifrostVM_t* self)
{
  --self->temp_roots_top;
}

static inline size_t bfGCFinalizePostMark(BifrostVM* self)
{
  BifrostObjInstance** cursor          = &self->finalized;
  size_t               collected_bytes = 0u;

  while (*cursor)
  {
    if (!(*cursor)->super.gc_mark)
    {
      BifrostObjInstance* garbage = *cursor;
      *cursor                     = (BifrostObjInstance*)garbage->super.next;

      collected_bytes += bfGCObjectSize(&garbage->super);
      bfVMObject_delete(self, &garbage->super);
    }
    else
    {
      (*cursor)->super.gc_mark = 0;
      cursor                   = (BifrostObjInstance**)&((*cursor)->super.next);
    }
  }

  return collected_bytes;
}

static inline void bfGCMarkValue(bfVMValue value)
{
  if (IS_POINTER(value))
  {
    void* const ptr = AS_POINTER(value);

    if (ptr)
    {
      bfGCMarkObj(ptr);
    }
  }
}

static inline void bfGCMarkValues(bfVMValue* values)
{
  const size_t size = Array_size(&values);

  for (size_t i = 0; i < size; ++i)
  {
    bfGCMarkValue(values[i]);
  }
}

static void bfGCMarkObj(BifrostObj* obj)
{
  if (!obj->gc_mark)
  {
    obj->gc_mark = 1;

    switch (obj->type & BifrostVMObjType_mask)
    {
      case BIFROST_VM_OBJ_MODULE:
      {
        BifrostObjModule* module = (BifrostObjModule*)obj;
        bfGCMarkSymbols(module->variables);

        if (module->init_fn.name)
        {
          bfGCMarkObj(&module->init_fn.super);

          bfGCMarkValues(module->init_fn.constants);
        }
        break;
      }
      case BIFROST_VM_OBJ_CLASS:
      {
        BifrostObjClass* const clz = (BifrostObjClass*)obj;
        bfGCMarkObj(&clz->module->super);
        bfGCMarkSymbols(clz->symbols);
        bfGCMarkSymbols(clz->field_initializers);
        break;
      }
      case BIFROST_VM_OBJ_INSTANCE:
      {
        BifrostObjInstance* const inst = (BifrostObjInstance*)obj;
        bfGCMarkObj(&inst->clz->super);
        bfHashMapFor(it, &inst->fields)
        {
          bfGCMarkValue(*(bfVMValue*)it.value);
        }
        break;
      }
      case BIFROST_VM_OBJ_FUNCTION:
      {
        BifrostObjFn* const fn = (BifrostObjFn*)obj;
        bfGCMarkValues(fn->constants);
        break;
      }
      case BIFROST_VM_OBJ_NATIVE_FN:
        break;
      case BIFROST_VM_OBJ_STRING:
        break;
    }
  }
}

static inline void bfGCMarkSymbols(BifrostVMSymbol* symbols)
{
  const size_t size = Array_size(&symbols);

  for (size_t i = 0; i < size; ++i)
  {
    bfGCMarkValue(symbols[i].value);
  }
}

static inline size_t bfGCObjectSize(BifrostObj* obj)
{
  switch (obj->type & BifrostVMObjType_mask)
  {
    case BIFROST_VM_OBJ_MODULE:
    {
      return sizeof(BifrostObjModule);
    }
    case BIFROST_VM_OBJ_CLASS:
    {
      return sizeof(BifrostObjClass);
    }
    case BIFROST_VM_OBJ_INSTANCE:
    {
      return sizeof(BifrostObjInstance) + ((BifrostObjInstance*)obj)->clz->extra_data;
    }
    case BIFROST_VM_OBJ_FUNCTION:
    {
      return sizeof(BifrostObjFn);
    }
    case BIFROST_VM_OBJ_NATIVE_FN:
    {
      return sizeof(BifrostObjNativeFn);
    }
    case BIFROST_VM_OBJ_STRING:
    {
      return sizeof(BifrostObjStr);
    }
  }

  return 0u;
}

static inline void bfGCFinalize(BifrostVM* self)
{
  const uint32_t      symbol = bfVM_getSymbol(self, bfMakeStringRangeC("dtor"));
  BifrostObjInstance* cursor = self->finalized;

  while (cursor)
  {
    //*
    BifrostObjClass* const clz   = cursor->clz;
    const bfVMValue        value = clz->symbols[symbol].value;

    // TODO(SR):
    //   Investigate if this breaks some reentrycy model rules.
    //   As it seems these registers are cloggered.
    //   Solution?: NO GC While in a native fn?

    bfVMValue stack_restore[2];

    bfVM_stackResize(self, 2);
    stack_restore[0]   = self->stack_top[0];
    stack_restore[1]   = self->stack_top[1];
    self->stack_top[0] = value;
    self->stack_top[1] = FROM_POINTER(cursor);
    if (bfVM_stackGetType(self, 0) == BIFROST_VM_FUNCTION)
    {
      bfVM_call(self, 0, 1, 1);
    }
    self->stack_top[0] = stack_restore[0];
    self->stack_top[1] = stack_restore[1];
    //*/
    cursor = (BifrostObjInstance*)cursor->super.next;
  }
}
