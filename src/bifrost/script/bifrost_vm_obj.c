#include "bifrost_vm_obj.h"

#include <string.h>  // memset

static inline void objSetup(BifrostObj* obj, BifrostVMObjType type, BifrostObj* next)
{
  obj->type    = type;
  obj->next    = next;
  obj->gc_mark = 0;
}

static void* allocObj(struct BifrostVM_t* self, size_t size, BifrostVMObjType type)
{
  if (self->bytes_allocated >= self->params.heap_size)
  {
    bfVM_gc(self);
  }

  BifrostObj* obj = bfGCAllocMemory(self, NULL, 0u, size, sizeof(void*));
  objSetup(obj, type, self->gc_object_list);
  self->gc_object_list = obj;

  self->bytes_allocated += size;

  return obj;
}

BifrostObjModule* bfVM_createModule(struct BifrostVM_t* self, bfStringRange name)
{
  BifrostObjModule* module = allocObj(self, sizeof(BifrostObjModule), BIFROST_VM_OBJ_MODULE);

  module->name      = String_newLen(name.bgn, bfStringRange_length(&name));
  module->variables = OLD_bfArray_newA(module->variables, 32);
  memset(&module->init_fn, 0x0, sizeof(module->init_fn));
  objSetup(&module->init_fn.super, BIFROST_VM_OBJ_FUNCTION, NULL);
  module->init_fn.module = module;
  return module;
}

BifrostObjClass* bfVM_createClass(struct BifrostVM_t* self, BifrostObjModule* module, bfStringRange name, BifrostObjClass* base_clz, size_t extra_data)
{
  BifrostObjClass* clz = allocObj(self, sizeof(BifrostObjClass), BIFROST_VM_OBJ_CLASS);

  clz->name               = String_newLen(name.bgn, bfStringRange_length(&name));
  clz->base_clz           = base_clz;
  clz->module             = module;
  clz->symbols            = OLD_bfArray_newA(clz->symbols, 32);
  clz->field_initializers = OLD_bfArray_newA(clz->field_initializers, 32);
  clz->extra_data         = extra_data;
  clz->finalizer          = NULL;

  return clz;
}

BifrostObjInstance* bfVM_createInstance(struct BifrostVM_t* self, BifrostObjClass* clz)
{
  BifrostObjInstance* inst = allocObj(self, sizeof(BifrostObjInstance) + clz->extra_data, BIFROST_VM_OBJ_INSTANCE);

  BifrostHashMapParams hash_params;
  bfHashMapParams_init(&hash_params);
  hash_params.value_size = sizeof(bfVMValue);
  // hash_params.hash = ModuleMap_hash;
  // hash_params.cmp  = ModuleMap_cmp;

  bfHashMap_ctor(&inst->fields, &hash_params);
  inst->clz = clz;

  const size_t num_fields = Array_size(&clz->field_initializers);

  for (size_t i = 0; i < num_fields; ++i)
  {
    BifrostVMSymbol* const sym = clz->field_initializers + i;

    bfHashMap_set(&inst->fields, sym->name, &sym->value);
  }

  return inst;
}

BifrostObjFn* bfVM_createFunction(struct BifrostVM_t* self, BifrostObjModule* module)
{
  BifrostObjFn* fn = allocObj(self, sizeof(BifrostObjFn), BIFROST_VM_OBJ_FUNCTION);

  fn->module = module;

  /* NOTE(Shareef): 'fn' Will be filled out later by a Function Builder. */

  return fn;
}

BifrostObjNativeFn* bfVM_createNativeFn(struct BifrostVM_t* self, bfNativeFnT fn_ptr, int32_t arity, uint32_t num_statics)
{
  BifrostObjNativeFn* fn = allocObj(self, sizeof(BifrostObjNativeFn) + sizeof(bfVMValue) * num_statics, BIFROST_VM_OBJ_NATIVE_FN);
  fn->value              = fn_ptr;
  fn->arity              = arity;
  fn->num_statics        = num_statics;
  fn->statics            = (bfVMValue*)((char*)fn + sizeof(BifrostObjNativeFn));

  return fn;
}

