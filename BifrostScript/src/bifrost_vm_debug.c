/******************************************************************************/
/*!
 * @file   bifrost_vm_debug.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief  
 *   This contains extra functions for dumping out internal vm
 *   state into strings.
 * 
 * @version 0.0.1
 * @date    2020-02-16
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#include "bifrost_vm_debug.h"

#include "bifrost_vm_obj.h" /* */
#include <stdio.h>          /* sprintf */

static void bfDbgIndentPrint(int indent);

size_t bfDbgValueToString(bfVMValue value, char* buffer, size_t buffer_size)
{
  if (bfVMValue_isNumber(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "%g", bfVmValue_asNumber(value));
  }

  if (bfVMValue_isBool(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "%s", bfVMValue_isTrue(value) ? "true" : "false");
  }

  if (bfVMValue_isNull(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "null");
  }

  if (bfVMValue_isPointer(value))
  {
    const BifrostObj* const obj = bfVmValue_asPointer(value);

    switch (obj->type)
    {
      case BIFROST_VM_OBJ_FUNCTION:
      {
        const BifrostObjFn* const obj_fn = (const BifrostObjFn*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<fn %s>", obj_fn->name);
      }
      case BIFROST_VM_OBJ_MODULE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<module>");
      }
      case BIFROST_VM_OBJ_CLASS:
      {
        const BifrostObjClass* const obj_clz = (const BifrostObjClass*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<class %s>", obj_clz->name);
      }
      case BIFROST_VM_OBJ_INSTANCE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<instance>");
      }
      case BIFROST_VM_OBJ_STRING:
      {
        const BifrostObjStr* const obj_string = (const BifrostObjStr*)obj;

        return (size_t)snprintf(buffer, buffer_size, "%s", obj_string->value);
      }
      case BIFROST_VM_OBJ_NATIVE_FN:
      {
        return (size_t)snprintf(buffer, buffer_size, "<native function>");
      }
      case BIFROST_VM_OBJ_REFERENCE:
      {
        const BifrostObjReference* const obj_ref = (const BifrostObjReference*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<obj reference class(%s)>", obj_ref->clz ? obj_ref->clz->name : "null");
      }
      case BIFROST_VM_OBJ_WEAK_REF:
      {
        const BifrostObjWeakRef* const obj_weak_ref = (const BifrostObjWeakRef*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<obj weak ref %p>", obj_weak_ref->data);
      }
    }
  }

  return 0u;
}

size_t bfDbgValueTypeToString(bfVMValue value, char* buffer, size_t buffer_size)
{
  if (bfVMValue_isNumber(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "<Number>");
  }

  if (bfVMValue_isBool(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "<Boolean>");
  }

  if (bfVMValue_isNull(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "<Nil>");
  }

  if (bfVMValue_isPointer(value))
  {
    const BifrostObj* const obj = bfVmValue_asPointer(value);

    switch (obj->type)
    {
      case BIFROST_VM_OBJ_FUNCTION:
      {
        const BifrostObjFn* const obj_fn = (const BifrostObjFn*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<fn %s>", obj_fn->name);
      }
      case BIFROST_VM_OBJ_MODULE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Module>");
      }
      case BIFROST_VM_OBJ_CLASS:
      {
        const BifrostObjClass* const obj_clz = (const BifrostObjClass*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<Class %s>", obj_clz->name);
      }
      case BIFROST_VM_OBJ_INSTANCE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Instance>");
      }
      case BIFROST_VM_OBJ_STRING:
      {
        return (size_t)snprintf(buffer, buffer_size, "<String>");
      }
      case BIFROST_VM_OBJ_NATIVE_FN:
      {
        return (size_t)snprintf(buffer, buffer_size, "<NativeFunction>");
      }
      case BIFROST_VM_OBJ_REFERENCE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Reference>");
      }
      case BIFROST_VM_OBJ_WEAK_REF:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Weak Ref>");
      }
    }
  }

  return (size_t)snprintf(buffer, buffer_size, "<Undefined>");
}

