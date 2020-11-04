/******************************************************************************/
/*!
 * @file   bifrost_vm_function_builder.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   The parser uses this API to generate instructions.
 *   The output from this is an executable set of bytecode.
 * 
 * @version 0.0.1
 * @date    2020-02-16
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#include "bifrost_vm_function_builder.h"

#include "bifrost_vm_obj.h"

#include <assert.h>
#include <stdio.h>

#define bfAssert(cond, msg) assert((cond) && (msg))

#define k_DefaultArraySize 5

void bfVMFunctionBuilder_ctor(BifrostVMFunctionBuilder* self, struct BifrostLexer_t* lexer)
{
  self->name                 = NULL;
  self->name_len             = 0;
  self->constants            = NULL;
  self->local_vars           = bfVMArray_new(lexer->vm, BifrostVMLocalVar, k_DefaultArraySize);
  self->local_var_scope_size = bfVMArray_new(lexer->vm, int, k_DefaultArraySize);
  self->instructions         = NULL;
  self->code_to_line         = NULL;
  self->max_local_idx        = 0;
  self->lexer                = lexer;
}

void bfVMFunctionBuilder_begin(BifrostVMFunctionBuilder* self, const char* name, size_t length)
{
  bfAssert(self->constants == NULL, "This builder has already began.");

  self->name      = name;
  self->name_len  = length;
  self->constants = bfVMArray_new(self->lexer->vm, bfVMValue, k_DefaultArraySize);
  bfVMArray_clear(&self->local_vars);
  bfVMArray_clear(&self->local_var_scope_size);
  self->instructions = bfVMArray_new(self->lexer->vm, uint32_t, k_DefaultArraySize);
  self->code_to_line = bfVMArray_newA(self->lexer->vm, self->code_to_line, k_DefaultArraySize);

  bfVMFunctionBuilder_pushScope(self);
}

uint32_t bfVMFunctionBuilder_addConstant(BifrostVMFunctionBuilder* self, bfVMValue value)
{
  size_t index = bfVMArray_find(&self->constants, &value, NULL);

  if (index == BIFROST_ARRAY_INVALID_INDEX)
  {
    index = bfVMArray_size(&self->constants);
    bfVMArray_push(self->lexer->vm, &self->constants, &value);
  }

  return (uint32_t)index;
}

void bfVMFunctionBuilder_pushScope(BifrostVMFunctionBuilder* self)
{
  bfScopeVarCount* count = bfVMArray_emplace(self->lexer->vm, &self->local_var_scope_size);
  *count                 = 0;
}

static inline size_t bfVMFunctionBuilder__getVariable(BifrostVMFunctionBuilder* self, const char* name, size_t length, bfBool32 in_current_scope)
{
  const int* count = (const int*)bfVMArray_back(&self->local_var_scope_size);
  const int  end   = in_current_scope ? *count : (int)bfVMArray_size(&self->local_vars);

  for (int i = end - 1; i >= 0; --i)
  {
    BifrostVMLocalVar* var = self->local_vars + i;

    assert((int)bfVMArray_size(&self->local_vars) > i && "This is a rt error");

    if (length == var->name_len && bfVMString_ccmpn(name, var->name, length) == 0)
    {
      return i;
    }
  }

  return BIFROST_ARRAY_INVALID_INDEX;
}

uint32_t bfVMFunctionBuilder_declVariable(BifrostVMFunctionBuilder* self, const char* name, size_t length)
{
  const size_t prev_decl = bfVMFunctionBuilder__getVariable(self, name, length, bfTrue);

  if (prev_decl != BIFROST_ARRAY_INVALID_INDEX)
  {
    // TODO(Shareef):
    //   Is it the callers job to call bfVMFunctionBuilder_getVariable first?
    //   Or maybe I should add a helper???
    printf("ERROR: [%.*s] already declared.\n", (int)length, name);
    return (uint32_t)prev_decl;
  }

  const size_t       var_loc = bfVMArray_size(&self->local_vars);
  BifrostVMLocalVar* var     = bfVMArray_emplace(self->lexer->vm, &self->local_vars);
  var->name                  = name;
  var->name_len              = length;

  int* count = (int*)bfVMArray_back(&self->local_var_scope_size);
  ++(*count);

  if (self->max_local_idx < var_loc)
  {
    self->max_local_idx = var_loc;
  }

  return (uint32_t)var_loc;
}

uint16_t bfVMFunctionBuilder_pushTemp(BifrostVMFunctionBuilder* self, uint16_t num_temps)
{
  const size_t       var_loc     = bfVMArray_size(&self->local_vars);
  const size_t       var_loc_end = var_loc + num_temps;
  BifrostVMLocalVar* vars        = bfVMArray_emplaceN(self->lexer->vm, &self->local_vars, num_temps);

  for (size_t i = 0; i < num_temps; ++i)
  {
    BifrostVMLocalVar* var = vars + i;
    var->name              = "__TEMP__";
    var->name_len          = 0;
  }

  if (self->max_local_idx < var_loc_end)
  {
    self->max_local_idx = var_loc_end;
  }

  return (uint16_t)var_loc;
}

void bfVMFunctionBuilder_popTemp(BifrostVMFunctionBuilder* self, uint16_t start)
{
  bfVMArray_resize(self->lexer->vm, &self->local_vars, start);
}

size_t bfVMFunctionBuilder_getVariable(BifrostVMFunctionBuilder* self, const char* name, size_t length)
{
  return bfVMFunctionBuilder__getVariable(self, name, length, bfFalse);
}

void bfVMFunctionBuilder_popScope(BifrostVMFunctionBuilder* self)
{
  const int*   count    = (const int*)bfVMArray_back(&self->local_var_scope_size);
  const size_t num_vars = bfVMArray_size(&self->local_vars);
  const size_t new_size = num_vars - *count;

  bfVMArray_resize(self->lexer->vm, &self->local_vars, new_size);
  bfVMArray_pop(&self->local_var_scope_size);
}

static inline bfInstruction* bfVMFunctionBuilder_addInst(BifrostVMFunctionBuilder* self)
{
  *(uint16_t*)bfVMArray_emplace(self->lexer->vm, &self->code_to_line) = (uint16_t)self->lexer->current_line_no;
  return bfVMArray_emplace(self->lexer->vm, &self->instructions);
}

void bfVMFunctionBuilder_addInstABC(BifrostVMFunctionBuilder* self, bfInstructionOp op, uint16_t a, uint16_t b, uint16_t c)
{
  *bfVMFunctionBuilder_addInst(self) = BIFROST_MAKE_INST_OP_ABC(op, a, b, c);
}

void bfVMFunctionBuilder_addInstABx(BifrostVMFunctionBuilder* self, bfInstructionOp op, uint16_t a, uint32_t bx)
{
  *bfVMFunctionBuilder_addInst(self) = BIFROST_MAKE_INST_OP_ABx(op, a, bx);
}

void bfVMFunctionBuilder_addInstAsBx(BifrostVMFunctionBuilder* self, bfInstructionOp op, uint16_t a, int32_t sbx)
{
  *bfVMFunctionBuilder_addInst(self) = BIFROST_MAKE_INST_OP_AsBx(op, a, sbx);
}

void bfVMFunctionBuilder_addInstBreak(BifrostVMFunctionBuilder* self)
{
  *bfVMFunctionBuilder_addInst(self) = BIFROST_INST_INVALID;
}

void bfVMFunctionBuilder_addInstOp(BifrostVMFunctionBuilder* self, bfInstructionOp op)
{
  bfInstruction* inst = bfVMArray_emplace(self->lexer->vm, &self->instructions);
  *inst               = BIFROST_MAKE_INST_OP(op);
}

void bfVMFunctionBuilder_end(BifrostVMFunctionBuilder* self, struct BifrostObjFn_t* out, int arity)
{
  bfVMFunctionBuilder_addInstABx(self, BIFROST_VM_OP_RETURN, 0, 0);
  bfVMFunctionBuilder_popScope(self);

  out->super.type         = BIFROST_VM_OBJ_FUNCTION;
  out->name               = bfVMString_newLen(self->lexer->vm, self->name, self->name_len);
  out->arity              = arity;
  out->code_to_line       = self->code_to_line;
  out->constants          = self->constants;
  out->instructions       = self->instructions;
  out->needed_stack_space = self->max_local_idx + arity + 1;

  // Transfer of ownership of the constants to output function.
  self->constants = NULL;
}

void bfVMFunctionBuilder_dtor(BifrostVMFunctionBuilder* self)
{
  bfVMArray_delete(self->lexer->vm, &self->local_vars);
  bfVMArray_delete(self->lexer->vm, &self->local_var_scope_size);
}
