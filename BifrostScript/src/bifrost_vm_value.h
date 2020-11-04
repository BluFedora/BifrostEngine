/******************************************************************************/
/*!
 * @file   bifrost_vm_value.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   Helpers for the value representation for the vm.
 *   Uses Nan-Tagging for compact storage of objects.
 *   (Use of double is required for Nan Tagging)
 * 
 * @version 0.0.1
 * @date    2020-02-16
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BIFROST_VM_VALUE_H
#define BIFROST_VM_VALUE_H

#include <stdint.h> /* uint64_t */

#if __cplusplus
extern "C" {
#endif
typedef double bfFloat64;

/* clang-format off */
/* static uint8_t TAG_GET(bfVMValue value) { return (uint8_t)(value & TAG_MASK); } */

#define k_Float64SignBit     ((uint64_t)(1) << 63)
#define k_QuietNan           (uint64_t)(0x7FFC000000000000ULL)
#define k_VMValuePointerMask (k_Float64SignBit | k_QuietNan)
#define k_VMValueTagMask     (uint64_t)0x3
#define k_VMValueTagNan      (uint64_t)0x0
#define k_VMValueTagNull     (uint64_t)0x1
#define k_VMValueTagTrue     (uint64_t)0x2
#define k_VMValueTagFalse    (uint64_t)0x3 /* Tags Bits 4-7 non used */
#define bfDefineTagValue(t)  (bfVMValue)((uint64_t)(k_QuietNan | (t)))
#define k_VMValueNull        bfDefineTagValue(k_VMValueTagNull)
#define k_VMValueTrue        bfDefineTagValue(k_VMValueTagTrue)
#define k_VMValueFalse       bfDefineTagValue(k_VMValueTagFalse)
/* clang-format on */

bfBool32  bfVMValue_isNull(bfVMValue value);
bfBool32  bfVMValue_isBool(bfVMValue value);
bfBool32  bfVMValue_isTrue(bfVMValue value);
bfBool32  bfVMValue_isFalse(bfVMValue value);
bfBool32  bfVMValue_isPointer(bfVMValue value);
bfBool32  bfVMValue_isNumber(bfVMValue value);
bfVMValue bfVMValue_fromNull(void);
bfVMValue bfVMValue_fromBool(bfBool32 value);
bfVMValue bfVMValue_fromNumber(bfFloat64 value);
bfVMValue bfVMValue_fromPointer(const void* value);
bfFloat64 bfVMValue_asNumber(bfVMValue self);
void*     bfVMValue_asPointer(bfVMValue self);
bfVMValue bfVMValue_sub(bfVMValue lhs, bfVMValue rhs);
bfVMValue bfVMValue_mul(bfVMValue lhs, bfVMValue rhs);
bfVMValue bfVMValue_div(bfVMValue lhs, bfVMValue rhs);
bfBool32  bfVMValue_isThuthy(bfVMValue self);
bfBool32  bfVMValue_ee(bfVMValue lhs, bfVMValue rhs);
bfBool32  bfVMValue_lt(bfVMValue lhs, bfVMValue rhs);
bfBool32  bfVMValue_gt(bfVMValue lhs, bfVMValue rhs);
bfBool32  bfVMValue_ge(bfVMValue lhs, bfVMValue rhs);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_VALUE_H */
