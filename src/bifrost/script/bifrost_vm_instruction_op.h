/******************************************************************************/
/*!
 * @file   bifrost_vm_instruction_op.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @par
 *    Bifrost Scripting Language
 *
 * @brief
 *
 *
 * @version 0.0.1-beta
 * @date    2019-07-01
 *
 * @copyright Copyright (c) 2019 Shareef Raheem
 */
/******************************************************************************/
#ifndef BIFROST_VM_INSTRUCTION_OP_H
#define BIFROST_VM_INSTRUCTION_OP_H

#include <stdint.h> /* uint32_t */

#if __cplusplus
extern "C" {
#endif
// Total of 28.5 / 31 possible ops.
typedef enum
{
  // To be replaced with return. / Fake OPs
  BIFROST_VM_OP_PRINT_LOCAL,  // print(local[rBx])

  /* Load OPs */
  BIFROST_VM_OP_LOAD_CONSTANT,    // rA = K[rBx]
  BIFROST_VM_OP_LOAD_SYMBOL,      // rA = rB.SYMBOLS[rC]
  BIFROST_VM_OP_LOAD_MODULE_VAR,  // rA = module.SYMBOLS[rBx]

  // TODO(Shareef): If I have space for this optimization OP that would be great.
  // BIFROST_VM_OP_LOAD_BASIC, // rA = (rBx == 1 : VAL_TRUE) || (rBx == 2 : VAL_FALSE) || (rBx == 3 : VAL_NULL)

  /* Store OPs */
  BIFROST_VM_OP_STORE_MOVE,        // rA     = rBx
  BIFROST_VM_OP_STORE_SYMBOL,      // rA.SYMBOLS[rB]  = rC

  // TODO(SR): This can be removed in favor of exclusively static variables for Module state.
  BIFROST_VM_OP_STORE_MODULE_VAR,  // module.SYMBOLS[rA] = rBx

  /* System OPs */
  BIFROST_VM_OP_NEW_CLZ,  // rA = new local[rBx];

  // Math OPs
  BIFROST_VM_OP_MATH_ADD,  // rA = rB + rC
  BIFROST_VM_OP_MATH_SUB,  // rA = rB - rC
  BIFROST_VM_OP_MATH_MUL,  // rA = rB * rC
  BIFROST_VM_OP_MATH_DIV,  // rA = rB / rC
  BIFROST_VM_OP_MATH_MOD,  // rA = rB % rC
  BIFROST_VM_OP_MATH_POW,  // rA = rB ^ rC
  BIFROST_VM_OP_MATH_INV,  // rA = -rB

  // TODO(Shareef): No integer div, guess you have to do:
  //   rA = Math.floor(rB / rC);

  // Additional Logical Ops
  // BIFROST_VM_OP_LOGIC_OR,   // rA = rB | rC
  // BIFROST_VM_OP_LOGIC_AND,  // rA = rB & rC
  // BIFROST_VM_OP_LOGIC_XOR,  // rA = rB ^ rC
  // BIFROST_VM_OP_LOGIC_NOT,  // rA = ~rB
  // BIFROST_VM_OP_LOGIC_LS,   // rA = (rB << rC)
  // BIFROST_VM_OP_LOGIC_RS,   // rA = (rB >> rC)

  // Comparisons
  BIFROST_VM_OP_CMP_EE,   // rA = rB == rC
  BIFROST_VM_OP_CMP_NE,   // rA = rB != rC
  BIFROST_VM_OP_CMP_LT,   // rA = rB <  rC
  BIFROST_VM_OP_CMP_LE,   // rA = rB <= rC
  BIFROST_VM_OP_CMP_GT,   // rA = rB >  rC
  BIFROST_VM_OP_CMP_GE,   // rA = rB >= rC
  BIFROST_VM_OP_CMP_AND,  // rA = rB && rC
  BIFROST_VM_OP_CMP_OR,   // rA = rB || rC
  BIFROST_VM_OP_NOT,      // rA = !rBx
  // Control Flow
  BIFROST_VM_OP_CALL_FN,      // call(local[rB]) (params-start = rA, num-args = rC)

  // TODO(SR): If I need another OP JUMP can be a JUMP_IF with a hardcoded true.
  BIFROST_VM_OP_JUMP,         // ip += rsBx
  BIFROST_VM_OP_JUMP_IF,      // if (rA) ip += rsBx
  BIFROST_VM_OP_JUMP_IF_NOT,  // if (!rA) ip += rsBx
  BIFROST_VM_OP_RETURN,       // breaks out of current function.

} bfInstructionOp;

/*!
   ///////////////////////////////////////////
   // 0     5         14        23       32 //
   // [ooooo|aaaaaaaaa|bbbbbbbbb|ccccccccc] //
   // [ooooo|aaaaaaaaa|bxbxbxbxbxbxbxbxbxb] //
   // [ooooo|aaaaaaaaa|sBxbxbxbxbxbxbxbxbx] //
   // opcode = 0       - 31                 //
   // rA     = 0       - 511                //
   // rB     = 0       - 511                //
   // rBx    = 0       - 262143             //
   // rsBx   = -131071 - 131072             //
   // rC     = 0       - 511                //
   ///////////////////////////////////////////
 */
typedef uint32_t bfInstruction;

#define BIFROST_INST_OP_MASK (bfInstruction)0x1F /* 31 in hex  */
#define BIFROST_INST_OP_OFFSET (bfInstruction)0
#define BIFROST_INST_RA_MASK (bfInstruction)0x1FF /* 511 in hex */
#define BIFROST_INST_RA_OFFSET (bfInstruction)5
#define BIFROST_INST_RB_MASK (bfInstruction)0x1FF /* 511 in hex */
#define BIFROST_INST_RB_OFFSET (bfInstruction)14
#define BIFROST_INST_RC_MASK (bfInstruction)0x1FF /* 511 in hex */
#define BIFROST_INST_RC_OFFSET (bfInstruction)23
#define BIFROST_INST_RBx_MASK (bfInstruction)0x3FFFF
#define BIFROST_INST_RBx_OFFSET (bfInstruction)14
#define BIFROST_INST_RsBx_MASK (bfInstruction)0x3FFFF
#define BIFROST_INST_RsBx_OFFSET (bfInstruction)14
#define BIFROST_INST_RsBx_MAX ((bfInstruction)BIFROST_INST_RsBx_MASK / 2)

#define BIFROST_INST_INVALID 0xFFFFFFFF

#define BIFROST_MAKE_INST_OP(op) \
  (bfInstruction)(op & BIFROST_INST_OP_MASK)

#define BIFROST_MAKE_INST_RC(c) \
  ((c & BIFROST_INST_RC_MASK) << BIFROST_INST_RC_OFFSET)

#define BIFROST_MAKE_INST_OP_ABC(op, a, b, c)               \
  BIFROST_MAKE_INST_OP(op) |                                \
   ((a & BIFROST_INST_RA_MASK) << BIFROST_INST_RA_OFFSET) | \
   ((b & BIFROST_INST_RB_MASK) << BIFROST_INST_RB_OFFSET) | \
   BIFROST_MAKE_INST_RC((c))

#define BIFROST_MAKE_INST_OP_ABx(op, a, bx)                 \
  BIFROST_MAKE_INST_OP(op) |                                \
   ((a & BIFROST_INST_RA_MASK) << BIFROST_INST_RA_OFFSET) | \
   ((bx & BIFROST_INST_RBx_MASK) << BIFROST_INST_RBx_OFFSET)

#define BIFROST_MAKE_INST_OP_AsBx(op, a, bx)                \
  BIFROST_MAKE_INST_OP(op) |                                \
   ((a & BIFROST_INST_RA_MASK) << BIFROST_INST_RA_OFFSET) | \
   (((bx + BIFROST_INST_RsBx_MAX) & BIFROST_INST_RsBx_MASK) << BIFROST_INST_RsBx_OFFSET)

/* NOTE(Shareef):
    This macro helps change / patch pieces of an instruction.

    inst : bfInstruction*;

    x    : can be equal to (capitalization matters!):
      > RA
      > RB
      > RC
      > RBx
      > RsBx
      > OP

    val  : integer (range depends on 'x').
 */
#define bfInstPatchX(inst, x, val) \
  *(inst) = (*(inst) & ~(BIFROST_INST_##x##_MASK << BIFROST_INST_##x##_OFFSET)) | BIFROST_MAKE_INST_##x(val)

// TODO(SR): Move to the Debug Header
const char* bfInstOpToString(bfInstructionOp op);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_INSTRUCTION_OP_H */