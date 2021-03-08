/******************************************************************************/
/*!
 * @file   bifrost_vm_api.c
 * @author Shareef Raheem (http://blufedora.github.io)
 * @par
 *    Bifrost Scripting Language
 *    Dependencies:
 *      > C99 or later.
 *      > Bifrost Data Structures Library
 *      > C Runtime Library
 *
 * @brief
 *  The main API for the Bifrost Scripting Language.
 *
 * @version 0.0.1-beta
 * @date    2019-07-01
 *
 * @copyright Copyright (c) 2019 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#include "bifrost/bifrost_vm.h"

#include "bifrost_vm_debug.h"
#include "bifrost_vm_gc.h"

#include <string.h>  // memset, memmove

struct bfValueHandle_t
{
  bfVMValue     value;
  bfValueHandle prev;
  bfValueHandle next;
};

bfVMValue bfVM_getHandleValue(bfValueHandle h)
{
  return h->value;
}

bfValueHandle bfVM_getHandleNext(bfValueHandle h)
{
  return h->next;
}

BifrostObjModule*     bfVM_findModule(BifrostVM* self, const char* name, size_t name_len);
uint32_t              bfVM_getSymbol(BifrostVM* self, bfStringRange name);
static BifrostVMError bfVM_runModule(BifrostVM* self, BifrostObjModule* module);
static BifrostVMError bfVM_compileIntoModule(BifrostVM* self, BifrostObjModule* module, const char* source, size_t source_len);

BifrostMethodBind bfMethodBind_make(const char* name, bfNativeFnT func, int32_t arity, uint32_t num_statics, uint16_t extra_data)
{
  BifrostMethodBind self;

  self.name        = name;
  self.fn          = func;
  self.arity       = arity;
  self.num_statics = num_statics;
  self.extra_data  = extra_data;

  return self;
}

BifrostMethodBind bfMethodBind_end(void)
{
  return bfMethodBind_make(NULL, NULL, 0, 0, 0);
}

void bfVMParams_init(BifrostVMParams* self)
{
  self->error_fn           = NULL;                 /* errors will have to be check with return values and 'bfVM_errorString' */
  self->print_fn           = NULL;                 /* 'print' will be a no op.                                               */
  self->module_fn          = NULL;                 /* unable to load user modules                                            */
  self->memory_fn          = bfGCDefaultAllocator; /* uses c library's realloc and free by default                           */
  self->min_heap_size      = 1000000;              /* 1mb                                                                    */
  self->heap_size          = 5242880;              /* 5mb                                                                    */
  self->heap_growth_factor = 0.5f;                 /* Grow by x1.5                                                           */
  self->user_data          = NULL;                 /* User data for the memory allocator, and maybe other future things.     */
}

static inline void bfVM_assertStackIndex(const BifrostVM* self, size_t idx)
{
  const size_t size = bfVMArray_size(&self->stack);

  if (!(idx < size))
  {
    assert(idx < size && "Invalid index passed into bfVM_stack* function.");
  }
}

BifrostVM* bfVM_new(const BifrostVMParams* params)
{
  BifrostVM* self = params->memory_fn(params->user_data, NULL, 0u, sizeof(BifrostVM));

  if (self)
  {
    bfVM_ctor(self, params);
  }

  return self;
}

static unsigned ModuleMap_hash(const void* key)
{
  return ((const BifrostObjStr*)key)->hash;
}

static int ModuleMap_cmp(const void* lhs, const void* rhs)
{
  const BifrostObjStr* str_lhs = (const BifrostObjStr*)lhs;
  const BifrostObjStr* str_rhs = (const BifrostObjStr*)rhs;

  return str_lhs->hash == str_rhs->hash && bfVMString_cmp(str_lhs->value, str_rhs->value) == 0;
}

void bfVM_ctor(BifrostVM* self, const BifrostVMParams* params)
{
  memset(self, 0x0, sizeof(*self));

  self->gc_is_running     = bfTrue;   // Make it so initialization doesn't cause a GC.
  self->params            = *params;  // Must happen first to copy over the allocator.
  self->frames            = bfVMArray_newA(self, self->frames, 12);
  self->stack             = bfVMArray_newA(self, self->stack, 10);
  self->stack_top         = self->stack;
  self->symbols           = bfVMArray_newA(self, self->symbols, 10);
  self->gc_object_list    = NULL;
  self->last_error        = bfVMString_new(self, "");
  self->bytes_allocated   = 0u;
  self->handles           = NULL;
  self->free_handles      = NULL;
  self->parser_stack      = NULL;
  self->temp_roots_top    = 0;
  self->finalized         = NULL;
  self->current_native_fn = NULL;

  /*
    NOTE(Shareef):
      Custom dtor are not needed as the strings being
      stored in the map will be garbage collected.
  */
  BifrostHashMapParams hash_params;
  bfHashMapParams_init(&hash_params, self);
  hash_params.hash       = ModuleMap_hash;
  hash_params.cmp        = ModuleMap_cmp;
  hash_params.value_size = sizeof(BifrostObjModule*);
  bfHashMap_ctor(&self->modules, &hash_params);

  self->build_in_symbols[BIFROST_VM_SYMBOL_CTOR] = bfVM_getSymbol(self, bfMakeStringRangeC("ctor"));
  self->build_in_symbols[BIFROST_VM_SYMBOL_DTOR] = bfVM_getSymbol(self, bfMakeStringRangeC("dtor"));
  self->build_in_symbols[BIFROST_VM_SYMBOL_CALL] = bfVM_getSymbol(self, bfMakeStringRangeC("call"));

  self->gc_is_running = bfFalse;
}

void* bfVM_userData(const BifrostVM* self)
{
  return self->params.user_data;
}

static BifrostVMError bfVM__moduleMake(BifrostVM* self, const char* module, BifrostObjModule** out)
{
  static const char* ANON_MODULE_NAME = "__anon_module__";

  const bfBool32 is_anon = module == NULL;

  if (is_anon)
  {
    module = ANON_MODULE_NAME;
  }

  const bfStringRange name_range = bfMakeStringRangeC(module);

  if (!is_anon)
  {
    // TODO(SR):
    //   Make it so this check only happens in debug builds??

    *out = bfVM_findModule(self, module, bfStringRange_length(&name_range));

    if (*out)
    {
      return BIFROST_VM_ERROR_MODULE_ALREADY_DEFINED;
    }
  }

  *out = bfVM_createModule(self, name_range);

  if (!is_anon)
  {
    bfGCPushRoot(self, &(*out)->super);
    BifrostObjStr* const module_name = bfVM_createString(self, name_range);
    bfHashMap_set(&self->modules, module_name, out);
    bfGCPopRoot(self);
  }

  return BIFROST_VM_ERROR_NONE;
}

BifrostVMError bfVM_moduleMake(BifrostVM* self, size_t idx, const char* module)
{
  bfVM_assertStackIndex(self, idx);

  BifrostObjModule*    temp;
  const BifrostVMError err = bfVM__moduleMake(self, module, &temp);

  self->stack_top[idx] = bfVMValue_fromPointer(temp);

  return err;
}

static void bfVM_moduleLoadStdIOPrint(BifrostVM* vm, int32_t num_args)
{
  const bfPrintFn print = vm->params.print_fn;

  if (print && num_args)
  {
    // TODO: Make this safer and better
    char buffer[1024];

    char* buffer_head = buffer;
    char* buffer_end  = buffer + sizeof(buffer);

    for (int32_t i = 0; i < num_args && buffer < buffer_end; ++i)
    {
      const bfVMValue value         = vm->stack_top[i];
      const size_t    written_bytes = bfDbgValueToString(value, buffer_head, buffer_end - buffer_head);

      buffer_head += written_bytes;
    }

    print(vm, buffer);
  }
}

