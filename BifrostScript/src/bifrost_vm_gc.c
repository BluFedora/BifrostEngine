/******************************************************************************/
/*!
 * @file   bifrost_vm_gc.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @par
 *    Bifrost Scriping Language
 *
 * @brief
 *   A simple tracing garbage collector for the Bifrost Scripting Language.
 *   This uses a very basic mark and sweep algorithm.
 *
 *   NOTE(Shareef):
 *     The memory counted is exclusively what is allocated for objects and the VM
 *     struct itself.
 *     Ignored Allocations:
 *       > Any Array allocation from the Bifrost Array Data Structure.
 *       > Any HashMap allocation from the Bifrost Array Data Structure.
 *       > Any Dynamic String from the Bifrost Array Data Structure.
 *
 *     To fix this we need :
 *        > A unified memory model for both the scripting language
 *          and the data structures. The problem is that architectually the data
 *          structures are drop in libraries while the VM is a monolithic stateful
 *          object leading to different ways of customizing allcoations.
 *          If I do force the VM allocator on the data structures then if you
 *          use the Bifrost Scripting Language part of the Library you are forced.
 *          into it's memory model for the data structures.
 *
 *          (Maybe this is What Bifrost Needs??, Just one allocator for all.)
 *            (This once again leads to global state of storing the allocators.)
 *          (But then you a forced into the whole lib, that's the main problem.)
 *
 *        > To reimplement bare bone versions of the data structures just for the
 *          VM but not being allowed to reuse that code would be VERY inconvenient.
 *
 *   References:
 *     [http://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/]
 *
 *    Something to Think About Language Design Wise.
 *     [https://stackoverflow.com/questions/28320213/why-do-we-need-to-call-luas-collectgarbage-twice]
 *
 * @version 0.0.1-beta
 * @date    2019-07-01
 *
 * @copyright Copyright (c) 2019 Shareef Raheem
 */
/******************************************************************************/
#include "bifrost_vm_gc.h"

#include "bifrost/script/bifrost_vm.h"                               // BifrostVM_t
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

#define GC_MARK_UNREACHABLE 0
#define GC_MARK_REACHABLE 1
#define GC_MARK_FINALIZE 3

static size_t bfGCFinalizePostMark(BifrostVM* self);
static void   bfGCMarkValue(bfVMValue value, int mark_value);
static void   bfGCMarkValues(bfVMValue* values, int mark_value);
static void   bfGCMarkValuesN(bfVMValue* values, size_t size, int mark_value);
static void   bfGCMarkObj(BifrostObj* obj, int mark_value);
static void   bfGCMarkSymbols(BifrostVMSymbol* symbols, int mark_value);
static size_t bfGCObjectSize(BifrostObj* obj);
static void   bfGCFinalize(BifrostVM* self);

extern bfVMValue     bfVM_getHandleValue(bfValueHandle h);
extern bfValueHandle bfVM_getHandleNext(bfValueHandle h);
extern uint32_t      bfVM_getSymbol(BifrostVM* self, bfStringRange name);

void bfGCMarkObjects(struct BifrostVM_t* self)
{
  const size_t stack_size = Array_size(&self->stack);

  for (size_t i = 0; i < stack_size; ++i)
  {
    bfGCMarkValue(self->stack[i], GC_MARK_REACHABLE);
  }

  const size_t frames_size = Array_size(&self->frames);

  // TODO(SR): Is this really needed?
  for (size_t i = 0; i < frames_size; ++i)
  {
    BifrostObjFn* const fn = self->frames[i].fn;

    if (fn)
    {
      bfGCMarkObj(&fn->super, GC_MARK_REACHABLE);
    }
  }

  bfHashMapFor(it, &self->modules)
  {
    BifrostObjStr* const key = (void*)it.key;

    bfGCMarkObj(&key->super, GC_MARK_REACHABLE);
    bfGCMarkObj(*(BifrostObj**)it.value, GC_MARK_REACHABLE);
  }

  bfValueHandle cursor = self->handles;

  while (cursor)
  {
    bfGCMarkValue(bfVM_getHandleValue(cursor), GC_MARK_REACHABLE);
    cursor = bfVM_getHandleNext(cursor);
  }

  BifrostParser* parsers = self->parser_stack;

  while (parsers)
  {
    if (parsers->current_module)
    {
      bfGCMarkObj(&parsers->current_module->super, GC_MARK_REACHABLE);
    }

    if (parsers->current_clz)
    {
      bfGCMarkObj(&parsers->current_clz->super, GC_MARK_REACHABLE);
    }

    const size_t num_builders = Array_size(&parsers->fn_builder_stack);

    for (size_t i = 0; i < num_builders; ++i)
    {
      BifrostVMFunctionBuilder* builder = parsers->fn_builder_stack + i;

      if (builder->constants)
      {
        bfGCMarkValues(builder->constants, GC_MARK_REACHABLE);
      }
    }

    parsers = parsers->parent;
  }

  for (uint8_t i = 0; i < self->temp_roots_top; ++i)
  {
    bfGCMarkObj(self->temp_roots[i], GC_MARK_REACHABLE);
  }
}

