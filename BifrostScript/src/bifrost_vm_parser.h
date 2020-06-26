/******************************************************************************/
/*!
 * @file   bifrost_vm_parser.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   Handles the pasring of the languages grammar and uses
 *   the function builder to generate a function.   
 * 
 *   The output is a module with an executable function assuming the
 *   parser ran into no issues.
 * 
 *   References:
 *     [http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/]
 *
 * @version 0.0.1
 * @date    2020-02-16
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
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