void bfVM_moduleLoadStd(BifrostVM* self, size_t idx, uint32_t module_flags)
{
  if (module_flags & BIFROST_VM_STD_MODULE_IO)
  {
    if (bfVM_moduleMake(self, idx, "std:io") == BIFROST_VM_ERROR_NONE)
    {
      bfVM_stackStoreNativeFn(self, idx, "print", &bfVM_moduleLoadStdIOPrint, -1);
    }
  }
}

BifrostVMError bfVM_moduleLoad(BifrostVM* self, size_t idx, const char* module)
{
  bfVM_assertStackIndex(self, idx);

  BifrostObjModule* const module_obj = bfVM_findModule(self, module, strlen(module));

  if (module_obj)
  {
    self->stack_top[idx] = bfVMValue_fromPointer(module_obj);
    return BIFROST_VM_ERROR_NONE;
  }

  return BIFROST_VM_ERROR_MODULE_NOT_FOUND;
}

typedef struct
{
  const char* str;
  size_t      str_len;
  unsigned    hash;

} TempModuleName;

static int bfVM_moduleUnloadCmp(const void* lhs, const void* rhs)
{
  const TempModuleName* str_lhs = (const TempModuleName*)lhs;
  const BifrostObjStr*  str_rhs = (const BifrostObjStr*)rhs;

  return str_lhs->hash == str_rhs->hash && bfVMString_ccmpn(str_rhs->value, str_lhs->str, str_lhs->str_len) == 0;
}

void bfVM_moduleUnload(BifrostVM* self, const char* module)
{
  /*
    NOTE(Shareef):
      The GC will handle deleting the module and
      string whenever we are low on memory.
   */
  const TempModuleName tmn =
   {
    .str     = module,
    .str_len = strlen(module),
    .hash    = bfVMString_hash(module)};

  bfHashMap_removeCmp(&self->modules, &tmn, &bfVM_moduleUnloadCmp);
}

void bfVM_moduleUnloadAll(BifrostVM* self)
{
  bfHashMap_clear(&self->modules);
}

size_t bfVM_stackSize(const BifrostVM* self)
{
  return bfVMArray_size(&self->stack) - (self->stack_top - self->stack);
}

BifrostVMError bfVM_stackResize(BifrostVM* self, size_t size)
{
  const size_t stack_size     = bfVMArray_size(&self->stack);
  const size_t stack_used     = self->stack_top - self->stack;
  const size_t requested_size = stack_used + size;

  if (stack_size < requested_size)
  {
    /*
      TODO(SR):
        Make "bfVMArray_resize" return whether or not the realloc was successful.
    */
    bfVMArray_resize(self, &self->stack, requested_size);
    self->stack_top = self->stack + stack_used;
  }

  return BIFROST_VM_ERROR_NONE;
}

bfVMValue bfVM_stackFindVariable(BifrostObjModule* module_obj, const char* variable, size_t variable_len)
{
  assert(module_obj && "bfVM_stackFindVariable: Module must not be null.");

  const size_t num_vars = bfVMArray_size(&module_obj->variables);

  for (size_t i = 0; i < num_vars; ++i)
  {
    BifrostVMSymbol* const var = module_obj->variables + i;

    if (bfVMString_length(var->name) == variable_len && strncmp(variable, var->name, variable_len) == 0)
    {
      return var->value;
    }
  }

  return bfVMValue_fromNull();
}

static int bfVM__stackStoreVariable(BifrostVM* self, bfVMValue obj, bfStringRange field_symbol, bfVMValue value);

BifrostVMError bfVM_stackMakeInstance(BifrostVM* self, size_t clz_idx, size_t dst_idx)
{
  bfVM_assertStackIndex(self, clz_idx);
  bfVM_assertStackIndex(self, dst_idx);

  bfVMValue clz_value = self->stack_top[clz_idx];

  /* TODO(SR): Only in Debug Builds */
  if (!bfVMValue_isPointer(clz_value))
  {
    return BIFROST_VM_ERROR_INVALID_OP_ON_TYPE;
  }

  BifrostObj* obj = bfVMValue_asPointer(clz_value);

  /* TODO(SR): Only in Debug Builds */
  if (!(obj->type == BIFROST_VM_OBJ_CLASS))
  {
    return BIFROST_VM_ERROR_INVALID_OP_ON_TYPE;
  }

  BifrostObjInstance* new_instance = bfVM_createInstance(self, (BifrostObjClass*)obj);

  if (!new_instance)
  {
    return BIFROST_VM_ERROR_OUT_OF_MEMORY;
  }

  self->stack_top[dst_idx] = bfVMValue_fromPointer(new_instance);

  return BIFROST_VM_ERROR_NONE;
}

void* bfVM_stackMakeReference(BifrostVM* self, size_t idx, size_t extra_data_size)
{
  bfVM_assertStackIndex(self, idx);
  self->stack_top[idx] = bfVMValue_fromPointer(bfVM_createReference(self, extra_data_size));

  return bfVM_stackReadInstance(self, idx);
}

static BifrostObjModule* bfVM__findModule(bfVMValue obj)
{
  if (!bfVMValue_isPointer(obj))
  {
    return NULL;
  }

  BifrostObj* obj_ptr = bfVMValue_asPointer(obj);

  if (obj_ptr->type == BIFROST_VM_OBJ_INSTANCE)
  {
    BifrostObjInstance* inst = (BifrostObjInstance*)obj_ptr;
    return inst->clz->module;
  }
  else if (obj_ptr->type == BIFROST_VM_OBJ_CLASS)
  {
    BifrostObjClass* clz = (BifrostObjClass*)obj_ptr;
    return clz->module;
  }
  else if (obj_ptr->type == BIFROST_VM_OBJ_MODULE)
  {
    return (BifrostObjModule*)obj_ptr;
  }

  return NULL;
}

BifrostObjClass* createClassBinding(BifrostVM* self, bfVMValue obj, const BifrostVMClassBind* clz_bind)
{
  BifrostObjModule* module_obj = bfVM__findModule(obj);

  if (module_obj == NULL)
  {
    return NULL;
  }

  const bfStringRange      name   = bfMakeStringRangeC(clz_bind->name);
  BifrostObjClass* const   clz    = bfVM_createClass(self, module_obj, name, NULL, clz_bind->extra_data_size);
  const BifrostMethodBind* method = clz_bind->methods;

  clz->finalizer = clz_bind->finalizer;

  bfGCPushRoot(self, &clz->super);
  if (bfVM__stackStoreVariable(self, obj, name, bfVMValue_fromPointer(clz)))
  {
    bfGCPopRoot(self);

    return NULL;
  }

  while (method->name && method->fn)
  {
    BifrostObjNativeFn* const fn = bfVM_createNativeFn(self, method->fn, method->arity, method->num_statics, method->extra_data);

    bfGCPushRoot(self, &fn->super);
    bfVM_xSetVariable(&clz->symbols, self, bfMakeStringRangeC(method->name), bfVMValue_fromPointer(fn));
    bfGCPopRoot(self);

    ++method;
  }

  bfGCPopRoot(self);

  return clz;
}

void* bfVM_stackMakeReferenceClz(BifrostVM* self, size_t module_idx, const BifrostVMClassBind* clz_bind, size_t dst_idx)
{
  bfVM_assertStackIndex(self, module_idx);
  bfVM_assertStackIndex(self, dst_idx);

  BifrostObjReference* ref = bfVM_createReference(self, clz_bind->extra_data_size);
  self->stack_top[dst_idx] = bfVMValue_fromPointer(ref);
  ref->clz                 = createClassBinding(self, self->stack_top[module_idx], clz_bind);

  return ref->extra_data;
}

void bfVM_stackMakeWeakRef(BifrostVM* self, size_t idx, void* value)
{
  bfVM_assertStackIndex(self, idx);

  self->stack_top[idx] = bfVMValue_fromPointer(bfVM_createWeakRef(self, value));
}