size_t bfGCSweep(struct BifrostVM_t* self)
{
  BifrostObj** cursor       = &self->gc_object_list;
  BifrostObj*  garbage_list = NULL;

  size_t collected_bytes = 0u;

  while (*cursor)
  {
    if ((*cursor)->gc_mark == GC_MARK_UNREACHABLE)
    {
      BifrostObj* garbage = *cursor;
      *cursor             = garbage->next;

      garbage->next = garbage_list;
      garbage_list  = garbage;

      collected_bytes += bfGCObjectSize(garbage);
    }
    else
    {
      (*cursor)->gc_mark = GC_MARK_UNREACHABLE;
      cursor             = &(*cursor)->next;
    }
  }

  BifrostObj* g_cursor      = garbage_list;
  BifrostObj* g_cursor_prev = NULL;

  // NOTE: Instances must be destroyed before classes.

  //
  // TODO(Shareef): Make this code nicer...
  //   Need to find a better more consistent want to handle finalizers.

  while (g_cursor)
  {
    BifrostObj* const next = g_cursor->next;

    if (g_cursor->type == BIFROST_VM_OBJ_INSTANCE || g_cursor->type == BIFROST_VM_OBJ_REFERENCE)
    {
      BifrostObjInstance* const inst   = (BifrostObjInstance*)g_cursor;
      BifrostObjClass* const    clz    = inst->clz;
      const uint32_t            symbol = bfVM_getSymbol(self, bfMakeStringRangeC("dtor"));

      if (clz && symbol < Array_size(&clz->symbols))
      {
        const bfVMValue value = clz->symbols[symbol].value;

        if (IS_POINTER(value) && bfObjIsFunction(AS_POINTER(value)))
        {
          bfGCMarkObj(g_cursor, GC_MARK_FINALIZE);

          collected_bytes -= bfGCObjectSize(g_cursor);
        }
      }

      bfObjFinalize(self, g_cursor);

      if (g_cursor->gc_mark != GC_MARK_FINALIZE)
      {
        if (g_cursor_prev)
        {
          g_cursor_prev->next = next;
        }

        if (garbage_list == g_cursor)
        {
          garbage_list = next;
        }

        bfVMObject_delete(self, g_cursor);

        g_cursor = next;
        continue;
      }
    }

    g_cursor_prev = g_cursor;
    g_cursor      = next;
  }

  g_cursor = garbage_list;

  while (g_cursor)
  {
    BifrostObj* const next = g_cursor->next;

    if (g_cursor->gc_mark == GC_MARK_UNREACHABLE)
    {
      bfVMObject_delete(self, g_cursor);
    }
    else if (g_cursor->gc_mark == GC_MARK_FINALIZE)
    {
      g_cursor->next  = self->finalized;
      self->finalized = g_cursor;
    }

    g_cursor = next;
  }

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

static size_t bfGCFinalizePostMark(BifrostVM* self)
{
  BifrostObj** cursor          = &self->finalized;
  size_t       collected_bytes = 0u;

  while (*cursor)
  {
    if (!(*cursor)->gc_mark)
    {
      BifrostObj* garbage = *cursor;
      *cursor             = garbage->next;

      collected_bytes += bfGCObjectSize(garbage);
      bfVMObject_delete(self, garbage);
    }
    else
    {
      (*cursor)->gc_mark = 6;
      cursor             = &(*cursor)->next;
    }
  }

  return collected_bytes;
}

static void bfGCMarkValue(bfVMValue value, int mark_value)
{
  if (IS_POINTER(value))
  {
    void* const ptr = AS_POINTER(value);

    if (ptr)
    {
      bfGCMarkObj(ptr, mark_value);
    }
  }
}

static void bfGCMarkValues(bfVMValue* values, int mark_value)
{
  bfGCMarkValuesN(values, Array_size(&values), mark_value);
}

static void bfGCMarkValuesN(bfVMValue* values, size_t size, int mark_value)
{
  for (size_t i = 0; i < size; ++i)
  {
    bfGCMarkValue(values[i], mark_value);
  }
}

static void bfGCMarkObj(BifrostObj* obj, int mark_value)
{
  if (!obj->gc_mark)
  {
    obj->gc_mark = mark_value;

    switch (obj->type & BifrostVMObjType_mask)
    {
      case BIFROST_VM_OBJ_MODULE:
      {
        BifrostObjModule* module = (BifrostObjModule*)obj;
        bfGCMarkSymbols(module->variables, mark_value);

        if (module->init_fn.name)
        {
          bfGCMarkObj(&module->init_fn.super, mark_value);
          bfGCMarkValues(module->init_fn.constants, mark_value);
        }
        break;
      }
      case BIFROST_VM_OBJ_CLASS:
      {
        BifrostObjClass* const clz = (BifrostObjClass*)obj;

        if (clz->base_clz)
        {
          bfGCMarkObj(&clz->base_clz->super, mark_value);
        }

        bfGCMarkObj(&clz->module->super, mark_value);
        bfGCMarkSymbols(clz->symbols, mark_value);
        bfGCMarkSymbols(clz->field_initializers, mark_value);
        break;
      }
      case BIFROST_VM_OBJ_INSTANCE:
      {
        BifrostObjInstance* const inst = (BifrostObjInstance*)obj;
        bfGCMarkObj(&inst->clz->super, mark_value);
        bfHashMapFor(it, &inst->fields)
        {
          bfGCMarkValue(*(bfVMValue*)it.value, mark_value);
        }
        break;
      }
      case BIFROST_VM_OBJ_FUNCTION:
      {
        BifrostObjFn* const fn = (BifrostObjFn*)obj;
        bfGCMarkValues(fn->constants, mark_value);
        break;
      }
      case BIFROST_VM_OBJ_NATIVE_FN:
      {
        BifrostObjNativeFn* const fn = (BifrostObjNativeFn*)obj;
        bfGCMarkValuesN(fn->statics, fn->num_statics, mark_value);
        break;
      }
      case BIFROST_VM_OBJ_STRING:
        break;
      case BIFROST_VM_OBJ_REFERENCE:
      {
        BifrostObjReference* const ref = (BifrostObjReference*)obj;

        if (ref->clz)
        {
          bfGCMarkObj(&ref->clz->super, mark_value);
        }

        break;
      }
      case BIFROST_VM_OBJ_WEAK_REF:
      {
        BifrostObjWeakRef* const weak_ref = (BifrostObjWeakRef*)obj;

        if (weak_ref->clz)
        {
          bfGCMarkObj(&weak_ref->clz->super, mark_value);
        }

        break;
      }
        bfInvalidDefaultCase();
    }
  }
}

static void bfGCMarkSymbols(BifrostVMSymbol* symbols, int mark_value)
{
  const size_t size = Array_size(&symbols);

  for (size_t i = 0; i < size; ++i)
  {
    bfGCMarkValue(symbols[i].value, mark_value);
  }
}

size_t bfGCObjectSize(BifrostObj* obj)
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
      return sizeof(BifrostObjNativeFn) + ((BifrostObjNativeFn*)obj)->num_statics * sizeof(bfVMValue) + ((BifrostObjNativeFn*)obj)->extra_data_size;
    }
    case BIFROST_VM_OBJ_STRING:
    {
      return sizeof(BifrostObjStr);
    }
    case BIFROST_VM_OBJ_REFERENCE:
    {
      return sizeof(BifrostObjReference) + ((BifrostObjReference*)obj)->extra_data_size;
    }
    case BIFROST_VM_OBJ_WEAK_REF:
    {
      return sizeof(BifrostObjWeakRef);
    }
      bfInvalidDefaultCase();
  }

  return 0u;
}

static void bfGCFinalize(BifrostVM* self)
{
  const uint32_t      symbol = bfVM_getSymbol(self, bfMakeStringRangeC("dtor"));
  BifrostObjInstance* cursor = (BifrostObjInstance*)self->finalized;

  while (cursor)
  {
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

    cursor = (BifrostObjInstance*)cursor->super.next;
  }
}
