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
#include <bifrost/data_structures/bifrost_array_t.h>
#include <stdio.h>

#define bfAssert(cond, msg) assert((cond) && (msg))

#define DEFAULT_ARRAY_SIZE 5

void bfVMFunctionBuilder_ctor(BifrostVMFunctionBuilder* self)
{
  self->name                 = NULL;
  self->name_len             = 0;
  self->constants            = NULL;
  self->local_vars           = Array_new(BifrostVMLocalVar, DEFAULT_ARRAY_SIZE);
  self->local_var_scope_size = Array_new(int, DEFAULT_ARRAY_SIZE);
  self->instructions         = NULL;
  self->line_to_code         = NULL;
  self->max_local_idx        = 0;
  self->lexer                = NULL;
}

void bfVMFunctionBuilder_begin(BifrostVMFunctionBuilder* self, const char* name, size_t length)
{
  bfAssert(self->constants == NULL, "This builder has already began.");

  self->name      = name;
  self->name_len  = length;
  self->constants = Array_new(bfVMValue, DEFAULT_ARRAY_SIZE);
  Array_clear(&self->local_vars);
  Array_clear(&self->local_var_scope_size);
  self->instructions = Array_new(uint32_t, DEFAULT_ARRAY_SIZE);
  self->line_to_code = OLD_bfArray_newA(self->line_to_code, DEFAULT_ARRAY_SIZE);

  bfVMFunctionBuilder_pushScope(self);
}

uint32_t bfVMFunctionBuilder_addConstant(BifrostVMFunctionBuilder* self, bfVMValue value)
{
  size_t index = Array_find(&self->constants, &value, NULL);

  if (index == BIFROST_ARRAY_INVALID_INDEX)
  {
    index = Array_size(&self->constants);
    Array_push(&self->constants, &value);
  }

  return (uint32_t)index;
}

void bfVMFunctionBuilder_pushScope(BifrostVMFunctionBuilder* self)
{
  bfScopeVarCount* count = Array_emplace(&self->local_var_scope_size);
  *count                 = 0;
}

static inline size_t bfVMFunctionBuilder__getVariable(BifrostVMFunctionBuilder* self, const char* name, size_t length, bfBool32 in_current_scope)
{
  const int* count = (const int*)Array_back(&self->local_var_scope_size);
  const int  end   = in_current_scope ? *count : (int)Array_size(&self->local_vars);

  for (int i = end - 1; i >= 0; --i)
  {
    BifrostVMLocalVar* var = self->local_vars + i;

    assert((int)Array_size(&self->local_vars) > i && "This is a rt error");

    if (length == var->name_len && String_ccmpn(name, var->name, length) == 0)
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

  const size_t       var_loc = Array_size(&self->local_vars);
  BifrostVMLocalVar* var     = Array_emplace(&self->local_vars);
  var->name                  = name;
  var->name_len              = length;

  int* count = (int*)Array_back(&self->local_var_scope_size);
  ++(*count);

  if (self->max_local_idx < var_loc)
  {
    self->max_local_idx = var_loc;
  }

  return (uint32_t)var_loc;
}

uint16_t bfVMFunctionBuilder_pushTemp(BifrostVMFunctionBuilder* self, uint16_t num_temps)
{
  const size_t       var_loc     = Array_size(&self->local_vars);
  const size_t       var_loc_end = var_loc + num_temps;
  BifrostVMLocalVar* vars        = Array_emplaceN(&self->local_vars, num_temps);

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
  Array_resize(&self->local_vars, start);
}

size_t bfVMFunctionBuilder_getVariable(BifrostVMFunctionBuilder* self, const char* name, size_t length)
{
  return bfVMFunctionBuilder__getVariable(self, name, length, bfFalse);
}

void bfVMFunctionBuilder_popScope(BifrostVMFunctionBuilder* self)
{
  const int*   count    = (const int*)Array_back(&self->local_var_scope_size);
  const size_t num_vars = Array_size(&self->local_vars);
  const size_t new_size = num_vars - *count;
  Array_resize(&self->local_vars, new_size);
  Array_pop(&self->local_var_scope_size);
}

static inline bfInstruction* bfVMFunctionBuilder_addInst(BifrostVMFunctionBuilder* self)
{
  *(uint16_t*)Array_emplace(&self->line_to_code) = (uint16_t)self->lexer->current_line_no;
  return Array_emplace(&self->instructions);
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
  bfInstruction* inst = Array_emplace(&self->instructions);
  *inst               = BIFROST_MAKE_INST_OP(op);
}

void bfVMFunctionBuilder_disassemble(BifrostVMFunctionBuilder* _)
{
  assert(!"Reimplement this function in the general debug header.");
  /*
  printf("\nDisassemble: [%s]: \n", self->name);
  printf("Num slots: %zu - ARITY\n", self->max_local_idx + 1);

  const size_t num_vars = Array_size(&self->local_vars);
  printf("Variables: (%zu)\n", num_vars);

  for (size_t i = 0; i < num_vars; ++i)
  {
    const BifrostVMLocalVar* var = self->local_vars + i;

    if (var->name_len)
    {
      printf("  [%zu] %.*s @ len(%zu)\n", i, (int)var->name_len, var->name, var->name_len);
    }
  }

  const size_t num_k = Array_size(&self->constants);
  printf("Konstants: (%zu)\n", num_k);

  bfInstDissasemble(self->instructions, Array_size(&self->instructions), 1);
  printf("\n");
  */
}

void bfVMFunctionBuilder_end(BifrostVMFunctionBuilder* self, struct BifrostObjFn_t* out, int arity)
{
  bfVMFunctionBuilder_popScope(self);

  out->super.type         = BIFROST_VM_OBJ_FUNCTION;
  out->name               = String_newLen(self->name, self->name_len);
  out->arity              = arity;
  out->line_to_code       = NULL;
  out->constants          = self->constants;
  out->instructions       = self->instructions;
  out->line_to_code       = self->line_to_code;
  out->needed_stack_space = self->max_local_idx + arity + 1;

  self->constants = NULL;
}

void bfVMFunctionBuilder_dtor(BifrostVMFunctionBuilder* self)
{
  Array_delete(&self->local_vars);
  Array_delete(&self->local_var_scope_size);
}