bfBool32 bfVMGrabObjectsOfType(bfVMValue obj_a, bfVMValue obj_b, BifrostVMObjType type_a, BifrostVMObjType type_b, BifrostObj** out_a, BifrostObj** out_b)
{
  if (bfVMValue_isPointer(obj_a) && bfVMValue_isPointer(obj_b))
  {
    BifrostObj* const obj_a_ptr = BIFROST_AS_OBJ(obj_a);
    BifrostObj* const obj_b_ptr = BIFROST_AS_OBJ(obj_b);

    if (obj_a_ptr->type == type_a && obj_b_ptr->type == type_b)
    {
      *out_a = obj_a_ptr;
      *out_b = obj_b_ptr;
    }
  }

  return bfFalse;
}

void bfVM_referenceSetClass(BifrostVM* self, size_t idx, size_t clz_idx)
{
  bfVM_assertStackIndex(self, idx);
  bfVM_assertStackIndex(self, clz_idx);

  const bfVMValue obj     = self->stack_top[idx];
  const bfVMValue clz     = self->stack_top[clz_idx];
  BifrostObj*     obj_ptr = NULL;
  BifrostObj*     clz_ptr = NULL;

  if (bfVMGrabObjectsOfType(obj, clz, BIFROST_VM_OBJ_REFERENCE, BIFROST_VM_OBJ_CLASS, &obj_ptr, &clz_ptr))
  {
    ((BifrostObjReference*)obj_ptr)->clz = (BifrostObjClass*)clz_ptr;
  }
}

void bfVM_classSetBaseClass(BifrostVM* self, size_t idx, size_t clz_idx)
{
  bfVM_assertStackIndex(self, idx);
  bfVM_assertStackIndex(self, clz_idx);

  const bfVMValue obj     = self->stack_top[idx];
  const bfVMValue clz     = self->stack_top[clz_idx];
  BifrostObj*     obj_ptr = NULL;
  BifrostObj*     clz_ptr = NULL;

  if (bfVMGrabObjectsOfType(obj, clz, BIFROST_VM_OBJ_CLASS, BIFROST_VM_OBJ_CLASS, &obj_ptr, &clz_ptr))
  {
    ((BifrostObjClass*)obj_ptr)->base_clz = (BifrostObjClass*)clz_ptr;
  }
}

void bfVM_stackLoadVariable(BifrostVM* self, size_t dst_idx, size_t inst_or_class_or_module, const char* variable)
{
  bfVM_assertStackIndex(self, dst_idx);
  bfVM_assertStackIndex(self, inst_or_class_or_module);

  BifrostObj*         obj      = bfVMValue_asPointer(self->stack_top[inst_or_class_or_module]);
  const bfStringRange var_name = bfMakeStringRangeC(variable);
  const size_t        symbol   = bfVM_getSymbol(self, var_name);

  if (obj->type == BIFROST_VM_OBJ_INSTANCE)
  {
    BifrostObjInstance* const inst = (BifrostObjInstance*)obj;

    bfVMValue* value = bfHashMap_get(&inst->fields, self->symbols[symbol]);

    if (value)
    {
      self->stack_top[dst_idx] = *value;
      return;
    }

    /*
      NOTE(Shareef):
        Fall back to class if not on instance.
     */
    obj = &inst->clz->super;
  }

  if (obj->type == BIFROST_VM_OBJ_CLASS)
  {
    BifrostObjClass* const clz = (BifrostObjClass*)obj;

    /* TODO: Look through base classes? */

    if (symbol < bfVMArray_size(&clz->symbols))
    {
      self->stack_top[dst_idx] = clz->symbols[symbol].value;
    }
    else
    {
      self->stack_top[dst_idx] = bfVMValue_fromNull();
    }
  }
  else if (obj->type == BIFROST_VM_OBJ_MODULE)
  {
    BifrostObjModule* const module = (BifrostObjModule*)obj;
    self->stack_top[dst_idx]       = bfVM_stackFindVariable(module, var_name.str_bgn, bfStringRange_length(&var_name));
  }
  else
  {
    self->stack_top[dst_idx] = bfVMValue_fromNull();
  }
}

static int bfVM__stackStoreVariable(BifrostVM* self, bfVMValue obj, bfStringRange field_symbol, bfVMValue value)
{
  if (!bfVMValue_isPointer(obj))
  {
    return 1;
  }

  BifrostObj*         obj_ptr = bfVMValue_asPointer(obj);
  const size_t        symbol  = bfVM_getSymbol(self, field_symbol);
  const BifrostString sym_str = self->symbols[symbol];

  if (obj_ptr->type == BIFROST_VM_OBJ_INSTANCE)
  {
    BifrostObjInstance* inst = (BifrostObjInstance*)obj_ptr;

    bfHashMap_set(&inst->fields, sym_str, &value);
  }
  else if (obj_ptr->type == BIFROST_VM_OBJ_CLASS)
  {
    BifrostObjClass* clz = (BifrostObjClass*)obj_ptr;

    bfVM_xSetVariable(&clz->symbols, self, field_symbol, value);
  }
  else if (obj_ptr->type == BIFROST_VM_OBJ_MODULE)
  {
    BifrostObjModule* const module_obj = (BifrostObjModule*)obj_ptr;

    bfVM_xSetVariable(&module_obj->variables, self, field_symbol, value);
  }
  else
  {
    return 2;
  }

  return 0;
}

BifrostVMError bfVM_stackStoreVariable(BifrostVM* self, size_t inst_or_class_or_module, const char* field, size_t value_idx)
{
  bfVM_assertStackIndex(self, value_idx);
  bfVM_assertStackIndex(self, inst_or_class_or_module);

  const bfVMValue     obj      = self->stack_top[inst_or_class_or_module];
  const bfStringRange var_name = bfMakeStringRangeC(field);
  const bfVMValue     value    = self->stack_top[value_idx];

  if (bfVM__stackStoreVariable(self, obj, var_name, value))
  {
    return BIFROST_VM_ERROR_INVALID_OP_ON_TYPE;
  }

  return BIFROST_VM_ERROR_NONE;
}

BifrostVMError bfVM_stackStoreNativeFn(BifrostVM* self, size_t inst_or_class_or_module, const char* field, bfNativeFnT func, int32_t arity)
{
  return bfVM_stackStoreClosure(self, inst_or_class_or_module, field, func, arity, 0, 0);
}

BifrostVMError bfVM_closureGetStatic(BifrostVM* self, size_t dst_idx, size_t static_idx)
{
  bfVM_assertStackIndex(self, dst_idx);
  bfVM_assertStackIndex(self, static_idx);

  const BifrostObjNativeFn* native_fn = self->current_native_fn;

  if (!native_fn || static_idx >= native_fn->num_statics)
  {
    return BIFROST_VM_ERROR_INVALID_ARGUMENT;
  }

  self->stack_top[dst_idx] = native_fn->statics[static_idx];

  return BIFROST_VM_ERROR_NONE;
}

BifrostVMError bfVM_stackStoreClosure(BifrostVM* self, size_t inst_or_class_or_module, const char* field, bfNativeFnT func, int32_t arity, uint32_t num_statics, uint16_t extra_data)
{
  bfVM_assertStackIndex(self, inst_or_class_or_module);

  const bfVMValue     obj      = self->stack_top[inst_or_class_or_module];
  const bfStringRange var_name = bfMakeStringRangeC(field);

  if (bfVM__stackStoreVariable(self, obj, var_name, bfVMValue_fromPointer(bfVM_createNativeFn(self, func, arity, num_statics, extra_data))))
  {
    return BIFROST_VM_ERROR_INVALID_OP_ON_TYPE;
  }

  return BIFROST_VM_ERROR_NONE;
}

