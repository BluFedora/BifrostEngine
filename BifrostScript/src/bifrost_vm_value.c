/******************************************************************************/
/*!
 * @file   bifrost_vm_value.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   Helpers for the value representation for the vm.
 *   Uses Nan-Tagging for compact storage of objects.
 * 
 * @version 0.0.1
 * @date    2020-02-16
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#include "bifrost_vm_value.h"

bfBool32 bfVMValue_isNull(bfVMValue value)
{
  return value == k_VMValueNull;
}

bfBool32 bfVMValue_isBool(bfVMValue value)
{
  return bfVMValue_isTrue(value) || bfVMValue_isFalse(value);
}

bfBool32 bfVMValue_isTrue(bfVMValue value)
{
  return value == k_VMValueTrue;
}

bfBool32 bfVMValue_isFalse(bfVMValue value)
{
  return value == k_VMValueFalse;
}

bfBool32 bfVMValue_isPointer(bfVMValue value)
{
  return (value & k_VMValuePointerMask) == k_VMValuePointerMask;
}

bfBool32 bfVMValue_isNumber(bfVMValue value)
{
  return (value & k_QuietNan) != k_QuietNan;
}

bfVMValue bfVMValue_fromNull()
{
  return k_VMValueNull;
}

bfVMValue bfVMValue_fromBool(bfBool32 value)
{
  return value ? k_VMValueTrue : k_VMValueFalse;
}

bfVMValue bfVMValue_fromNumber(bfFloat64 value)
{
  uint64_t bits64;
  memcpy(&bits64, &value, sizeof(bits64));
  return bits64;
}

bfVMValue bfVMValue_fromPointer(const void* value)
{
  return value ? (bfVMValue)(k_VMValuePointerMask | (uint64_t)((uintptr_t)value))  : bfVMValue_fromNull();
}

bfFloat64 bfVmValue_asNumber(bfVMValue self)
{
  bfFloat64 num;
  memcpy(&num, &self, sizeof(num));

  return num;
}

void* bfVmValue_asPointer(bfVMValue self)
{
  return (void*)((uintptr_t)(self & ~k_VMValuePointerMask));
}

bfVMValue bfVMValue_sub(bfVMValue lhs, bfVMValue rhs)
{
  return bfVMValue_fromNumber(bfVmValue_asNumber(lhs) - bfVmValue_asNumber(rhs));
}

bfVMValue bfVMValue_mul(bfVMValue lhs, bfVMValue rhs)
{
  if (bfVMValue_isNumber(lhs) && bfVMValue_isNumber(rhs))
  {
    return bfVMValue_fromNumber(bfVmValue_asNumber(lhs) * bfVmValue_asNumber(rhs));
  }

  return bfVMValue_fromNull();
}

bfVMValue bfVMValue_div(bfVMValue lhs, bfVMValue rhs)
{
  if (bfVMValue_isNumber(lhs) && bfVMValue_isNumber(rhs))
  {
    return bfVMValue_fromNumber(bfVmValue_asNumber(lhs) / bfVmValue_asNumber(rhs));
  }

  return bfVMValue_fromNull();
}

bfBool32 bfVMValue_isThuthy(bfVMValue self)
{
  if (bfVMValue_isNull(self) || bfVMValue_isFalse(self) || (bfVMValue_isPointer(self) && !bfVmValue_asPointer(self)))
  {
    return bfFalse;
  }

  return bfTrue;
}

bfBool32 bfVMValue_ee(bfVMValue lhs, bfVMValue rhs)
{
  if (bfVMValue_isNumber(lhs) && bfVMValue_isNumber(rhs))
  {
    const bfFloat64 lhs_num = bfVmValue_asNumber(lhs);
    const bfFloat64 rhs_num = bfVmValue_asNumber(rhs);

    return lhs_num == rhs_num;
  }

  if (bfVMValue_isPointer(lhs) && bfVMValue_isPointer(rhs))
  {
    BifrostObj* const lhs_obj = BIFROST_AS_OBJ(lhs);
    BifrostObj* const rhs_obj = BIFROST_AS_OBJ(rhs);

    if (lhs_obj->type == rhs_obj->type)
    {
      if (lhs_obj->type == BIFROST_VM_OBJ_STRING)
      {
        BifrostObjStr* const lhs_string = (BifrostObjStr*)lhs_obj;
        BifrostObjStr* const rhs_string = (BifrostObjStr*)rhs_obj;

        return lhs_string->hash == rhs_string->hash && String_cmp(lhs_string->value, rhs_string->value) == 0;
      }
    }

    return bfFalse;
  }

  // TODO(Shareef): Specilize things more than just numbers
  return lhs == rhs;
}

bfBool32 bfVMValue_lt(bfVMValue lhs, bfVMValue rhs)
{
  if (bfVMValue_isNumber(lhs) && bfVMValue_isNumber(rhs))
  {
    return bfVmValue_asNumber(lhs) < bfVmValue_asNumber(rhs);
  }

  return bfFalse;
}

bfBool32 bfVMValue_gt(bfVMValue lhs, bfVMValue rhs)
{
  if (bfVMValue_isNumber(lhs) && bfVMValue_isNumber(rhs))
  {
    return bfVmValue_asNumber(lhs) > bfVmValue_asNumber(rhs);
  }

  return bfFalse;
}

bfBool32 bfVMValue_ge(bfVMValue lhs, bfVMValue rhs)
{
  if (bfVMValue_isNumber(lhs) && bfVMValue_isNumber(rhs))
  {
    return bfVmValue_asNumber(lhs) >= bfVmValue_asNumber(rhs);
  }

  return bfFalse;
}
