/******************************************************************************/
/*!
 * @file   bifrost_vm_debug.h
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
#ifndef BIFROST_VM_DEBUG_H
#define BIFROST_VM_DEBUG_H

#include "bifrost_vm_instruction_op.h" /* bfInstruction */
#include "bifrost_vm_value.h"          /* bfVMValue     */

#include <stddef.h> /* size_t */

#if __cplusplus
extern "C" {
#endif
typedef struct BifrostObjFn_t BifrostObjFn;

enum
{
  // IEEE 754-1985 says we need 24 chars: "-2.2250738585072020E-308"
  k_bfDbgDoubleMaxDigits = 1 + 15 + 1 + 1 + 1 + 4 + 1 + 1 /* Sign + Digits + Dot + e + "+/-" + exp + '\0' + Safety? */,
};

size_t      bfDbgValueToString(bfVMValue value, char* buffer, size_t buffer_size);
size_t      bfDbgValueTypeToString(bfVMValue value, char* buffer, size_t buffer_size);
const char* bfInstOpToString(bfInstructionOp op);
void        bfDbgDisassembleInstructions(int indent, const bfInstruction* code, size_t code_length, uint16_t* code_to_line);
void        bfDbgDisassembleFunction(int indent, BifrostObjFn* function);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_DEBUG_H */
