#include "bifrost_vm_debug.h"

#include "bifrost_vm_obj.h"  //
#include <stdio.h>  //sprintf

static inline void bfDbgIndentPrint(int indent);

size_t bfDbgValueToString(bfVMValue value, char* buffer, size_t buffer_size)
{
  if (IS_NUMBER(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "%g", bfVmValue_asNumber(value));
  }

  if (IS_BOOL(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "%s", value == VAL_TRUE ? "true" : "false");
  }

  if (IS_NULL(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "null");
  }

  if (IS_POINTER(value))
  {
    const BifrostObj* const obj = AS_POINTER(value);

    switch (obj->type)
    {
      case BIFROST_VM_OBJ_FUNCTION:
      {
        const BifrostObjFn* const obj_fn = (const BifrostObjFn*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<fn %s>", obj_fn->name);
        break;
      }
      case BIFROST_VM_OBJ_MODULE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<module>");
        break;
      }
      case BIFROST_VM_OBJ_CLASS:
      {
        const BifrostObjClass* const obj_clz = (const BifrostObjClass*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<class %s>", obj_clz->name);
        break;
      }
      case BIFROST_VM_OBJ_INSTANCE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<instance>");
        break;
      }
      case BIFROST_VM_OBJ_STRING:
      {
        const BifrostObjStr* const obj_string = (const BifrostObjStr*)obj;

        return (size_t)snprintf(buffer, buffer_size, "%s", obj_string->value);
        break;
      }
      case BIFROST_VM_OBJ_NATIVE_FN:
      {
        return (size_t)snprintf(buffer, buffer_size, "<native function>");
        break;
      }
    }
  }

  return 0u;
}

size_t bfDbgValueTypeToString(bfVMValue value, char* buffer, size_t buffer_size)
{
  if (IS_NUMBER(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "<Number>");
  }

  if (IS_BOOL(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "<Boolean>");
  }

  if (IS_NULL(value))
  {
    return (size_t)snprintf(buffer, buffer_size, "<Nil>");
  }

  if (IS_POINTER(value))
  {
    const BifrostObj* const obj = AS_POINTER(value);

    switch (obj->type)
    {
      case BIFROST_VM_OBJ_FUNCTION:
      {
        const BifrostObjFn* const obj_fn = (const BifrostObjFn*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<fn %s>", obj_fn->name);
        break;
      }
      case BIFROST_VM_OBJ_MODULE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Module>");
        break;
      }
      case BIFROST_VM_OBJ_CLASS:
      {
        const BifrostObjClass* const obj_clz = (const BifrostObjClass*)obj;

        return (size_t)snprintf(buffer, buffer_size, "<Class %s>", obj_clz->name);
        break;
      }
      case BIFROST_VM_OBJ_INSTANCE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Instance>");
        break;
      }
      case BIFROST_VM_OBJ_STRING:
      {
        return (size_t)snprintf(buffer, buffer_size, "<String>");
        break;
      }
      case BIFROST_VM_OBJ_NATIVE_FN:
      {
        return (size_t)snprintf(buffer, buffer_size, "<NativeFunction>");
        break;
      }
      case BIFROST_VM_OBJ_REFERENCE:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Reference>");
        break;
      }
      case BIFROST_VM_OBJ_WEAK_REF:
      {
        return (size_t)snprintf(buffer, buffer_size, "<Weak Ref>");
        break;
      }
    }
  }

  return (size_t)snprintf(buffer, buffer_size, "<Undefined>");
}

void bfDbgDisassembleInstructions(int indent, const bfInstruction* code, size_t code_length)
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

    printf("| 0x%08X ", code[i]);
    printf("| %15s ", bfInstOpToString(op));
    printf("|a: %3u| b: %3u| c: %3u| bx: %7u| sbx: %+7i|\n", (uint32_t)regs[0], (uint32_t)regs[1], (uint32_t)regs[2], regs[3], rsbx);
  }

  bfDbgIndentPrint(indent);
  printf("----------------------------------------------------------------------------------\n");
}

static inline void bfDbgIndentPrint(int indent)
{
  for (int j = 0; j < indent; ++j)
  {
    printf("  ");
  }
}