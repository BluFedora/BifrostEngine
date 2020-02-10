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
 *   NOTE(Shareef):
 *     The memory counted is exclusively what is allocated for objects and the VM
 *     struct itself.
 *     Ignored Allocations:
 *       > Any Array allocation from the Bifrost Array Data Structure.
 *       > Any HashMap allocation from the Bifrost Array Data Structure.
 *       > Any Dynamic String from the Bifrost Array Data Structure.
 *
 *     To fix this we need :
 *        > A unified memory model for both the scripting language
 *          and the data structures. The problem is that architectually the data
 *          structures are drop in libraries while the VM is a monolithic stateful
 *          object leading to different ways of customizing allcoations.
 *          If I do force the VM allocator on the data structures then if you
 *          use the Bifrost Scripting Language part of the Library you are forced.
 *          into it's memory model for the data structures.
 *
 *          (Maybe this is What Bifrost Needs??, Just one allocator for all.)
 *            (This once again leads to global state of storing the allocators.)
 *          (But then you a forced into the whole lib, that's the main problem.)
 *
 *        > To reimplement bare bone versions of the data structures just for the
 *          VM but not being allowed to reuse that code would be VERY inconvenient.
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
 * @copyright Copyright (c) 2019 Shareef Raheem
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
void*  bfGCDefaultAllocator(void* user_data, void* ptr, size_t old_size, size_t new_size, size_t alignment);
void*  bfGCAllocMemory(struct BifrostVM_t* self, void* ptr, size_t old_size, size_t new_size, size_t alignment);
void   bfGCPushRoot(struct BifrostVM_t* self, struct BifrostObj_t* obj);
void   bfGCPopRoot(struct BifrostVM_t* self);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_GC_H */
