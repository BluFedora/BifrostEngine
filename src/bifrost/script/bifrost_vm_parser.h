// [http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/]
#ifndef BIFROST_VM_PARSER_H
#define BIFROST_VM_PARSER_H

#include "bifrost_vm_lexer.h"  // size_t, BifrostLexer, bfToken, bfBool32

#if __cplusplus
extern "C" {
#endif
struct BifrostVM_t;
struct BifrostVMFunctionBuilder_t;
struct BifrostObjModule_t;
struct BifrostObjClass_t;
struct BifrostObjFn_t;
struct LoopInfo_t;
typedef struct LoopInfo_t LoopInfo;

typedef struct BifrostParser_t
{
  struct BifrostParser_t*            parent;
  BifrostLexer*                      lexer;
  bfToken                            current_token;
  struct BifrostVMFunctionBuilder_t* fn_builder_stack;
  struct BifrostVMFunctionBuilder_t* fn_builder;
  struct BifrostObjModule_t*         current_module;
  struct BifrostObjClass_t*          current_clz;
  struct BifrostVM_t*                vm;
  bfBool32                           has_error;
  LoopInfo*                          loop_stack;

} BifrostParser;

void     bfParser_ctor(BifrostParser* self, struct BifrostVM_t* vm, BifrostLexer* lexer, struct BifrostObjModule_t* current_module);
bfBool32 bfParser_compile(BifrostParser* self);
void     bfParser_dtor(BifrostParser* self);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_PARSER_H */