BifrostVMError bfVM_closureSetStatic(BifrostVM* self, size_t closure_idx, size_t static_idx, size_t value_idx)
{
  bfVM_assertStackIndex(self, closure_idx);
  bfVM_assertStackIndex(self, value_idx);

  const bfVMValue obj = self->stack_top[closure_idx];

  if (!bfVMValue_isPointer(obj))
  {
    return BIFROST_VM_ERROR_INVALID_OP_ON_TYPE;
  }

  BifrostObj* obj_ptr = BIFROST_AS_OBJ(obj);

  if (obj_ptr->type != BIFROST_VM_OBJ_NATIVE_FN)
  {
    return BIFROST_VM_ERROR_INVALID_OP_ON_TYPE;
  }

  BifrostObjNativeFn* native_fn = (BifrostObjNativeFn*)obj_ptr;

  if (static_idx >= native_fn->num_statics)
  {
    return BIFROST_VM_ERROR_INVALID_ARGUMENT;
  }

  native_fn->statics[static_idx] = self->stack_top[value_idx];

  return BIFROST_VM_ERROR_NONE;
}

void* bfVM_closureStackGetExtraData(BifrostVM* self, size_t closure_idx)
{
  bfVM_assertStackIndex(self, closure_idx);

  const bfVMValue obj = self->stack_top[closure_idx];

  if (!bfVMValue_isPointer(obj))
  {
    return NULL;
  }

  BifrostObj* obj_ptr = BIFROST_AS_OBJ(obj);

  if (obj_ptr->type != BIFROST_VM_OBJ_NATIVE_FN)
  {
    return NULL;
  }

  BifrostObjNativeFn* native_fn = (BifrostObjNativeFn*)obj_ptr;

  return native_fn->extra_data;
}

void* bfVM_closureGetExtraData(BifrostVM* self)
{
  BifrostObjNativeFn* const native_fn = self->current_native_fn;

  return native_fn ? native_fn->extra_data : NULL;
}

BifrostVMError bfVM_stackStoreClass(BifrostVM* self, size_t inst_or_class_or_module, const BifrostVMClassBind* clz_bind)
{
  bfVM_assertStackIndex(self, inst_or_class_or_module);

  if (createClassBinding(self, self->stack_top[inst_or_class_or_module], clz_bind) == NULL)
  {
    return BIFROST_VM_ERROR_INVALID_OP_ON_TYPE;
  }

  return BIFROST_VM_ERROR_NONE;
}

void bfVM_stackSetString(BifrostVM* self, size_t idx, const char* value)
{
  bfVM_stackSetStringLen(self, idx, value, strlen(value));
}

void bfVM_stackSetStringLen(BifrostVM* self, size_t idx, const char* value, size_t len)
{
  bfVM_assertStackIndex(self, idx);
  self->stack_top[idx] = bfVMValue_fromPointer(bfVM_createString(self, (bfStringRange){.str_bgn = value, .str_end = value + len}));
}

void bfVM_stackSetNumber(BifrostVM* self, size_t idx, bfFloat64 value)
{
  bfVM_assertStackIndex(self, idx);
  self->stack_top[idx] = bfVMValue_fromNumber(value);
}

void bfVM_stackSetBool(BifrostVM* self, size_t idx, bfBool32 value)
{
  bfVM_assertStackIndex(self, idx);
  self->stack_top[idx] = bfVMValue_fromBool(value);
}

void bfVM_stackSetNil(BifrostVM* self, size_t idx)
{
  bfVM_assertStackIndex(self, idx);
  self->stack_top[idx] = bfVMValue_fromNull();
}

void* bfVM_stackReadInstance(const BifrostVM* self, size_t idx)
{
  bfVM_assertStackIndex(self, idx);
  const bfVMValue value = self->stack_top[idx];

  if (bfVMValue_isNull(value))
  {
    return NULL;
  }

  assert(bfVMValue_isPointer(value) && "The value being read is not an object.");

  BifrostObj* obj = bfVMValue_asPointer(value);

  if (obj->type == BIFROST_VM_OBJ_INSTANCE)
  {
    BifrostObjInstance* inst = (BifrostObjInstance*)obj;
    return inst->extra_data;
  }

  if (obj->type == BIFROST_VM_OBJ_REFERENCE)
  {
    BifrostObjReference* inst = (BifrostObjReference*)obj;
    return inst->extra_data;
  }

  if (obj->type == BIFROST_VM_OBJ_WEAK_REF)
  {
    BifrostObjWeakRef* inst = (BifrostObjWeakRef*)obj;
    return inst->data;
  }

  assert(!"This object is not a instance.");
  return NULL;
}

const char* bfVM_stackReadString(const BifrostVM* self, size_t idx, size_t* out_size)
{
  bfVM_assertStackIndex(self, idx);
  bfVMValue value = self->stack_top[idx];

  assert(bfVMValue_isPointer(value) && "The value being read is not an object.");

  BifrostObj* obj = bfVMValue_asPointer(value);

  assert(obj->type == BIFROST_VM_OBJ_STRING && "This object is not a string.");

  BifrostObjStr* str = (BifrostObjStr*)obj;

  if (out_size)
  {
    *out_size = bfVMString_length(str->value);
  }

  return str->value;
}

bfFloat64 bfVM_stackReadNumber(const BifrostVM* self, size_t idx)
{
  bfVM_assertStackIndex(self, idx);
  const bfVMValue value = self->stack_top[idx];

  assert(bfVMValue_isNumber(value) && "The value is not a number.");

  return bfVMValue_asNumber(value);
}

bfBool32 bfVM_stackReadBool(const BifrostVM* self, size_t idx)
{
  bfVM_assertStackIndex(self, idx);
  const bfVMValue value = self->stack_top[idx];

  assert(bfVMValue_isBool(value) && "The value is not a boolean.");

  return bfVMValue_isThuthy(value);
}

static int32_t bfVMGetArity(bfVMValue value)
{
  assert(bfVMValue_isPointer(value));

  const BifrostObj* const obj = bfVMValue_asPointer(value);

  if (obj->type == BIFROST_VM_OBJ_FUNCTION)
  {
    const BifrostObjFn* const fn = (const BifrostObjFn*)obj;
    return fn->arity;
  }

  if (obj->type == BIFROST_VM_OBJ_NATIVE_FN)
  {
    const BifrostObjNativeFn* const fn = (const BifrostObjNativeFn*)obj;
    return fn->arity;
  }
  // TODO: If an instance / reference has a 'call' operator that should be checked.

  assert(!"Invalid type for arity check!");
  return 0;
}

static BifrostVMType bfVMGetType(bfVMValue value)
{
  if (bfVMValue_isBool(value))
  {
    return BIFROST_VM_BOOL;
  }

  if (bfVMValue_isNumber(value))
  {
    return BIFROST_VM_NUMBER;
  }

  if (bfVMValue_isPointer(value))
  {
    const BifrostObj* const obj = bfVMValue_asPointer(value);

    if (obj->type == BIFROST_VM_OBJ_STRING)
    {
      return BIFROST_VM_STRING;
    }

    if (obj->type == BIFROST_VM_OBJ_INSTANCE || obj->type == BIFROST_VM_OBJ_REFERENCE || obj->type == BIFROST_VM_OBJ_WEAK_REF)
    {
      return BIFROST_VM_OBJECT;
    }

    if (obj->type == BIFROST_VM_OBJ_FUNCTION || obj->type == BIFROST_VM_OBJ_NATIVE_FN)
    {
      return BIFROST_VM_FUNCTION;
    }

    if (obj->type == BIFROST_VM_OBJ_MODULE)
    {
      return BIFROST_VM_MODULE;
    }
  }

  if (value == bfVMValue_fromNull())
  {
    return BIFROST_VM_NIL;
  }

  return BIFROST_VM_UNDEFINED;
}

BifrostVMType bfVM_stackGetType(BifrostVM* self, size_t idx)
{
  bfVM_assertStackIndex(self, idx);
  return bfVMGetType(self->stack_top[idx]);
}

int32_t bfVM_stackGetArity(const BifrostVM* self, size_t idx)
{
  bfVM_assertStackIndex(self, idx);
  return bfVMGetArity(self->stack_top[idx]);
}

int32_t bfVM_handleGetArity(bfValueHandle handle)
{
  return bfVMGetArity(handle->value);
}

