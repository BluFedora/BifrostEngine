#ifndef BIFROST_VM_DEBUG_H
#define BIFROST_VM_DEBUG_H

#include "bifrost_vm_instruction_op.h" /* bfInstruction */
#include "bifrost_vm_value.h"          /* bfVMValue     */
#include <stddef.h>                    /* size_t        */

#if __cplusplus
extern "C" {
#endif
typedef enum
{
  // IEEE 754-1985 says we need 24 chars: "-2.2250738585072020E-308"
  BIFROST_DBG_DOUBLE_MAX_DIGITS = 1 + 15 + 1 + 1 + 1 + 4 + 1 + 1 /* Sign + Digits + Dot + e + "+/-" + exp + '\0' + Safety? */,

} BifrostDebugConstants;

size_t      bfDbgValueToString(bfVMValue value, char* buffer, size_t buffer_size);
size_t      bfDbgValueTypeToString(bfVMValue value, char* buffer, size_t buffer_size);
const char* bfInstOpToString(bfInstructionOp op);
void        bfDbgDisassembleInstructions(int indent, const bfInstruction* code, size_t code_length);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_DEBUG_H */
