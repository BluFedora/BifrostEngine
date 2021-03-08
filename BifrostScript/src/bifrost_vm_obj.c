/******************************************************************************/
/*!
 * @file   bifrost_vm_obj.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   Handles the object's available to the vm runtime.
 * 
 * @version 0.0.1
 * @date    2020-02-16
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#include "bifrost_vm_obj.h"

#include "bifrost_vm_gc.h"  // Allocation Functions

#include <string.h>  // memset

static inline void SetupGCObject(BifrostObj* obj, BifrostVMObjType type, BifrostObj** next)
{
  obj->type    = type;
  obj->next    = next ? *next : NULL;
  obj->gc_mark = 0;

  if (next)
  {
    *next = obj;
  }
}

static void* allocObj(struct BifrostVM_t* self, size_t size, BifrostVMObjType type)
{
  BifrostObj* obj = bfGCAllocMemory(self, NULL, 0u, size);

  memset(obj, 0xFD, size);

  return obj;
}

BifrostObjModule* bfVM_createModule(struct BifrostVM_t* self, bfStringRange name)
{
  BifrostObjModule* module = allocObj(self, sizeof(BifrostObjModule), BIFROST_VM_OBJ_MODULE);

  module->name      = bfVMString_newLen(self, name.str_bgn, bfStringRange_length(&name));
  module->variables = bfVMArray_newA(self, module->variables, 32);
  memset(&module->init_fn, 0x0, sizeof(module->init_fn));
  module->init_fn.module = module;

  SetupGCObject(&module->init_fn.super, BIFROST_VM_OBJ_FUNCTION, NULL);
  SetupGCObject(&module->super, BIFROST_VM_OBJ_MODULE, &self->gc_object_list);

  return module;
}

BifrostObjClass* bfVM_createClass(struct BifrostVM_t* self, BifrostObjModule* module, bfStringRange name, BifrostObjClass* base_clz, size_t extra_data)
{
  BifrostObjClass* clz = allocObj(self, sizeof(BifrostObjClass), BIFROST_VM_OBJ_CLASS);

  clz->name               = bfVMString_newLen(self, name.str_bgn, bfStringRange_length(&name));
  clz->base_clz           = base_clz;
  clz->module             = module;
  clz->symbols            = bfVMArray_newA(self, clz->symbols, 32);
  clz->field_initializers = bfVMArray_newA(self, clz->field_initializers, 32);
  clz->extra_data         = extra_data;
  clz->finalizer          = NULL;

  SetupGCObject(&clz->super, BIFROST_VM_OBJ_CLASS, &self->gc_object_list);

  return clz;
}

BifrostObjInstance* bfVM_createInstance(struct BifrostVM_t* self, BifrostObjClass* clz)
{
  BifrostObjInstance* inst = allocObj(self, sizeof(BifrostObjInstance) + clz->extra_data, BIFROST_VM_OBJ_INSTANCE);

  BifrostHashMapParams hash_params;
  bfHashMapParams_init(&hash_params, self);
  hash_params.value_size = sizeof(bfVMValue);
  // hash_params.hash = ModuleMap_hash;
  // hash_params.cmp  = ModuleMap_cmp;

  bfHashMap_ctor(&inst->fields, &hash_params);
  inst->clz = clz;

  const size_t num_fields = bfVMArray_size(&clz->field_initializers);

  for (size_t i = 0; i < num_fields; ++i)
  {
    BifrostVMSymbol* const sym = clz->field_initializers + i;

    bfHashMap_set(&inst->fields, sym->name, &sym->value);
  }

  SetupGCObject(&inst->super, BIFROST_VM_OBJ_INSTANCE, &self->gc_object_list);

  return inst;
}

BifrostObjFn* bfVM_createFunction(struct BifrostVM_t* self, BifrostObjModule* module)
{
  BifrostObjFn* fn = allocObj(self, sizeof(BifrostObjFn), BIFROST_VM_OBJ_FUNCTION);

  fn->module = module;

  /* NOTE(Shareef): 'fn' Will be filled out later by a Function Builder. */

  SetupGCObject(&fn->super, BIFROST_VM_OBJ_FUNCTION, &self->gc_object_list);

  return fn;
}