BifrostVMType bfVM_handleGetType(bfValueHandle handle)
{
  return bfVMGetType(handle->value);
}

bfValueHandle bfVM_stackMakeHandle(BifrostVM* self, size_t idx)
{
  bfVM_assertStackIndex(self, idx);

  bfValueHandle handle;

  if (self->free_handles)
  {
    handle             = self->free_handles;
    self->free_handles = self->free_handles->next;
  }
  else
  {
    handle = bfGCAllocMemory(self, NULL, 0u, sizeof(struct bfValueHandle_t));
  }

  if (handle)
  {
    handle->value = self->stack_top[idx];
    handle->prev  = NULL;
    handle->next  = self->handles;

    if (self->handles)
    {
      self->handles->prev = handle;
    }

    self->handles = handle;
  }

  return handle;
}

void bfVM_stackLoadHandle(BifrostVM* self, size_t dst_idx, bfValueHandle handle)
{
  bfVM_assertStackIndex(self, dst_idx);
  self->stack_top[dst_idx] = handle->value;
}

void bfVM_stackDestroyHandle(BifrostVM* self, bfValueHandle handle)
{
  if (handle)
  {
    if (self->handles == handle)
    {
      self->handles = handle->next;
    }

    if (handle->next)
    {
      handle->next->prev = handle->prev;
    }

    if (handle->prev)
    {
      handle->prev->next = handle->next;
    }

    /*
    TODO(SR):
      Only do this for debug extra security builds.
   */
    handle->value = bfVMValue_fromNull();
    handle->next  = NULL;
    handle->prev  = NULL;

    handle->next       = self->free_handles;
    self->free_handles = handle;
  }
}

// TODO(SR): Optimize the main interpreter loop.
enum
{
  REG_RA,
  REG_RB,
  REG_RC,
  REG_RBx,
};

static void bfVM_decode(const bfInstruction inst, uint8_t* op_out, uint32_t* ra_out, uint32_t* rb_out, uint32_t* rc_out, uint32_t* rbx_out, int32_t* rsbx_out)
{
  *op_out   = bfVM_decodeOp(inst);
  *ra_out   = bfVM_decodeRa(inst);
  *rb_out   = bfVM_decodeRb(inst);
  *rc_out   = bfVM_decodeRc(inst);
  *rbx_out  = bfVM_decodeRBx(inst);
  *rsbx_out = bfVM_decodeRsBx(inst);
}

static bfBool32 bfVM_ensureStackspace(BifrostVM* self, size_t stack_space, const bfVMValue* top)
{
  const size_t stack_size     = bfVMArray_size(&self->stack);
  const size_t stack_used     = top - self->stack;
  const size_t requested_size = stack_used + stack_space;

  if (stack_size < requested_size)
  {
    bfVMArray_resize(self, &self->stack, requested_size);
    return bfTrue;
  }

  return bfFalse;
}

BifrostVMStackFrame* bfVM_pushCallFrame(BifrostVM* self, BifrostObjFn* fn, size_t new_start)
{
  const size_t old_top = self->stack_top - self->stack;

  if (fn)
  {
    const size_t stack_space = new_start + fn->needed_stack_space;

    if (bfVM_ensureStackspace(self, stack_space, self->stack_top))
    {
      self->stack_top = self->stack + stack_space;
    }
  }
  else
  {
    self->stack_top = self->stack + new_start;
  }

  BifrostVMStackFrame* new_frame = bfVMArray_emplace(self, &self->frames);
  new_frame->ip                  = fn ? fn->instructions : NULL;
  new_frame->fn                  = fn;
  new_frame->stack               = new_start;
  new_frame->old_stack           = old_top;

  return new_frame;
}

static void bfVM_popAllCallFrames(BifrostVM* self, const BifrostVMStackFrame* ref_frame)
{
  const size_t    num_frames   = ref_frame - self->frames;
  const size_t    total_frames = bfVMArray_size(&self->frames);
  const bfErrorFn error_fn     = self->params.error_fn;

  if (error_fn)
  {
    error_fn(self, BIFROST_VM_ERROR_STACK_TRACE_BEGIN, -1, "");

    BifrostString error_str = bfVMString_new(self, "");

    for (size_t i = num_frames; i < total_frames; ++i)
    {
      const BifrostVMStackFrame* frame    = bfVMArray_at(&self->frames, i);
      const BifrostObjFn* const  fn       = frame->fn;
      const uint16_t             line_num = fn ? fn->code_to_line[frame->ip - fn->instructions] : (uint16_t)-1;
      const char* const          fn_name  = fn ? fn->name : "<native>";

      bfVMString_sprintf(self, &error_str, "%*.s[%zu] Stack Frame Line(%u): %s\n", (int)i * 3, "", i, (unsigned)line_num, fn_name);

      error_fn(self, BIFROST_VM_ERROR_STACK_TRACE, (int)line_num, error_str);
    }

    bfVMString_delete(self, error_str);

    error_fn(self, BIFROST_VM_ERROR_STACK_TRACE, -1, self->last_error);
    error_fn(self, BIFROST_VM_ERROR_STACK_TRACE_END, -1, "");
  }

  self->stack_top = self->stack + ref_frame->old_stack;
  bfVMArray_resize(self, &self->frames, num_frames);
}

void bfVM_popCallFrame(BifrostVM* self, BifrostVMStackFrame* frame)
{
  self->stack_top = self->stack + frame->old_stack;
  bfVMArray_pop(&self->frames);
}