const char* bfInstOpToString(bfInstructionOp op)
{
#define bfInstOpToStringImpl(n) \
  case BIFROST_VM_OP_##n: return #n

  switch (op)
  {
    bfInstOpToStringImpl(CMP_EE);
    bfInstOpToStringImpl(CMP_NE);
    bfInstOpToStringImpl(CMP_LE);
    bfInstOpToStringImpl(NOT);
    bfInstOpToStringImpl(LOAD_BASIC);
    bfInstOpToStringImpl(CMP_AND);
    bfInstOpToStringImpl(CMP_OR);
    bfInstOpToStringImpl(RETURN);
    bfInstOpToStringImpl(LOAD_SYMBOL);
    bfInstOpToStringImpl(STORE_SYMBOL);
    bfInstOpToStringImpl(NEW_CLZ);
    bfInstOpToStringImpl(STORE_MOVE);
    bfInstOpToStringImpl(CALL_FN);
    bfInstOpToStringImpl(MATH_ADD);
    bfInstOpToStringImpl(MATH_SUB);
    bfInstOpToStringImpl(MATH_MUL);
    bfInstOpToStringImpl(MATH_DIV);
    bfInstOpToStringImpl(MATH_MOD);
    bfInstOpToStringImpl(MATH_POW);
    bfInstOpToStringImpl(MATH_INV);
    bfInstOpToStringImpl(CMP_LT);
    bfInstOpToStringImpl(CMP_GT);
    bfInstOpToStringImpl(CMP_GE);
    bfInstOpToStringImpl(JUMP);
    bfInstOpToStringImpl(JUMP_IF);
    bfInstOpToStringImpl(JUMP_IF_NOT);
    default: return "OP_UNKNOWN";
  }
#undef bfInstOpToStringImpl
}

void bfDbgDisassembleInstructions(int indent, const bfInstruction* code, size_t code_length, uint16_t* code_to_line)
{
  bfDbgIndentPrint(indent);

  printf("----------------------------------------------------------------------------------\n");
  for (size_t i = 0; i < code_length; ++i)
  {
    uint8_t  op;
    uint32_t regs[4];
    int32_t  rsbx;
    bfVM_decode(code[i], &op, regs + 0, regs + 1, regs + 2, regs + 3, &rsbx);

    bfDbgIndentPrint(indent);

    if (code_to_line)
    {
      printf("Line[%3i]: ", (int)code_to_line[i]);
    }

    printf("| 0x%08X ", code[i]);
    printf("| %15s ", bfInstOpToString(op));
    printf("|a: %3u| b: %3u| c: %3u| bx: %7u| sbx: %+7i|\n", (uint32_t)regs[0], (uint32_t)regs[1], (uint32_t)regs[2], regs[3], rsbx);
  }

  bfDbgIndentPrint(indent);
  printf("----------------------------------------------------------------------------------\n");
}

void bfDbgDisassembleFunction(int indent, BifrostObjFn* function)
{
  const size_t num_constants      = Array_size(&function->constants);
  const size_t num_instructions   = Array_size(&function->instructions);
  const size_t needed_stack_space = function->needed_stack_space;
  char         temp_buffer[128];

  bfDbgIndentPrint(indent + 0);
  printf("Function(%s, arity = %i, stack_space = %i, module = '%s'):\n", function->name, function->arity, (int)needed_stack_space, function->module->name);
  
  bfDbgIndentPrint(indent + 1);
  printf("Constants(%i):\n", (int)num_constants);

  for (size_t i = 0; i < num_constants; ++i)
  {
    bfDbgValueToString(function->constants[i], temp_buffer, sizeof(temp_buffer));

    bfDbgIndentPrint(indent + 2);
    printf("[%i] = %s\n", (int)i, temp_buffer);
  }

  bfDbgIndentPrint(indent + 1);
  printf("Instructions(%i):\n", (int)num_instructions);
  bfDbgDisassembleInstructions(indent + 2, function->instructions, num_instructions, function->code_to_line);

  bfDbgIndentPrint(indent + 1);
}

static void bfDbgIndentPrint(int indent)
{
  for (int j = 0; j < indent; ++j)
  {
    printf("  ");
  }
}