BifrostObjNativeFn* bfVM_createNativeFn(struct BifrostVM_t* self, bfNativeFnT fn_ptr, int32_t arity, uint32_t num_statics, uint16_t extra_data)
{
  BifrostObjNativeFn* fn = allocObj(self, sizeof(BifrostObjNativeFn) + sizeof(bfVMValue) * num_statics + extra_data, BIFROST_VM_OBJ_NATIVE_FN);

  fn->value           = fn_ptr;
  fn->arity           = arity;
  fn->num_statics     = num_statics;
  fn->statics         = (bfVMValue*)((char*)fn + sizeof(BifrostObjNativeFn));
  fn->extra_data_size = extra_data;

  SetupGCObject(&fn->super, BIFROST_VM_OBJ_NATIVE_FN, &self->gc_object_list);

  return fn;
}

BifrostObjStr* bfVM_createString(struct BifrostVM_t* self, bfStringRange value)
{
  BifrostObjStr* obj = allocObj(self, sizeof(BifrostObjStr), BIFROST_VM_OBJ_STRING);

  obj->value = bfVMString_newLen(self, value.str_bgn, bfStringRange_length(&value));
  bfVMString_unescape(obj->value);
  obj->hash = bfVMString_hashN(obj->value, bfVMString_length(obj->value));

  SetupGCObject(&obj->super, BIFROST_VM_OBJ_STRING, &self->gc_object_list);

  return obj;
}

BifrostObjReference* bfVM_createReference(struct BifrostVM_t* self, size_t extra_data_size)
{
  BifrostObjReference* obj = allocObj(self, sizeof(BifrostObjReference) + extra_data_size, BIFROST_VM_OBJ_REFERENCE);

  obj->clz             = NULL;
  obj->extra_data_size = extra_data_size;
  memset(&obj->extra_data, 0x0, extra_data_size);

  SetupGCObject(&obj->super, BIFROST_VM_OBJ_REFERENCE, &self->gc_object_list);

  return obj;
}

BifrostObjWeakRef* bfVM_createWeakRef(struct BifrostVM_t* self, void* data)
{
  BifrostObjWeakRef* obj = allocObj(self, sizeof(BifrostObjWeakRef), BIFROST_VM_OBJ_WEAK_REF);

  obj->clz  = NULL;
  obj->data = data;

  SetupGCObject(&obj->super, BIFROST_VM_OBJ_WEAK_REF, &self->gc_object_list);

  return obj;
}