BifrostVMError bfVM_execTopFrame(BifrostVM* self)
{
  const BifrostVMStackFrame* const reference_frame = bfVMArray_back(&self->frames);
  BifrostVMError                   err             = BIFROST_VM_ERROR_NONE;

#define BF_RUNTIME_ERROR(...)                     \
  bfVMString_sprintf(self, &self->last_error, __VA_ARGS__); \
  goto runtime_error

/*
  NOTE(Shareef):
    Must be called after any allocation since an allocation may cause
    a GC leading to a finalizer being called and since a finalizer
    is user defined code it may do anything.
 */
#define BF_REFRESH_LOCALS() \
  locals = self->stack + frame->stack

frame_start:;
  BifrostVMStackFrame* frame          = bfVMArray_back(&self->frames);
  BifrostObjModule*    current_module = frame->fn->module;
  bfVMValue*           constants      = frame->fn->constants;
  bfVMValue*           locals         = self->stack + frame->stack;

#if 0
  // TODO(SR): Only in Debug Builds
  size_t               stack_size     = bfVMArray_size(&self->stack);

  if (stack_size < (frame->stack + frame->fn->needed_stack_space))
  {
#if _MSC_VER
    __debugbreak();
#endif
  }
#endif

  while (bfTrue)
  {
    uint8_t  op;
    uint32_t regs[4];
    int32_t  rsbx;
    bfVM_decode(*frame->ip, &op, regs + 0, regs + 1, regs + 2, regs + 3, &rsbx);

    switch (op & BIFROST_INST_OP_MASK)
    {
      case BIFROST_VM_OP_RETURN:
      {
        locals[0] = locals[regs[REG_RBx]];
        goto halt;
      }
      case BIFROST_VM_OP_LOAD_SYMBOL:
      {
        const bfVMValue     obj_value  = locals[regs[REG_RB]];
        const uint32_t      symbol     = regs[REG_RC];
        const BifrostString symbol_str = self->symbols[symbol];

        if (!bfVMValue_isPointer(obj_value))
        {
          char error_buffer[512];
          bfDbgValueToString(obj_value, error_buffer, sizeof(error_buffer));
          BF_RUNTIME_ERROR("Cannot load symbol (%s) from non object %s\n", symbol_str, error_buffer);
        }

        BifrostObj* obj = bfVMValue_asPointer(obj_value);

        if (obj->type == BIFROST_VM_OBJ_INSTANCE)
        {
          BifrostObjInstance* inst = (BifrostObjInstance*)obj;

          bfVMValue* value = bfHashMap_get(&inst->fields, self->symbols[symbol]);

          if (value)
          {
            locals[regs[REG_RA]] = *value;
          }
          else if (inst->clz)
          {
            obj = &inst->clz->super;
          }
        }
        else if (obj->type == BIFROST_VM_OBJ_REFERENCE || obj->type == BIFROST_VM_OBJ_WEAK_REF)
        {
          BifrostObjReference* inst = (BifrostObjReference*)obj;

          if (inst->clz)
          {
            obj = &inst->clz->super;
          }
        }

        if (obj->type == BIFROST_VM_OBJ_CLASS)
        {
          BifrostObjClass* original_clz = (BifrostObjClass*)obj;
          BifrostObjClass* clz          = original_clz;
          bfBool32         found_field  = bfFalse;

          while (clz)
          {
            if (symbol < bfVMArray_size(&clz->symbols) && clz->symbols[symbol].value != bfVMValue_fromNull())
            {
              locals[regs[REG_RA]] = clz->symbols[symbol].value;
              found_field          = bfTrue;
              break;
            }

            clz = clz->base_clz;
          }

          if (!found_field)
          {
            BF_RUNTIME_ERROR("'%s::%s' is not defined (also not found in any base class).\n", original_clz->name, self->symbols[symbol]);
          }
        }
        else if (obj->type == BIFROST_VM_OBJ_MODULE)
        {
          BifrostObjModule* module = (BifrostObjModule*)obj;

          locals[regs[REG_RA]] = bfVM_stackFindVariable(module, symbol_str, bfVMString_length(symbol_str));
        }
        else
        {
          BF_RUNTIME_ERROR("(%u) ERROR, loading a symbol (%s) on a non instance obj.\n", obj->type, self->symbols[symbol]);
        }
        break;
      }
      case BIFROST_VM_OP_STORE_SYMBOL:
      {
        const BifrostString sym_str   = self->symbols[regs[REG_RB]];
        const int           err_store = bfVM__stackStoreVariable(self, locals[regs[REG_RA]], bfMakeStringRangeLen(sym_str, bfVMString_length(sym_str)), locals[regs[REG_RC]]);

        if (err_store)
        {
          if (err_store == 1)
          {
            BF_RUNTIME_ERROR("Cannot store symbol into non object\n");
          }

          if (err_store == 2)
          {
            BF_RUNTIME_ERROR("ERRRO, storing a symbol on a non instance or class obj.\n");
          }
        }
        break;
      }
      case BIFROST_VM_OP_LOAD_BASIC:
      {
        const uint32_t action = regs[REG_RBx];

        if (action < BIFROST_VM_OP_LOAD_BASIC_CURRENT_MODULE)
        {
          static const bfVMValue s_Literals[] =
           {
            k_VMValueTrue,
            k_VMValueFalse,
            k_VMValueNull,
           };

          locals[regs[REG_RA]] = s_Literals[action];
        }
        else if (action == BIFROST_VM_OP_LOAD_BASIC_CURRENT_MODULE)
        {
          locals[regs[REG_RA]] = bfVMValue_fromPointer(current_module);
        }
        else
        {
          locals[regs[REG_RA]] = constants[regs[REG_RBx] - BIFROST_VM_OP_LOAD_BASIC_CONSTANT];
        }
        break;
      }
      case BIFROST_VM_OP_NEW_CLZ:
      {
        const bfVMValue value = locals[regs[REG_RBx]];

        if (bfVMValue_isPointer(value))
        {
          BifrostObj* obj = bfVMValue_asPointer(value);

          if (obj->type == BIFROST_VM_OBJ_CLASS)
          {
            BifrostObjClass* clz  = (BifrostObjClass*)obj;
            void* const      inst = bfVM_createInstance(self, clz);
            BF_REFRESH_LOCALS();
            locals[regs[REG_RA]] = bfVMValue_fromPointer(inst);
          }
          else
          {
            char string_buffer[512];
            bfDbgValueTypeToString(value, string_buffer, sizeof(string_buffer));

            BF_RUNTIME_ERROR("Called new on a non Class type (%s).\n", string_buffer);
          }
        }
        else
        {
          char string_buffer[512];
          bfDbgValueTypeToString(value, string_buffer, sizeof(string_buffer));

          BF_RUNTIME_ERROR("Called new on a non Class type (%s).\n", string_buffer);
        }
        break;
      }
      case BIFROST_VM_OP_NOT:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(bfVMValue_isThuthy(locals[regs[REG_RBx]]));
        break;
      }
      case BIFROST_VM_OP_STORE_MOVE:
      {
        locals[regs[REG_RA]] = locals[regs[REG_RBx]];
        break;
      }
      case BIFROST_VM_OP_CALL_FN:
      {
        const bfVMValue value     = locals[regs[REG_RB]];
        const uint32_t  ra        = regs[REG_RA];
        const size_t    new_stack = frame->stack + ra;
        uint32_t        num_args  = regs[REG_RC];

        if (bfVMValue_isPointer(value))
        {
          BifrostObj*               obj      = bfVMValue_asPointer(value);
          const BifrostObjInstance* instance = (const BifrostObjInstance*)obj;

          if (obj->type == BIFROST_VM_OBJ_INSTANCE || obj->type == BIFROST_VM_OBJ_REFERENCE || obj->type == BIFROST_VM_OBJ_WEAK_REF)
          {
            instance = (const BifrostObjInstance*)obj;
            obj      = instance->clz ? &instance->clz->super : obj;
          }

          if (obj->type == BIFROST_VM_OBJ_CLASS)
          {
            const BifrostObjClass* const clz      = (const BifrostObjClass*)obj;
            const size_t                 call_sym = self->build_in_symbols[BIFROST_VM_SYMBOL_CALL];

            if (call_sym < bfVMArray_size(&clz->symbols))
            {
              const bfVMValue call_value = clz->symbols[call_sym].value;

              if (bfVMValue_isPointer(call_value))
              {
                BifrostObj* const call_obj = BIFROST_AS_OBJ(call_value);

                if (call_obj->type != BIFROST_VM_OBJ_FUNCTION && call_obj->type != BIFROST_VM_OBJ_NATIVE_FN)
                {
                  BF_RUNTIME_ERROR("'%s::call' must be defined as a function to use instance as function.\n", clz->name);
                }

                if (bfVM_ensureStackspace(self, num_args + (size_t)1, locals + ra))
                {
                  BF_REFRESH_LOCALS();
                }

                bfVMValue* new_top = locals + ra;

                memmove(new_top + 1, new_top, sizeof(bfVMValue) * num_args);

                new_top[0] = bfVMValue_fromPointer(instance);
                obj        = call_obj;
                ++num_args;
              }
              else
              {
                BF_RUNTIME_ERROR("'%s::call' must be defined as a function to use instance as function.\n", clz->name);
              }
            }
            else
            {
              BF_RUNTIME_ERROR("%s does not define a 'call' function.\n", clz->name);
            }
          }

          if (obj->type == BIFROST_VM_OBJ_FUNCTION)
          {
            BifrostObjFn* const fn = (BifrostObjFn*)obj;

            if (fn->arity >= 0 && num_args != (size_t)fn->arity)
            {
              BF_RUNTIME_ERROR("Function (%s) called with %i argument(s) but requires %i.\n", fn->name, (int)num_args, (int)fn->arity);
            }

            ++frame->ip;
            bfVM_pushCallFrame(self, fn, new_stack);
            goto frame_start;
          }

          if (obj->type == BIFROST_VM_OBJ_NATIVE_FN)
          {
            BifrostObjNativeFn* const fn = (BifrostObjNativeFn*)obj;

            if (fn->arity >= 0 && num_args != (uint32_t)fn->arity)
            {
              BF_RUNTIME_ERROR("Function<native> called with %i arguments but requires %i.\n", (int)num_args, (int)fn->arity);
            }

            BifrostVMStackFrame* const native_frame = bfVM_pushCallFrame(self, NULL, new_stack);
            self->current_native_fn                 = fn;
            fn->value(self, (int32_t)num_args);
            self->current_native_fn = NULL;
            bfVM_popCallFrame(self, native_frame);

            BF_REFRESH_LOCALS();
          }
          else
          {
            BF_RUNTIME_ERROR("Not a callable value.\n");
          }
        }
        else
        {
          BF_RUNTIME_ERROR("Not a pointer value to call.\n");
        }
        break;
      }
      case BIFROST_VM_OP_MATH_ADD:
      {
        const bfVMValue lhs = locals[regs[REG_RB]];
        const bfVMValue rhs = locals[regs[REG_RC]];

        if (bfVMValue_isNumber(lhs) && bfVMValue_isNumber(rhs))
        {
          locals[regs[REG_RA]] = bfVMValue_fromNumber(bfVMValue_asNumber(lhs) + bfVMValue_asNumber(rhs));
        }
        else if ((bfVMValue_isPointer(lhs) && BIFROST_AS_OBJ(lhs)->type == BIFROST_VM_OBJ_STRING) || (bfVMValue_isPointer(rhs) && BIFROST_AS_OBJ(rhs)->type == BIFROST_VM_OBJ_STRING))
        {
          char         string_buffer[512];
          const size_t offset = bfDbgValueToString(lhs, string_buffer, sizeof(string_buffer));
          bfDbgValueToString(rhs, string_buffer + offset, sizeof(string_buffer) - offset);

          BifrostObjStr* const str_obj = bfVM_createString(self, bfMakeStringRangeC(string_buffer));

          BF_REFRESH_LOCALS();

          locals[regs[REG_RA]] = bfVMValue_fromPointer(str_obj);
        }
        else
        {
          char         string_buffer[512];
          const size_t offset = bfDbgValueTypeToString(lhs, string_buffer, sizeof(string_buffer));
          bfDbgValueTypeToString(rhs, string_buffer + offset + 1, sizeof(string_buffer) - offset - 1);

          BF_RUNTIME_ERROR("'+' operator of two incompatible types (%s + %s).", string_buffer, string_buffer + offset + 1);
        }
        break;
      }
      case BIFROST_VM_OP_MATH_SUB:
      {
        const bfVMValue lhs = locals[regs[REG_RB]];
        const bfVMValue rhs = locals[regs[REG_RC]];

        if (!bfVMValue_isNumber(lhs) || !bfVMValue_isNumber(rhs))
        {
          BF_RUNTIME_ERROR("Subtraction is not allowed on non number values.\n");
        }

        locals[regs[REG_RA]] = bfVMValue_sub(lhs, rhs);
        break;
      }
      case BIFROST_VM_OP_MATH_MUL:
      {
        locals[regs[REG_RA]] = bfVMValue_mul(locals[regs[REG_RB]], locals[regs[REG_RC]]);
        break;
      }
      case BIFROST_VM_OP_MATH_DIV:
      {
        locals[regs[REG_RA]] = bfVMValue_div(locals[regs[REG_RB]], locals[regs[REG_RC]]);
        break;
      }
      case BIFROST_VM_OP_CMP_EE:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(bfVMValue_ee(locals[regs[REG_RB]], locals[regs[REG_RC]]));
        break;
      }
      case BIFROST_VM_OP_CMP_NE:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(!bfVMValue_ee(locals[regs[REG_RB]], locals[regs[REG_RC]]));
        break;
      }
      case BIFROST_VM_OP_CMP_LT:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(bfVMValue_lt(locals[regs[REG_RB]], locals[regs[REG_RC]]));
        break;
      }
      case BIFROST_VM_OP_CMP_GT:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(bfVMValue_gt(locals[regs[REG_RB]], locals[regs[REG_RC]]));
        break;
      }
      case BIFROST_VM_OP_CMP_GE:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(bfVMValue_ge(locals[regs[REG_RB]], locals[regs[REG_RC]]));
        break;
      }
      case BIFROST_VM_OP_CMP_AND:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(bfVMValue_isThuthy(locals[regs[REG_RB]]) && bfVMValue_isThuthy(locals[regs[REG_RC]]));
        break;
      }
      case BIFROST_VM_OP_CMP_OR:
      {
        locals[regs[REG_RA]] = bfVMValue_fromBool(bfVMValue_isThuthy(locals[regs[REG_RB]]) || bfVMValue_isThuthy(locals[regs[REG_RC]]));
        break;
      }
      case BIFROST_VM_OP_JUMP:
      {
        frame->ip += rsbx;
        continue;
      }
      case BIFROST_VM_OP_JUMP_IF:
      {
        if (bfVMValue_isThuthy(locals[regs[REG_RA]]))
        {
          frame->ip += rsbx;
          continue;
        }
        break;
      }
      case BIFROST_VM_OP_JUMP_IF_NOT:
      {
        if (!bfVMValue_isThuthy(locals[regs[REG_RA]]))
        {
          frame->ip += rsbx;
          continue;
        }
        break;
      }
      default:
      {
        BF_RUNTIME_ERROR("Invalid OP: %i\n", (int)op);
      }
    }

    ++frame->ip;
  }

