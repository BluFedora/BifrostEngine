/******************************************************************************/
/*!
 * @file   bifrost_vm_gc.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @par
 *    Bifrost Scriping Language
 *
 * @brief
 *   A simple tracing garbage collector for the Bifrost Scripting Language.
 *   This uses a very basic mark and sweep algorithm.
 *
 *   References:
 *     [http://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/]
 *
 *    Something to Think About Language Design Wise.
 *     [https://stackoverflow.com/questions/28320213/why-do-we-need-to-call-luas-collectgarbage-twice]
 *
 * @version 0.0.1-beta
 * @date    2019-07-01
 *
 * @copyright Copyright (c) 2019-2020 Shareef Raheem
 */
/******************************************************************************/
#ifndef BIFROST_VM_GC_H
#define BIFROST_VM_GC_H

#include <stddef.h> /* size_t */

#if __cplusplus
extern "C" {
#endif
struct BifrostVM_t;
struct BifrostObj_t;

void   bfGCMarkObjects(struct BifrostVM_t* self);
size_t bfGCSweep(struct BifrostVM_t* self);
void   bfGCCollect(struct BifrostVM_t* self);
void*  bfGCDefaultAllocator(void* user_data, void* ptr, size_t old_size, size_t new_size);
void*  bfGCAllocMemory(struct BifrostVM_t* self, void* ptr, size_t old_size, size_t new_size);
void   bfGCPushRoot(struct BifrostVM_t* self, struct BifrostObj_t* obj);
void   bfGCPopRoot(struct BifrostVM_t* self);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_GC_H */