void bfVMObject__delete(struct BifrostVM_t* self, BifrostObj* obj)
{
  switch (obj->type & BifrostVMObjType_mask)
  {
    case BIFROST_VM_OBJ_MODULE:
    {
      BifrostObjModule* const module = (BifrostObjModule*)obj;
      bfVMString_delete(self, module->name);
      bfVMArray_delete(self, &module->variables);
      if (module->init_fn.name)
      {
        bfVMObject__delete(self, &module->init_fn.super);
      }
      break;
    }
    case BIFROST_VM_OBJ_CLASS:
    {
      BifrostObjClass* const clz = (BifrostObjClass*)obj;

      bfVMString_delete(self, clz->name);
      bfVMArray_delete(self, &clz->symbols);
      bfVMArray_delete(self, &clz->field_initializers);
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

      bfVMString_delete(self, fn->name);
      bfVMArray_delete(self, &fn->constants);
      bfVMArray_delete(self, &fn->instructions);
      bfVMArray_delete(self, &fn->code_to_line);
      break;
    }
    case BIFROST_VM_OBJ_NATIVE_FN:
    {
      break;
    }
    case BIFROST_VM_OBJ_STRING:
    {
      BifrostObjStr* const str = (BifrostObjStr*)obj;
      bfVMString_delete(self, str->value);
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
  bfGCAllocMemory(self, obj, obj_size, 0u);
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

/* array */

#include <assert.h> /* assert         */
#include <stdint.h> /* uint8_t        */
#include <string.h> /* memcpy         */

typedef struct
{
  size_t      stride;
  const void* key;

} ArrayDefaultCompareData;

#define PRISM_ASSERT(arg, msg) assert((arg) && (msg))
#define SELF_CAST(s) ((unsigned char**)(s))
#define PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS 1  // Disable for a faster 'bfVMArray_at'.

typedef struct
{
  size_t capacity;
  size_t size;
  size_t stride;

} BifrostArrayHeader;

static BifrostArrayHeader* Array_getHeader(unsigned char* self)
{
  return (BifrostArrayHeader*)(self - sizeof(BifrostArrayHeader));
}

static size_t ArrayAllocationSize(size_t capacity, size_t stride)
{
  return sizeof(BifrostArrayHeader) + capacity * stride;
}

void* _bfVMArrayT_new(struct BifrostVM_t* vm, const size_t stride, const size_t initial_capacity)
{
  PRISM_ASSERT(stride, "_ArrayT_new:: The struct must be greater than 0.");
  PRISM_ASSERT(initial_capacity * stride, "_ArrayT_new:: Please initialize the Array with a size greater than 0");

  vm->gc_is_running              = bfTrue;
  BifrostArrayHeader* const self = (BifrostArrayHeader*)bfGCAllocMemory(vm, NULL, 0u, ArrayAllocationSize(initial_capacity, stride));
  vm->gc_is_running              = bfFalse;

  PRISM_ASSERT(self, "Array_new:: The Dynamic Array could not be allocated");

  if (!self)
  {
    return NULL;
  }

  self->capacity = initial_capacity;
  self->size     = 0;
  self->stride   = stride;

  return (uint8_t*)self + sizeof(BifrostArrayHeader);
}

// bfVMArray_push
void* Array_end(const void* const self)
{
  BifrostArrayHeader* const header = Array_getHeader(*SELF_CAST(self));
  return *(char**)self + (header->size * header->stride);
}

size_t bfVMArray_size(const void* const self)
{
  return Array_getHeader(*SELF_CAST(self))->size;
}

void bfVMArray_clear(void* const self)
{
  Array_getHeader(*SELF_CAST(self))->size = 0;
}

void Array_reserve(struct BifrostVM_t* vm, void* const self, const size_t num_elements)
{
  BifrostArrayHeader* header = Array_getHeader(*SELF_CAST(self));

  if (header->capacity < num_elements)
  {
    size_t new_capacity = (header->capacity >> 3) + (header->capacity < 9 ? 3 : 6) + header->capacity;

    if (new_capacity < num_elements)
    {
      new_capacity = num_elements;
    }

    vm->gc_is_running              = bfTrue;
    BifrostArrayHeader* new_header = (BifrostArrayHeader*)bfGCAllocMemory(
     vm,
     header,
     ArrayAllocationSize(header->capacity, header->stride),
     ArrayAllocationSize(new_capacity, header->stride));

    if (new_header)
    {
      new_header->capacity = new_capacity;
      *SELF_CAST(self)     = (unsigned char*)new_header + sizeof(BifrostArrayHeader);
    }
    else
    {
      bfVMArray_delete(vm, self);
      *SELF_CAST(self) = NULL;
    }

    vm->gc_is_running = bfFalse;
  }
}

void bfVMArray_resize(struct BifrostVM_t* vm, void* const self, const size_t size)
{
  Array_reserve(vm, self, size);
  Array_getHeader(*SELF_CAST(self))->size = size;
}

void bfVMArray_push(struct BifrostVM_t* vm, void* const self, const void* const data)
{
  const size_t stride = Array_getHeader(*SELF_CAST(self))->stride;

  Array_reserve(vm, self, bfVMArray_size(self) + 1);

  memcpy(Array_end(self), data, stride);
  ++Array_getHeader(*SELF_CAST(self))->size;
}

void* bfVMArray_emplace(struct BifrostVM_t* vm, void* const self)
{
  return bfVMArray_emplaceN(vm, self, 1);
}

void* bfVMArray_emplaceN(struct BifrostVM_t* vm, void* const self, const size_t num_elements)
{
  const size_t old_size = bfVMArray_size(self);
  Array_reserve(vm, self, old_size + num_elements);
  uint8_t* const      new_element = Array_end(self);
  BifrostArrayHeader* header      = Array_getHeader(*SELF_CAST(self));
  memset(new_element, 0x0, header->stride * num_elements);
  header->size += num_elements;
  return new_element;
}

static int Array_findDefaultCompare(const void* lhs, const void* rhs)
{
  ArrayDefaultCompareData* data = (ArrayDefaultCompareData*)lhs;
  return memcmp(data->key, rhs, data->stride) == 0;
}

size_t bfVMArray_find(const void* const self, const void* key, bfVMArrayFindCompare compare)
{
  const size_t len = bfVMArray_size(self);

  if (compare)
  {
    for (size_t i = 0; i < len; ++i)
    {
      if (compare(key, bfVMArray_at(self, i)))
      {
        return i;
      }
    }
  }
  else
  {
    const size_t            stride = Array_getHeader(*SELF_CAST(self))->stride;
    ArrayDefaultCompareData data   = {stride, key};

    for (size_t i = 0; i < len; ++i)
    {
      if (Array_findDefaultCompare(&data, bfVMArray_at(self, i)))
      {
        return i;
      }
    }
  }

  return BIFROST_ARRAY_INVALID_INDEX;
}

void* bfVMArray_at(const void* const self, const size_t index)
{
#if PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS
  const size_t size         = bfVMArray_size(self);
  const int    is_in_bounds = index < size;

  PRISM_ASSERT(is_in_bounds, "Array_at:: index array out of bounds error");
#endif /* PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS */

  return *(char**)self + (Array_getHeader(*SELF_CAST(self))->stride * index);
}

void* bfVMArray_pop(void* const self)
{
#if PRISM_DATA_STRUCTURES_ARRAY_CHECK_BOUNDS
  PRISM_ASSERT(bfVMArray_size(self) != 0, "Array_pop:: attempt to pop empty array");
#endif

  BifrostArrayHeader* const header      = Array_getHeader(*SELF_CAST(self));
  void* const               old_element = bfVMArray_at(self, header->size - 1);
  --header->size;

  return old_element;
}

void* bfVMArray_back(const void* const self)
{
  const BifrostArrayHeader* const header = Array_getHeader(*SELF_CAST(self));

  return (char*)Array_end(self) - header->stride;
}

void bfVMArray_delete(struct BifrostVM_t* vm, void* const self)
{
  vm->gc_is_running = bfTrue;

  BifrostArrayHeader* const header = Array_getHeader(*SELF_CAST(self));

  bfGCAllocMemory(vm, header, ArrayAllocationSize(header->capacity, header->stride), 0u);

  vm->gc_is_running = bfFalse;
}

/* string */

typedef struct
{
  size_t capacity;
  size_t length;

} BifrostStringHeader;

static BifrostStringHeader* bfVMString_getHeader(ConstBifrostString self);

static size_t StringAllocationSize(size_t capacity)
{
  return sizeof(BifrostStringHeader) + capacity;
}

BifrostString bfVMString_new(struct BifrostVM_t* vm, const char* initial_data)
{
  return bfVMString_newLen(vm, initial_data, strlen(initial_data));
}

BifrostString bfVMString_newLen(struct BifrostVM_t* vm, const char* initial_data, size_t string_length)
{
  const size_t str_capacity = string_length + 1;
  const size_t total_size   = StringAllocationSize(str_capacity);

  BifrostStringHeader* const self = bfGCAllocMemory(vm, NULL, 0u, total_size);

  if (self)
  {
    self->capacity   = str_capacity;
    self->length     = string_length;
    char* const data = (char*)self + sizeof(BifrostStringHeader);

    /*
     // NOTE(Shareef):
     //   According to the standard memcpy cannot take in a NULL
     //   pointer and "size" must be non-zero, kinda stupid but ok.
    */
    if (initial_data && string_length)
    {
      memcpy(data, initial_data, string_length);
    }

    data[string_length] = '\0';

    return data;
  }

  return NULL;
}

const char* bfVMString_cstr(ConstBifrostString self)
{
  return self;
}

size_t bfVMString_length(ConstBifrostString self)
{
  return bfVMString_getHeader(self)->length;
}

void bfVMString_reserve(struct BifrostVM_t* vm, BifrostString* self, size_t new_capacity)
{
  BifrostStringHeader* header = bfVMString_getHeader(*self);

  if (new_capacity > header->capacity)
  {
    const size_t old_capacity = header->capacity;

    while (header->capacity < new_capacity)
    {
      header->capacity *= 2;
    }

    vm->gc_is_running = bfTrue;

    header = (BifrostStringHeader*)bfGCAllocMemory(
     vm,
     header,
     StringAllocationSize(old_capacity),
     StringAllocationSize(header->capacity));

    if (header)
    {
      *self = (char*)header + sizeof(BifrostStringHeader);
    }
    else
    {
      bfVMString_delete(vm, *self);
      *self = NULL;
    }

    vm->gc_is_running = bfFalse;
  }
}

void bfVMString_sprintf(struct BifrostVM_t* vm, BifrostString* self, const char* format, ...)
{
  va_list args, args_cpy;
  va_start(args, format);

  va_copy(args_cpy, args);
  const size_t num_chars = (size_t)vsnprintf(NULL, 0, format, args_cpy);
  va_end(args_cpy);

  bfVMString_reserve(vm, self, num_chars + 2ul);
  vsnprintf(*self, num_chars + 1ul, format, args);
  bfVMString_getHeader(*self)->length = num_chars;

  va_end(args);
}

static unsigned char EscapeConvert(const unsigned char c)
{
  switch (c)
  {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '\\': return '\\';
    case '\'': return '\'';
    case '\"': return '\"';
    case '?': return '\?';
    default: return c;
  }
}

static size_t CString_unescape(char* str)
{
  const char* oldStr = str;
  char*       newStr = str;

  while (*oldStr)
  {
    unsigned char c = *(unsigned char*)(oldStr++);

    if (c == '\\')
    {
      c = *(unsigned char*)(oldStr++);
      if (c == '\0') break;
      c = EscapeConvert(c);
    }

    *newStr++ = (char)c;
  }

  *newStr = '\0';

  return (newStr - str);
}

void bfVMString_unescape(BifrostString self)
{
  bfVMString_getHeader(self)->length = CString_unescape(self);
}

int bfVMString_cmp(ConstBifrostString self, ConstBifrostString other)
{
  const size_t len1 = bfVMString_length(self);
  const size_t len2 = bfVMString_length(other);

  if (len1 != len2)
  {
    return -1;
  }

  return strncmp(self, other, len1);
}

int bfVMString_ccmpn(ConstBifrostString self, const char* other, size_t length)
{
  if (length > bfVMString_length(self))
  {
    return -1;
  }

  return strncmp(bfVMString_cstr(self), other, length);
}

uint32_t bfVMString_hash(const char* str)
{
  uint32_t hash = 0x811c9dc5;

  while (*str)
  {
    hash ^= (unsigned char)*str++;
    hash *= 0x01000193;
  }

  return hash;
}

uint32_t bfVMString_hashN(const char* str, size_t length)
{
  uint32_t hash = 0x811c9dc5;

  const char* str_end = str + length;

  while (str != str_end)
  {
    hash ^= (unsigned char)*str;
    hash *= 0x01000193;
    ++str;
  }

  return hash;
}

void bfVMString_delete(struct BifrostVM_t* vm, BifrostString self)
{
  vm->gc_is_running = bfTrue;

  BifrostStringHeader* const header = bfVMString_getHeader(self);

  bfGCAllocMemory(vm, header, StringAllocationSize(header->capacity), 0u);

  vm->gc_is_running = bfFalse;
}

static BifrostStringHeader* bfVMString_getHeader(ConstBifrostString self)
{
  return ((BifrostStringHeader*)(self)) - 1;
}