#undef BF_RUNTIME_ERROR
#undef BF_REFRESH_LOCALS

runtime_error:
  bfVM_popAllCallFrames(self, reference_frame);
  err = BIFROST_VM_ERROR_RUNTIME;
  goto done;

halt:
  bfVM_popCallFrame(self, frame);

  if (reference_frame < frame)
  {
    goto frame_start;
  }

done:
  return err;
}

BifrostVMError bfVM_call(BifrostVM* self, size_t idx, size_t args_start, int32_t num_args)
{
  bfVM_assertStackIndex(self, idx);
  const bfVMValue value = self->stack_top[idx];
  BifrostVMError  err   = BIFROST_VM_ERROR_NONE;

  assert(bfVMValue_isPointer(value));

  BifrostObj* obj = bfVMValue_asPointer(value);

  const size_t base_stack = (self->stack_top - self->stack);

  if (obj->type == BIFROST_VM_OBJ_FUNCTION)
  {
    /* NOTE(Shareef):
        The 'bfVM_execTopFrame' function automatically pops the
        stackframe once the call is done.
    */

    BifrostObjFn* const fn = (BifrostObjFn*)obj;

    if (fn->arity < 0 || fn->arity == num_args)
    {
      bfVM_pushCallFrame(self, fn, base_stack + args_start);
      err = bfVM_execTopFrame(self);
    }
    else
    {
      err = BIFROST_VM_ERROR_FUNCTION_ARITY_MISMATCH;
    }
  }
  else if (obj->type == BIFROST_VM_OBJ_NATIVE_FN)
  {
    BifrostObjNativeFn* native_fn = (BifrostObjNativeFn*)obj;

    if (native_fn->arity < 0 || native_fn->arity == num_args)
    {
      /* TODO(SR):
         Add an API to be able to set errors from user defined functions.
      */
      BifrostVMStackFrame* frame = bfVM_pushCallFrame(self, NULL, base_stack + args_start);
      native_fn->value(self, num_args);
      bfVM_popCallFrame(self, frame);
    }
    else
    {
      err = BIFROST_VM_ERROR_FUNCTION_ARITY_MISMATCH;
    }
  }
  else
  {
    assert(!"bfVM_call called with a non function object.");
  }

  return err;
}

BifrostVMError bfVM_execInModule(BifrostVM* self, const char* module, const char* source, size_t source_length)
{
  BifrostObjModule* module_obj;
  BifrostVMError    err = bfVM__moduleMake(self, module, &module_obj);

  if (!err)
  {
    bfGCPushRoot(self, &module_obj->super);

    // Short Circuit. The || operator turns ints into bools (0 or 1) so can't assign directly.
    ((err = bfVM_compileIntoModule(self, module_obj, source, source_length))) || ((err = bfVM_runModule(self, module_obj)));

    bfVM_stackResize(self, 1);
    self->stack_top[0] = bfVMValue_fromPointer(module_obj);
    bfGCPopRoot(self);
  }

  return err;
}

