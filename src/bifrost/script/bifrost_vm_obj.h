// An arity of -1 indicates infinite (read: 0-511) params

#ifndef BIFROST_VM_OBJ_H
#define BIFROST_VM_OBJ_H

#include "bifrost/bifrost_vm.h"                             /* bfNativeFnT, bfClassFinalizer, bfVMValue */
#include <bifrost/data_structures/bifrost_dynamic_string.h> /* BifrostString, ConstBifrostString        */
#include <bifrost/data_structures/bifrost_hash_map.h>       /* BifrostHashMap                           */

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

  // These next two types would be C/++ owned memory.
  // BIFROST_VM_OBJ_REFERENCE,  // 0b110
  // BIFROST_VM_OBJ_WEAK_REF,  // 0b111

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
  int32_t                    arity;
  uint16_t*                  line_to_code;
  bfVMValue*                 constants;
  uint32_t*                  instructions;
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
  BifrostObj        super;
  BifrostString     name;
  BifrostObjModule* module;
  BifrostVMSymbol*  symbols;
  BifrostVMSymbol*  field_initializers;
  size_t            extra_data;
  bfClassFinalizer  finalizer;

} BifrostObjClass;

typedef struct BifrostObjInstance_t
{
  BifrostObj       super;
  BifrostObjClass* clz;
  BifrostHashMap   fields;                   // <ConstBifrostString, bfVMValue> (/* Non owning string, [BifrostVM::symbols] is the owner. */)
  char             extra_data[bfStructHack]; /* This is for native class data. */

} BifrostObjInstance;

typedef struct BifrostObjStr_t
{
  BifrostObj    super;
  BifrostString value;
  size_t        hash;

} BifrostObjStr;

typedef struct BifrostObjNativeFn_t
{
  BifrostObj  super;
  bfNativeFnT value;
  int32_t     arity;

} BifrostObjNativeFn;

typedef struct BifrostVMStackFrame_t
{
  BifrostObjFn* fn;        /*!< Needed for addition debug info for stack traces. */
  uint32_t*     ip;        /*!< The current instruction being executed.          */
  size_t        old_stack; /*!< The top of the stack to restore to.              */
  size_t        stack;     /*!< The place where this stacks locals start.        */

} BifrostVMStackFrame;

#define BIFROST_AS_OBJ(value) ((BifrostObj*)AS_POINTER((value)))

// Move to "GC" Header.
BifrostObjModule*   bfVM_createModule(struct BifrostVM_t* self, bfStringRange name);
BifrostObjClass*    bfVM_createClass(struct BifrostVM_t* self, BifrostObjModule* module, bfStringRange name, size_t extra_data);
BifrostObjInstance* bfVM_createInstance(struct BifrostVM_t* self, BifrostObjClass* clz);
BifrostObjFn*       bfVM_createFunction(struct BifrostVM_t* self, BifrostObjModule* module);
BifrostObjNativeFn* bfVM_createNativeFn(struct BifrostVM_t* self, bfNativeFnT fn_ptr, int32_t arity);
BifrostObjStr*      bfVM_createString(struct BifrostVM_t* self, bfStringRange value);
void                bfVMObject_delete(struct BifrostVM_t* self, BifrostObj* obj);
bfBool32            bfObjIsFunction(const BifrostObj* obj);
void                bfObjFinalize(struct BifrostVM_t* self, BifrostObjInstance* inst);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_OBJ_H */