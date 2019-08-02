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

void* bfVM__valueToPointer(bfVMValue value)
{
  return AS_POINTER(value);
}

double bfVM__valueToNumber(bfVMValue value)
{
  return bfVmValue_asNumber(value);
}

bfBool32 bfVM__valueToBool(bfVMValue value)
{
  return IS_TRUE(value);
}

bfBool32 bfVM__valueIsPointer(bfVMValue value)
{
  return IS_POINTER(value);
}

bfBool32 bfVM__valueIsNumber(bfVMValue value)
{
  return IS_NUMBER(value);
}

bfBool32 bfVM__valueIsBool(bfVMValue value)
{
  return IS_BOOL(value);
}

bfBool32 bfVM__valueIsNull(bfVMValue value)
{
  return IS_NULL(value);
}
