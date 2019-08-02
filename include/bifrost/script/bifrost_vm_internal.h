#ifndef BIFROST_VM_INTERNAL_H
#define BIFROST_VM_INTERNAL_H

#include "../bifrost_vm.h"

#if __cplusplus
extern "C" {
#endif
void*    bfVM__valueToPointer(bfVMValue value);
double   bfVM__valueToNumber(bfVMValue value);
bfBool32 bfVM__valueToBool(bfVMValue value);
bfBool32 bfVM__valueIsPointer(bfVMValue value);
bfBool32 bfVM__valueIsNumber(bfVMValue value);
bfBool32 bfVM__valueIsBool(bfVMValue value);
bfBool32 bfVM__valueIsNull(bfVMValue value);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_INTERNAL_H */