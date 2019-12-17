#ifndef BIFROST_VM_VALUE_H
#define BIFROST_VM_VALUE_H

#include "bifrost/bifrost_vm.h" /* uint64_t, bfFloat64 */

#if __cplusplus
extern "C" {
#endif

#define SIGN_BIT ((uint64_t)(1) << 63)
#define QUIET_NAN (uint64_t)(0x7FFC000000000000ULL)
#define TAG_MASK (uint64_t)0x3
/* static  const uint64_t TAG_NAN = 0x0; */
#define TAG_TRUE (uint64_t)0x1
#define TAG_FALSE (uint64_t)0x2
#define TAG_NULL (uint64_t)0x3                                    // 4-7 unused
#define POINTER_MASK (((uint64_t)1 << 63) | 0x7FFC000000000000ULL)  // SIGN_BIT | QUIET_NAN;
/* static  uint8_t TAG_GET(bfVMValue value) { return (uint8_t)(value & TAG_MASK); } */

#define TAG_DEF(t) (bfVMValue)((uint64_t)(QUIET_NAN | t))

/*
static const uint64_t VAL_TRUE  = TAG_DEF(TAG_TRUE);
static const uint64_t VAL_FALSE = TAG_DEF(TAG_FALSE);
static const uint64_t VAL_NULL  = TAG_DEF(TAG_NULL);
*/

#define VAL_TRUE  TAG_DEF(TAG_TRUE)
#define VAL_FALSE TAG_DEF(TAG_FALSE)
#define VAL_NULL  TAG_DEF(TAG_NULL)

// Used By:
// debug, value
static inline bfBool32 IS_NULL(bfVMValue v)
{
  return v == VAL_NULL;
}

// Used By:
// value
static inline bfBool32 IS_TRUE(bfVMValue v)
{
  return v == VAL_TRUE;
}

// Used By:
// value
static inline bfBool32 IS_FALSE(bfVMValue v)
{
  return v == VAL_FALSE;
}

// Used By:
// main, value, API, debug
static inline bfBool32 IS_BOOL(bfVMValue v)
{
  return IS_TRUE(v) || IS_FALSE(v);
}

// Used By:
// main, value, API, debug, gc
static inline bfBool32 IS_POINTER(bfVMValue v)
{
  return (v & POINTER_MASK) == POINTER_MASK;
}

// Used By:
// main, value, API, debug, gc, obj
static inline void* AS_POINTER(bfVMValue v)
{
  return (void*)((uintptr_t)(v & ~POINTER_MASK));
}

// Used By:
// API, gc, parser
static inline bfVMValue FROM_POINTER(const void* p)
{
  if (p == NULL)
  {
    return VAL_NULL;
  }

  return (bfVMValue)(POINTER_MASK | (uint64_t)((uintptr_t)(p)));
}

static inline bfBool32 IS_NUMBER(bfVMValue v)
{
  return (v & QUIET_NAN) != QUIET_NAN;
}

static inline bfVMValue FROM_NUMBER(bfVMNumberT p)
{
  union
  {
    uint64_t    bits64;
    bfVMNumberT num;
  } c;

  c.num = p;

  return (bfVMValue)(c.bits64);
}

// TOOD(SR): Inline into the main interpreter loop.
bfVMValue   bfVMValue_fromBool(bfBool32 v);
bfVMNumberT bfVmValue_asNumber(bfVMValue self);
bfVMValue   bfVMValue_mul(bfVMValue lhs, bfVMValue rhs);
bfVMValue   bfVMValue_div(bfVMValue lhs, bfVMValue rhs);
bfBool32    bfVMValue_isThuthy(bfVMValue self);
bfBool32    bfVMValue_ee(bfVMValue lhs, bfVMValue rhs);
bfBool32    bfVMValue_lt(bfVMValue lhs, bfVMValue rhs);
bfBool32    bfVMValue_gt(bfVMValue lhs, bfVMValue rhs);
bfBool32    bfVMValue_ge(bfVMValue lhs, bfVMValue rhs);

#if __cplusplus
}
#endif

#endif /* BIFROST_VM_VALUE_H */