void bfVM_gc(BifrostVM* self)
{
  if (!self->gc_is_running)
  {
    self->gc_is_running = bfTrue;
    bfGCCollect(self);
    self->gc_is_running = bfFalse;
  }
}

const char* bfVM_buildInSymbolStr(const BifrostVM* self, BifrostVMBuildInSymbol symbol)
{
  (void)self;
  static const char* const k_EnumToString[] =
   {
    "ctor",
    "dtor",
    "call",
    "__error__",
   };

  return k_EnumToString[symbol];
}

ConstBifrostString bfVM_errorString(const BifrostVM* self)
{
  return self->last_error;
}

void bfVM_dtor(BifrostVM* self)
{
  BifrostObj* garbage_list = self->gc_object_list;

  while (garbage_list)
  {
    void* const next = garbage_list->next;
    bfObjFinalize(self, garbage_list);
    garbage_list = next;
  }

  while (self->gc_object_list)
  {
    void* const next = self->gc_object_list->next;
    bfVMObject_delete(self, self->gc_object_list);
    self->gc_object_list = next;
  }

  while (self->finalized)
  {
    void* const next = self->finalized->next;
    bfVMObject_delete(self, self->finalized);
    self->finalized = next;
  }

  const size_t num_symbols = bfVMArray_size(&self->symbols);

  for (size_t i = 0; i < num_symbols; ++i)
  {
    bfVMString_delete(self, self->symbols[i]);
  }

  bfVMArray_delete(self, &self->symbols);
  bfVMArray_delete(self, &self->frames);
  bfVMArray_delete(self, &self->stack);
  bfHashMap_dtor(&self->modules);
  bfVMString_delete(self, self->last_error);

  while (self->free_handles)
  {
    const bfValueHandle next = self->free_handles->next;  // NOLINT(misc-misplaced-const)
    bfGCAllocMemory(self, self->free_handles, sizeof(struct bfValueHandle_t), 0u);
    self->free_handles = next;
  }

  if (self->handles)
  {
    assert(!"You are leaking a handle to a VM Object.");
  }
}

void bfVM_delete(BifrostVM* self)
{
  bfVM_dtor(self);
  bfGCAllocMemory(self, self, sizeof(BifrostVM), 0u);
}

BifrostObjModule* bfVM_findModule(BifrostVM* self, const char* name, size_t name_len)
{
  const size_t    hash    = bfVMString_hashN(name, name_len);
  BifrostHashMap* modules = &self->modules;

  bfHashMapFor(it, modules)
  {
    const BifrostObjStr* key     = it.key;
    const size_t         key_len = bfVMString_length(key->value);

    if (key->hash == hash && key_len == name_len && bfVMString_ccmpn(key->value, name, name_len) == 0)
    {
      return *(void**)it.value;
    }
  }

  return NULL;
}

static int bfVM_getSymbolHelper(const void* lhs, const void* rhs)
{
  const bfStringRange* const name    = lhs;
  const BifrostString* const sym     = (void*)rhs;
  const size_t               lhs_len = bfStringRange_length(name);
  const size_t               rhs_len = bfVMString_length(*sym);

  return lhs_len == rhs_len && bfVMString_ccmpn(*sym, name->str_bgn, lhs_len) == 0;
}

uint32_t bfVM_getSymbol(BifrostVM* self, bfStringRange name)
{
  size_t idx = bfVMArray_find(&self->symbols, &name, &bfVM_getSymbolHelper);

  if (idx == BIFROST_ARRAY_INVALID_INDEX)
  {
    idx                = bfVMArray_size(&self->symbols);
    BifrostString* sym = bfVMArray_emplace(self, &self->symbols);
    *sym               = bfVMString_newLen(self, name.str_bgn, bfStringRange_length(&name));
  }

  return (uint32_t)idx;
}

static BifrostVMError bfVM_runModule(BifrostVM* self, BifrostObjModule* module)
{
  const size_t old_top = self->stack_top - self->stack;
  bfVM_pushCallFrame(self, &module->init_fn, old_top);
  const BifrostVMError err = bfVM_execTopFrame(self);
  return err;
}

static BifrostVMError bfVM_compileIntoModule(BifrostVM* self, BifrostObjModule* module, const char* source, size_t source_len)
{
#define KEYWORD(kw, tt)                  \
  {                                      \
    kw, sizeof(kw) - 1,                  \
    {                                    \
      .type = (tt), .as = {.str = (kw) } \
    }                                    \
  }

  static const bfKeyword s_Keywords[] =
   {
    KEYWORD("true", CONST_BOOL),
    KEYWORD("false", CONST_BOOL),
    KEYWORD("return", CTRL_RETURN),
    KEYWORD("if", CTRL_IF),
    KEYWORD("else", CTRL_ELSE),
    KEYWORD("for", CTRL_FOR),
    KEYWORD("while", BIFROST_TOKEN_CTRL_WHILE),
    KEYWORD("func", FUNC),
    KEYWORD("var", VAR_DECL),
    KEYWORD("nil", CONST_NIL),
    KEYWORD("class", BIFROST_TOKEN_CLASS),
    KEYWORD("import", IMPORT),
    KEYWORD("break", BIFROST_TOKEN_CTRL_BREAK),
    KEYWORD("new", BIFROST_TOKEN_NEW),
    KEYWORD("static", BIFROST_TOKEN_STATIC),
    KEYWORD("as", BIFROST_TOKEN_AS),
    KEYWORD("super", BIFROST_TOKEN_SUPER),
   };
#undef KEYWORD

  const BifrostLexerParams lex_params =
   {
    .source       = source,
    .length       = source_len,
    .keywords     = s_Keywords,
    .num_keywords = bfCArraySize(s_Keywords),
    .vm           = self,
    .do_comments  = bfTrue,
   };

  BifrostLexer lexer = bfLexer_make(&lex_params);

  BifrostParser parser;
  bfParser_ctor(&parser, self, &lexer, module);
  const bfBool32 has_error = bfParser_compile(&parser);
  bfParser_dtor(&parser);

  return has_error ? BIFROST_VM_ERROR_COMPILE : BIFROST_VM_ERROR_NONE;
}

BifrostObjModule* bfVM_importModule(BifrostVM* self, const char* from, const char* name, size_t name_len)
{
  BifrostObjModule* m = bfVM_findModule(self, name, name_len);

  if (!m)
  {
    const bfModuleFn module_fn = self->params.module_fn;

    if (module_fn)
    {
      const bfStringRange name_range  = {.str_bgn = name, .str_end = name + name_len};
      BifrostObjStr*      module_name = bfVM_createString(self, name_range);

      bfGCPushRoot(self, &module_name->super);

      BifrostVMModuleLookUp look_up =
       {
        .source     = NULL,
        .source_len = 0u,
       };

      module_fn(self, from, module_name->value, &look_up);

      if (look_up.source && look_up.source_len)
      {
        m = bfVM_createModule(self, name_range);

        bfGCPushRoot(self, &m->super);

        // NOTE(Shareef): No error is 0. So if an error occurs we short-circuit
        const bfBool32 has_error = bfVM_compileIntoModule(self, m, look_up.source, look_up.source_len) || bfVM_runModule(self, m);

        if (!has_error)
        {
          bfHashMap_set(&self->modules, module_name, &m);
        }

        // m
        bfGCPopRoot(self);
        bfGCAllocMemory(self, (void*)look_up.source, look_up.source_len, 0u);
      }
      else
      {
        bfVMString_sprintf(self, &self->last_error, "Failed to find module '%.*s'", name_len, name);
      }

      // module_name
      bfGCPopRoot(self);
    }
    else
    {
      bfVMString_sprintf(self, &self->last_error, "No module function registered when loading module '%.*s'", name_len, name);
    }
  }

  return m;
}