BifrostObjStr* bfVM_createString(struct BifrostVM_t* self, bfStringRange value)
{
  BifrostObjStr* obj = allocObj(self, sizeof(BifrostObjStr), BIFROST_VM_OBJ_STRING);
  obj->value         = String_newLen(value.bgn, bfStringRange_length(&value));
  String_unescape(obj->value);
  obj->hash = bfString_hashN(obj->value, String_length(obj->value));

  return obj;
}

BifrostObjReference* bfVM_createReference(struct BifrostVM_t* self, size_t extra_data_size)
{
  BifrostObjReference* obj = allocObj(self, sizeof(BifrostObjReference) + extra_data_size, BIFROST_VM_OBJ_REFERENCE);
  obj->clz                 = NULL;
  obj->extra_data_size     = extra_data_size;
  memset(&obj->extra_data, 0x0, extra_data_size);

  return obj;
}

BifrostObjWeakRef* bfVM_createWeakRef(struct BifrostVM_t* self, void* data)
{
  BifrostObjWeakRef* obj = allocObj(self, sizeof(BifrostObjWeakRef), BIFROST_VM_OBJ_WEAK_REF);
  obj->data              = data;

  return obj;
}

void bfVMObject__delete(struct BifrostVM_t* self, BifrostObj* obj)
{
  switch (obj->type & BifrostVMObjType_mask)
  {
    case BIFROST_VM_OBJ_MODULE:
    {
      BifrostObjModule* const module = (BifrostObjModule*)obj;
      String_delete(module->name);
      Array_delete(&module->variables);
      if (module->init_fn.name)
      {
        bfVMObject__delete(self, &module->init_fn.super);
      }
      break;
    }
    case BIFROST_VM_OBJ_CLASS:
    {
      BifrostObjClass* const clz = (BifrostObjClass*)obj;

      String_delete(clz->name);
      Array_delete(&clz->symbols);
      Array_delete(&clz->field_initializers);
      break;
    }
    case BIFROST_VM_OBJ_INSTANCE:
    {
      BifrostObjInstance* const inst = (BifrostObjInstance*)obj;

      bfHashMap_dtor(&inst->fields);
      break;
    }
    case BIFROST_VM_OBJ_FUNCTION:
    {
      BifrostObjFn* const fn = (BifrostObjFn*)obj;

      String_delete(fn->name);
      Array_delete(&fn->constants);
      Array_delete(&fn->instructions);
      Array_delete(&fn->line_to_code);
      break;
    }
    case BIFROST_VM_OBJ_NATIVE_FN:
    {
      break;
    }
    case BIFROST_VM_OBJ_STRING:
    {
      BifrostObjStr* const str = (BifrostObjStr*)obj;
      String_delete(str->value);
      break;
    }
    case BIFROST_VM_OBJ_REFERENCE:
    {
      break;
    }
    case BIFROST_VM_OBJ_WEAK_REF:
    {
      break;
    }
    default:
    {
      break;
    }
  }
}

void bfVMObject_delete(struct BifrostVM_t* self, BifrostObj* obj)
{
  const size_t obj_size = bfGCObjectSize(obj);

  bfVMObject__delete(self, obj);
  bfGCAllocMemory(self, obj, obj_size, 0u, sizeof(void*));
}

bfBool32 bfObjIsFunction(const BifrostObj* obj)
{
  return obj->type == BIFROST_VM_OBJ_FUNCTION || obj->type == BIFROST_VM_OBJ_NATIVE_FN;
}

void bfObjFinalize(struct BifrostVM_t* self, BifrostObj* obj)
{
  // TODO(SR): Find a way to guarantee instances don't get finalized twice

  if (obj->type == BIFROST_VM_OBJ_INSTANCE)
  {
    BifrostObjInstance* inst = (BifrostObjInstance*)obj;

    if (inst->clz->finalizer)
    {
      inst->clz->finalizer(self, &inst->extra_data);
    }
  }
  else if (obj->type == BIFROST_VM_OBJ_REFERENCE)
  {
    BifrostObjReference* ref = (BifrostObjReference*)obj;

    if (ref->clz->finalizer)
    {
      ref->clz->finalizer(self, &ref->extra_data);
    }
  }
}
