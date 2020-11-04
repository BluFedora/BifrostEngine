/******************************************************************************/
/*!
 * @file   bifrost_vm_obj.h
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
#ifndef BIFROST_VM_OBJ_H
#define BIFROST_VM_OBJ_H

#include "bifrost/script/bifrost_vm.h" /* bfNativeFnT, bfClassFinalizer, bfVMValue, BifrostHashMap */

#if __cplusplus
extern "C" {
#endif
struct BifrostVM_t;
struct BifrostObjModule_t;

typedef enum
{
  BIFROST_VM_OBJ_FUNCTION,   // 0b000
  BIFROST_VM_OBJ_MODULE,     // 0b001
  BIFROST_VM_OBJ_CLASS,      // 0b010
  BIFROST_VM_OBJ_INSTANCE,   // 0b011
  BIFROST_VM_OBJ_STRING,     // 0b100
  BIFROST_VM_OBJ_NATIVE_FN,  // 0b101
  BIFROST_VM_OBJ_REFERENCE,  // 0b110
  BIFROST_VM_OBJ_WEAK_REF,   // 0b111

} BifrostVMObjType;

#define BifrostVMObjType_mask 0x7 /*!< 0b111 */

typedef struct
{
  ConstBifrostString name;  /*!< Non owning string, [BifrostVM::symbols] is the owner. */
  bfVMValue          value; /*!< The associated value.                                 */

} BifrostVMSymbol;

typedef struct BifrostObj_t
{
  // NOTE(Shareef):
  //  If we needed to save space 'type' and 'gc_mark' can probably be combined
  //  into a single 'uint8_t'

  BifrostVMObjType     type;
  struct BifrostObj_t* next;
  unsigned char        gc_mark;

} BifrostObj;

typedef struct BifrostObjFn_t
{
  BifrostObj                 super;
  BifrostString              name;
  int32_t                    arity;  //!< An arity of -1 indicates infinite (read: 0-511) params
  uint16_t*                  code_to_line;
  bfVMValue*                 constants;
  bfInstruction*             instructions;
  size_t                     needed_stack_space; /* params + locals + temps */
  struct BifrostObjModule_t* module;

} BifrostObjFn;

typedef struct BifrostObjModule_t
{
  BifrostObj       super;
  BifrostString    name;
  BifrostVMSymbol* variables;
  BifrostObjFn     init_fn;

} BifrostObjModule;

typedef struct BifrostObjClass_t
{
  BifrostObj                super;
  BifrostString             name;
  struct BifrostObjClass_t* base_clz;
  BifrostObjModule*         module;
  BifrostVMSymbol*          symbols;
  BifrostVMSymbol*          field_initializers;
  size_t                    extra_data;
  bfClassFinalizer          finalizer;

} BifrostObjClass;

typedef struct BifrostObjInstance_t
{
  BifrostObj       super;
  BifrostObjClass* clz;
  BifrostHashMap   fields;                   // <ConstBifrostString (Non owning string, [BifrostVM::symbols] is the owner), bfVMValue>
  char             extra_data[bfStructHack]; /* This is for native class data. */

} BifrostObjInstance;

typedef struct BifrostObjStr_t
{
  BifrostObj    super;
  BifrostString value;
  unsigned      hash;

} BifrostObjStr;

typedef struct BifrostObjNativeFn_t
{
  BifrostObj  super;
  bfNativeFnT value;
  int32_t     arity;
  uint32_t    num_statics;
  bfVMValue*  statics;  // Fixed size array.
  uint16_t    extra_data_size;
  char        extra_data[bfStructHack]; /* This is for native data. */

} BifrostObjNativeFn;

typedef struct BifrostObjReference_t
{
  // NOTE(Shareef): Matches the same beginning as 'BifrostObjInstance'.
  BifrostObj       super;
  BifrostObjClass* clz;  // Optional
  size_t           extra_data_size;
  char             extra_data[bfStructHack]; /* This is for native data. */

} BifrostObjReference;

typedef struct BifrostObjWeakRef_t
{
  // NOTE(Shareef): Matches the same beginning as 'BifrostObjInstance'.
  BifrostObj       super;
  BifrostObjClass* clz;  // Optional
  void*            data;

} BifrostObjWeakRef;

typedef struct BifrostVMStackFrame_t
{
  BifrostObjFn*  fn;        /*!< Needed for addition debug info for stack traces. */
  bfInstruction* ip;        /*!< The current instruction being executed.          */
  size_t         old_stack; /*!< The top of the stack to restore to.              */
  size_t         stack;     /*!< The place where this stacks locals start.        */

} BifrostVMStackFrame;

#define BIFROST_AS_OBJ(value) ((BifrostObj*)bfVMValue_asPointer((value)))

BifrostObjModule*    bfVM_createModule(struct BifrostVM_t* self, bfStringRange name);
BifrostObjClass*     bfVM_createClass(struct BifrostVM_t* self, BifrostObjModule* module, bfStringRange name, BifrostObjClass* base_clz, size_t extra_data);
BifrostObjInstance*  bfVM_createInstance(struct BifrostVM_t* self, BifrostObjClass* clz);
BifrostObjFn*        bfVM_createFunction(struct BifrostVM_t* self, BifrostObjModule* module);
BifrostObjNativeFn*  bfVM_createNativeFn(struct BifrostVM_t* self, bfNativeFnT fn_ptr, int32_t arity, uint32_t num_statics, uint16_t extra_data);
BifrostObjStr*       bfVM_createString(struct BifrostVM_t* self, bfStringRange value);
BifrostObjReference* bfVM_createReference(struct BifrostVM_t* self, size_t extra_data_size);
BifrostObjWeakRef*   bfVM_createWeakRef(struct BifrostVM_t* self, void* data);
void                 bfVMObject_delete(struct BifrostVM_t* self, BifrostObj* obj);
bfBool32             bfObjIsFunction(const BifrostObj* obj);
void                 bfObjFinalize(struct BifrostVM_t* self, BifrostObj* obj);

/*
  Data Structures
*/

/* array */

#define BIFROST_ARRAY_INVALID_INDEX ((size_t)(-1))
#define bfVMArray_new(vm, T, initial_size) (T*)_bfVMArrayT_new((vm), sizeof(T), (initial_size))
#define bfVMArray_newA(vm, arr, initial_size) _bfVMArrayT_new((vm), sizeof((arr)[0]), (initial_size))

typedef int (*bfVMArrayFindCompare)(const void*, const void*);

void*  _bfVMArrayT_new(struct BifrostVM_t* vm, const size_t stride, const size_t initial_size);
size_t bfVMArray_size(const void* const self);
void*  bfVMArray_at(const void* const self, const size_t index);
void   bfVMArray_resize(struct BifrostVM_t* vm, void* const self, const size_t size);
void*  bfVMArray_emplace(struct BifrostVM_t* vm, void* const self);
void*  bfVMArray_emplaceN(struct BifrostVM_t* vm, void* const self, const size_t num_elements);
void*  bfVMArray_pop(void* const self);
void*  bfVMArray_back(const void* const self);
void   bfVMArray_clear(void* const self);
size_t bfVMArray_find(const void* const self, const void* key, bfVMArrayFindCompare compare);
void   bfVMArray_push(struct BifrostVM_t* vm, void* const self, const void* const data);
void   bfVMArray_delete(struct BifrostVM_t* vm, void* const self);

/* string */

BifrostString bfVMString_new(struct BifrostVM_t* vm, const char* initial_data);
BifrostString bfVMString_newLen(struct BifrostVM_t* vm, const char* initial_data, size_t string_length);
const char*   bfVMString_cstr(ConstBifrostString self);
size_t        bfVMString_length(ConstBifrostString self);
void          bfVMString_reserve(struct BifrostVM_t* vm, BifrostString* self, size_t new_capacity);
void          bfVMString_sprintf(struct BifrostVM_t* vm, BifrostString* self, const char* format, ...);
void          bfVMString_unescape(BifrostString self);
int           bfVMString_cmp(ConstBifrostString self, ConstBifrostString other);
int           bfVMString_ccmpn(ConstBifrostString self, const char* other, size_t length);
uint32_t      bfVMString_hash(const char* str);
uint32_t      bfVMString_hashN(const char* str, size_t length);
void          bfVMString_delete(struct BifrostVM_t* vm, BifrostString self);

/* hash-map */

#if __cplusplus
}
#endif

#endif /* BIFROST_VM_OBJ_H */