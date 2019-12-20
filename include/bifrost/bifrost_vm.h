/******************************************************************************/
/*!
 * @file   bifrost_vm.h
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
#ifndef BIFROST_VM_API_H
#define BIFROST_VM_API_H

#include "bifrost/data_structures/bifrost_dynamic_string.h" /* BifrostString                  */
#include "bifrost/data_structures/bifrost_hash_map.h"       /* BifrostHashMap                 */
#include "bifrost_std.h"                                    /* bfStringRange, int32_t, size_t */

#if __cplusplus
extern "C" {
#endif
struct BifrostParser_t;
struct BifrostVMStackFrame_t;
struct BifrostObj_t;
struct BifrostObjInstance_t;
struct BifrostObjModule_t;
struct BifrostObjNativeFn_t;
struct BifrostVM_t;
typedef struct BifrostVM_t BifrostVM;
struct bfValueHandle_t;
typedef struct bfValueHandle_t* bfValueHandle;

typedef uint64_t  bfVMValue;   /*!< The Nan-Tagged value representation of this scripting language. */
typedef bfFloat64 bfVMNumberT; /*!< Required to be a double from this Nan-Tagging Trick.            */

typedef void (*bfNativeFnT)(BifrostVM* vm, int32_t num_args);
typedef void (*bfClassFinalizer)(BifrostVM* vm, void* instance);

typedef enum
{
  BIFROST_VM_ERROR_NONE,                    /*!< NONE       */
  BIFROST_VM_ERROR_OUT_OF_MEMORY,           /*!< ANYONE     */
  BIFROST_VM_ERROR_RUNTIME,                 /*!< VM Runtime */
  BIFROST_VM_ERROR_LEXER,                   /*!< Lexer      */
  BIFROST_VM_ERROR_COMPILE,                 /*!< Parser     */
  BIFROST_VM_ERROR_FUNCTION_ARITY_MISMATCH, /*!< VM         */
  BIFROST_VM_ERROR_MODULE_ALREADY_DEFINED,  /*!< VM         */
  BIFROST_VM_ERROR_MODULE_NOT_FOUND,        /*!< VM         */
  BIFROST_VM_ERROR_INVALID_OP_ON_TYPE,      /*!< VM         */
  BIFROST_VM_ERROR_INVALID_ARGUMENT,        /*!< VM         */
  BIFROST_VM_ERROR_STACK_TRACE_BEGIN,       /*!< VM Runtime */
  BIFROST_VM_ERROR_STACK_TRACE,             /*!< VM Runtime */
  BIFROST_VM_ERROR_STACK_TRACE_END,         /*!< VM Runtime */

} BifrostVMError;

typedef enum
{
  BIFROST_VM_STD_MODULE_IO          = (1 << 0),
  BIFROST_VM_STD_MODULE_MEMORY      = (1 << 1),
  BIFROST_VM_STD_MODULE_FUNCTIONAL  = (1 << 2),
  BIFROST_VM_STD_MODULE_COLLECTIONS = (1 << 3),
  BIFROST_VM_STD_MODULE_ALL         = 0xFFFFFFFF,

} BifrostVMStandardModule;

typedef enum
{
  BIFROST_VM_STRING,
  BIFROST_VM_NUMBER,
  BIFROST_VM_BOOL,
  BIFROST_VM_NIL,
  BIFROST_VM_OBJECT,
  BIFROST_VM_FUNCTION,
  BIFROST_VM_MODULE,
  BIFROST_VM_UNDEFINED,

} BifrostVMType;

typedef struct
{
  const char* name;
  bfNativeFnT fn;
  int32_t     arity;
  uint32_t    num_statics;

} BifrostMethodBind;

typedef struct
{
  const char*              name;
  size_t                   extra_data_size;
  const BifrostMethodBind* methods;
  bfClassFinalizer         finalizer;

} BifrostVMClassBind;

/*!
 * @brief
 *  If the source is NULL then it is assumed the module could not be found.
 *  and an appropriate error will be issued.
 */
typedef struct
{
  const char* source;     /*!< Must have been allocated by the same allocator as the vm's one. (BifrostVMParams::memory_fn) */
  size_t      source_len; /*!< The number of bytes to used by [BifrostVMModuleLookUp::source]                               */

} BifrostVMModuleLookUp;

typedef void (*bfErrorFn)(BifrostVM* vm, BifrostVMError err, int line_no, const char* message);
typedef void (*bfPrintFn)(BifrostVM* vm, const char* message);

/*!
 * @brief
 *   The [BifrostVMModuleLookUp::source] field must be allocated from the same allocator
 *   that was passed in as [BifrostVMParams::memory_fn].
 */
typedef void (*bfModuleFn)(BifrostVM* vm, const char* from, const char* module, BifrostVMModuleLookUp* out);

/*!
 * @brief
 *   If ptr is NULL : Act as Malloc.
 *   If size == 0u  : Act as Free.
 *   Otherwise      : Act as Realloc.
 */
typedef void* (*bfMemoryFn)(void* user_data, void* ptr, size_t old_size, size_t new_size, size_t alignment);

typedef struct
{
  bfErrorFn  error_fn;
  bfPrintFn  print_fn;
  bfModuleFn module_fn;
  bfMemoryFn memory_fn;
  size_t     min_heap_size;
  size_t     heap_size;
  float      heap_growth_factor;
  void*      user_data;

} BifrostVMParams;

void bfVMParams_init(BifrostVMParams* self);

typedef enum
{
  BIFROST_VM_SYMBOL_CTOR,
  BIFROST_VM_SYMBOL_DTOR,
  BIFROST_VM_SYMBOL_CALL,
  BIFROST_VM_SYMBOL_MAX,

} BifrostVMBuildInSymbol;

/*!
 * @brief
 *   The self contained virtual machine for the Bifrost
 *   scripting language.
 *
 *   Consider all these member variables private. They
 *   are exposed so that you may declare a VM on the stack.
 *
 *   If you want ABI compatibility use 'bfVM_new' and 'bfVM_delete'
 *   and do not use this struct directly.
 */
struct BifrostVM_t
{
  struct BifrostVMStackFrame_t*      frames;                                  /*!< The call stack.                                                */
  bfVMValue*                         stack;                                   /*!< The base pointer to the stack memory.                          */
  bfVMValue*                         stack_top;                               /*!< The usable top of the [BifrostVM::stack].                      */
  BifrostString*                     symbols;                                 /*!< Every symbol ever used in the vm, a 'perfect hash'             */
  BifrostVMParams                    params;                                  /*!< The user defined parameters used by the VM                     */
  struct BifrostObj_t*               gc_object_list;                          /*!< The list of every object allocated by this VM.                 */
  BifrostHashMap                     modules;                                 /*!< <BifrostObjStr, BifrostObjModule*> for fast module lookup      */
  struct BifrostParser_t*            parser_stack;                            /*!< For handling the recursive nature of importing modules.        */
  bfValueHandle                      handles;                                 /*!< Additional GC Roots for Extended C Lifetimes                   */
  bfValueHandle                      free_handles;                            /*!< A pool of handles for reduced allocations.                     */
  BifrostString                      last_error;                              /*!< The last error to happen in a user readable way                */
  size_t                             bytes_allocated;                         /*!< The total amount of memory this VM has asked for               */
  struct BifrostObj_t*               finalized;                               /*!< Objects that have finalized but still need to be freed         */
  struct BifrostObj_t*               temp_roots[8];                           /*!< Objects temporarily protected from the GC                      */
  uint8_t                            temp_roots_top;                          /*!< BifrostVM_t::temp_roots size                                   */
  bfBool32                           gc_is_running;                           /*!< This is so that when calling finalizers the GC isn't run.      */
  size_t                             build_in_symbols[BIFROST_VM_SYMBOL_MAX]; /*!< Symbols that should be loaded at startup for a faster runtime. */
  const struct BifrostObjNativeFn_t* current_native_fn;
};

BifrostVM*         bfVM_new(const BifrostVMParams* params);
void               bfVM_ctor(BifrostVM* self, const BifrostVMParams* params);
BifrostVMError     bfVM_moduleMake(BifrostVM* self, size_t idx, const char* module);
void               bfVM_moduleLoadStd(BifrostVM* self, size_t idx, uint32_t module_flags);
BifrostVMError     bfVM_moduleLoad(BifrostVM* self, size_t idx, const char* module);
void               bfVM_moduleUnload(BifrostVM* self, const char* module);
void               bfVM_moduleUnloadAll(BifrostVM* self);
size_t             bfVM_stackSize(const BifrostVM* self);  // The usable size from an API standpoint
BifrostVMError     bfVM_stackResize(BifrostVM* self, size_t size);
void               bfVM_stackMakeInstance(BifrostVM* self, size_t clz_idx, size_t dst_idx);
void*              bfVM_stackMakeReference(BifrostVM* self, size_t idx, size_t extra_data_size);
void*              bfVM_stackMakeReferenceClz(BifrostVM* self, size_t module_idx, const BifrostVMClassBind* clz_bind, size_t dst_idx);
void               bfVM_stackMakeWeakRef(BifrostVM* self, size_t idx, void* value);
void               bfVM_referenceSetClass(BifrostVM* self, size_t idx, size_t clz_idx);
void               bfVM_classSetBaseClass(BifrostVM* self, size_t idx, size_t clz_idx);
void               bfVM_stackLoadVariable(BifrostVM* self, size_t dst_idx, size_t inst_or_class_or_module, const char* variable);
BifrostVMError     bfVM_stackStoreVariable(BifrostVM* self, size_t inst_or_class_or_module, const char* field, size_t value_idx);
BifrostVMError     bfVM_stackStoreNativeFn(BifrostVM* self, size_t inst_or_class_or_module, const char* field, bfNativeFnT func, int32_t arity);
BifrostVMError     bfVM_stackStoreClosure(BifrostVM* self, size_t inst_or_class_or_module, const char* field, bfNativeFnT func, int32_t arity, uint32_t num_statics);
BifrostVMError     bfVM_closureGetStatic(BifrostVM* self, size_t dst_idx, size_t static_idx);  // Must ony be called in the current closure.
BifrostVMError     bfVM_closureSetStatic(BifrostVM* self, size_t closure_idx, size_t static_idx, size_t value_idx);
BifrostVMError     bfVM_stackStoreClass(BifrostVM* self, size_t inst_or_class_or_module, const BifrostVMClassBind* clz_bind);
void               bfVM_stackSetString(BifrostVM* self, size_t idx, const char* value);
void               bfVM_stackSetStringLen(BifrostVM* self, size_t idx, const char* value, size_t len);
void               bfVM_stackSetNumber(BifrostVM* self, size_t idx, bfVMNumberT value);
void               bfVM_stackSetBool(BifrostVM* self, size_t idx, bfBool32 value);
void               bfVM_stackSetNil(BifrostVM* self, size_t idx);
void*              bfVM_stackReadInstance(const BifrostVM* self, size_t idx);  // Also works on null values, just returns NULL
const char*        bfVM_stackReadString(const BifrostVM* self, size_t idx, size_t* out_size);
bfVMNumberT        bfVM_stackReadNumber(const BifrostVM* self, size_t idx);
bfBool32           bfVM_stackReadBool(const BifrostVM* self, size_t idx);
BifrostVMType      bfVM_stackGetType(BifrostVM* self, size_t idx);
int32_t            bfVM_stackGetArity(const BifrostVM* self, size_t idx);
bfValueHandle      bfVM_stackMakeHandle(BifrostVM* self, size_t idx);
void               bfVM_stackLoadHandle(BifrostVM* self, size_t dst_idx, bfValueHandle handle);
void               bfVM_stackDestroyHandle(BifrostVM* self, bfValueHandle handle);  // Freeing a null handle is safe.
int32_t            bfVM_handleGetArity(bfValueHandle handle);
BifrostVMType      bfVM_handleGetType(bfValueHandle handle);
BifrostVMError     bfVM_call(BifrostVM* self, size_t idx, size_t args_start, int32_t num_args);
BifrostVMError     bfVM_execInModule(BifrostVM* self, const char* module, const char* source, size_t source_length);  // if 'module' == NULL we will exec in an anon module.
void               bfVM_gc(BifrostVM* self);
const char*        bfVM_buildInSymbolStr(const BifrostVM* self, BifrostVMBuildInSymbol symbol);
ConstBifrostString bfVM_errorString(const BifrostVM* self);
void               bfVM_dtor(BifrostVM* self);
void               bfVM_delete(BifrostVM* self);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_API_H */