#ifndef BIFROST_VM_FUNCTION_BUILDER_H
#define BIFROST_VM_FUNCTION_BUILDER_H

#include "bifrost_vm_instruction_op.h"
#include "bifrost_vm_value.h"

#if __cplusplus
extern "C" {
#endif
struct BifrostObjFn_t;
struct BifrostLexer_t;

typedef struct
{
  const char* name;     /*!< Stored in the source so no need to dynamically alloc */
  size_t      name_len; /*!< For Quicker Comparisons                              */

} BifrostVMLocalVar;

typedef int bfScopeVarCount;

typedef struct BifrostVMFunctionBuilder_t
{
  const char*            name;     /*!< Stored in the source so no need to dynamically alloc */
  size_t                 name_len; /*!< Length of [BifrostVMFunctionBuilder::name] */
  bfVMValue*             constants;
  BifrostVMLocalVar*     local_vars;
  bfScopeVarCount*       local_var_scope_size;
  uint32_t*              instructions;
  uint16_t*              line_to_code;
  size_t                 max_local_idx;
  struct BifrostLexer_t* lexer;

} BifrostVMFunctionBuilder;

void     bfVMFunctionBuilder_ctor(BifrostVMFunctionBuilder* self);
void     bfVMFunctionBuilder_begin(BifrostVMFunctionBuilder* self, const char* name, size_t length);
uint32_t bfVMFunctionBuilder_addConstant(BifrostVMFunctionBuilder* self, bfVMValue value);
void     bfVMFunctionBuilder_pushScope(BifrostVMFunctionBuilder* self);
uint32_t bfVMFunctionBuilder_declVariable(BifrostVMFunctionBuilder* self, const char* name, size_t length);
uint16_t bfVMFunctionBuilder_pushTemp(BifrostVMFunctionBuilder* self, uint16_t num_temps);
void     bfVMFunctionBuilder_popTemp(BifrostVMFunctionBuilder* self, uint16_t start);
size_t   bfVMFunctionBuilder_getVariable(BifrostVMFunctionBuilder* self, const char* name, size_t length);
void     bfVMFunctionBuilder_popScope(BifrostVMFunctionBuilder* self);
void     bfVMFunctionBuilder_addInstABC(BifrostVMFunctionBuilder* self, bfInstructionOp op, uint16_t a, uint16_t b, uint16_t c);
void     bfVMFunctionBuilder_addInstABx(BifrostVMFunctionBuilder* self, bfInstructionOp op, uint16_t a, uint32_t bx);
void     bfVMFunctionBuilder_addInstAsBx(BifrostVMFunctionBuilder* self, bfInstructionOp op, uint16_t a, int32_t sbx);
void     bfVMFunctionBuilder_addInstBreak(BifrostVMFunctionBuilder* self);
void     bfVMFunctionBuilder_addInstOp(BifrostVMFunctionBuilder* self, bfInstructionOp op);
void     bfVMFunctionBuilder_disassemble(BifrostVMFunctionBuilder* _);
void     bfVMFunctionBuilder_end(BifrostVMFunctionBuilder* self, struct BifrostObjFn_t* out, int arity);
void     bfVMFunctionBuilder_dtor(BifrostVMFunctionBuilder* self);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_FUNCTION_BUILDER_H */
