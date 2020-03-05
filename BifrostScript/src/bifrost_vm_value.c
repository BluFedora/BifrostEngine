
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

bfVMValue bfVMValue_fromBool(bfBool32 v)
{
  return v ? VAL_TRUE : VAL_FALSE;
}

bfVMNumberT bfVmValue_asNumber(bfVMValue self)
{
  union
  {
    uint64_t    bits64;
    bfVMNumberT num;
  } c;

  c.bits64 = self;
  return c.num;
}

bfVMValue bfVMValue_mul(bfVMValue lhs, bfVMValue rhs)
{
  if (IS_NUMBER(lhs) && IS_NUMBER(rhs))
  {
    return FROM_NUMBER(
     bfVmValue_asNumber(lhs) * bfVmValue_asNumber(rhs));
  }

  return VAL_NULL;
}

bfVMValue bfVMValue_div(bfVMValue lhs, bfVMValue rhs)
{
  if (IS_NUMBER(lhs) && IS_NUMBER(rhs))
  {
    return FROM_NUMBER(
     bfVmValue_asNumber(lhs) / bfVmValue_asNumber(rhs));
  }

  return VAL_NULL;
}

bfBool32 bfVMValue_isThuthy(bfVMValue self)
{
  if (IS_NULL(self) || IS_FALSE(self) || (IS_POINTER(self) && !AS_POINTER(self)))
  {
    return bfFalse;
  }

  return bfTrue;
}

bfBool32 bfVMValue_ee(bfVMValue lhs, bfVMValue rhs)
{
  if (IS_NUMBER(lhs) && IS_NUMBER(rhs))
  {
    const bfVMNumberT lhs_num = bfVmValue_asNumber(lhs);
    const bfVMNumberT rhs_num = bfVmValue_asNumber(rhs);

    return lhs_num == rhs_num;
  }

  if (IS_POINTER(lhs) && IS_POINTER(rhs))
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
  if (IS_NUMBER(lhs) && IS_NUMBER(rhs))
  {
    return bfVmValue_asNumber(lhs) < bfVmValue_asNumber(rhs);
  }

  return bfFalse;
}

bfBool32 bfVMValue_gt(bfVMValue lhs, bfVMValue rhs)
{
  if (IS_NUMBER(lhs) && IS_NUMBER(rhs))
  {
    return bfVmValue_asNumber(lhs) > bfVmValue_asNumber(rhs);
  }

  return bfFalse;
}

bfBool32 bfVMValue_ge(bfVMValue lhs, bfVMValue rhs)
{
  if (IS_NUMBER(lhs) && IS_NUMBER(rhs))
  {
    return bfVmValue_asNumber(lhs) >= bfVmValue_asNumber(rhs);
  }

  return bfFalse;
}
