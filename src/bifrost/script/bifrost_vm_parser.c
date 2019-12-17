#include "bifrost_vm_parser.h"

#include "bifrost_vm_function_builder.h"
#include "bifrost_vm_gc.h"
#include "bifrost_vm_obj.h"
#include <assert.h>
#include <bifrost/bifrost_vm.h>
#include <bifrost/data_structures/bifrost_array_t.h>
#include <bifrost/data_structures/bifrost_dynamic_string.h>
#include <stdarg.h> /* va_list, va_start, va_copy, va_end */
#include <stdio.h>

static const uint16_t BIFROST_VM_INVALID_SLOT = 0xFFFF;

extern uint32_t bfVM_getSymbol(BifrostVM* self, bfStringRange name);

uint32_t bfVM_xSetVariable(BifrostVMSymbol** variables, BifrostVM* vm, bfStringRange name, bfVMValue value)
{
  const size_t idx      = bfVM_getSymbol(vm, name);
  const size_t old_size = Array_size(variables);

  if (idx >= old_size)
  {
    const size_t new_size = idx + 1;

    Array_resize(variables, new_size);

    for (size_t i = old_size; i < new_size; ++i)
    {
      (*variables)[i].name  = "___UNUSED___";
      (*variables)[i].value = VAL_NULL;
    }
  }

  (*variables)[idx].name  = vm->symbols[idx];
  (*variables)[idx].value = value;

  return idx & 0xFFFFFFFF;
}

struct LoopInfo_t
{
  size_t    body_start;
  LoopInfo* parent;
};

void loopPush(BifrostParser* self, LoopInfo* loop)
{
  loop->parent     = self->loop_stack;
  self->loop_stack = loop;
}

void loopBodyStart(const BifrostParser* self)
{
  assert(self->loop_stack);
  self->loop_stack->body_start = Array_size(&self->fn_builder->instructions);
}

void loopPop(BifrostParser* self)
{
  assert(self->loop_stack);

  const size_t body_end = Array_size(&self->fn_builder->instructions);

  for (size_t i = self->loop_stack->body_start; i < body_end; ++i)
  {
    bfInstruction* const inst = self->fn_builder->instructions + i;

    if (*inst == BIFROST_INST_INVALID)
    {
      *inst = BIFROST_MAKE_INST_OP_AsBx(
       BIFROST_VM_OP_JUMP,
       0,
       body_end - i);
    }
  }

  self->loop_stack = self->loop_stack->parent;
}

typedef struct
{
  enum
  {
    V_LOCAL,
    V_MODULE,

  } kind;

  uint16_t location; /* V_LOCAL => register index, V_MODULE => symbol index */

} VariableInfo;

typedef struct
{
  uint16_t     write_loc;
  bfToken      token;
  VariableInfo var;

} ExprInfo;

static bfBool32      parserIsConstexpr(const BifrostParser* self);
static bfVMValue     parserConstexprValue(const BifrostParser* self);
static bfVMValue     parserTokenConstexprValue(const BifrostParser* self, const bfToken* token);
static uint32_t      parserGetSymbol(const BifrostParser* self, bfStringRange name);
static bfStringRange parserBeginFunction(BifrostParser* self, bfBool32 require_name);
static int           parserParseFunction(BifrostParser* self);
static void          parserEndFunction(BifrostParser* self, BifrostObjFn* out, int arity);
static void          parseClassDecl(BifrostParser* self);
static void          parseClassVarDecl(BifrostParser* self, BifrostObjClass* clz, bfBool32 is_static);
static void          parseClassFunc(BifrostParser* self, BifrostObjClass* clz, bfBool32 is_static);
static void          parseGroup(BifrostParser* self, ExprInfo* expr, const bfToken* token);
static void          parseLiteral(BifrostParser* self, ExprInfo* expr, const bfToken* token);
static void          parseNew(BifrostParser* self, ExprInfo* expr, const bfToken* token);
static void          parseVariable(BifrostParser* self, ExprInfo* expr, const bfToken* token);
static void          parseBinOp(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec);
static void          parseSubscript(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec);
static void          parseDotOp(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec);
static void          parseAssign(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec);
static void          parseCall(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec);
static void          parseMethodCall(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec);
static void          parseVarDecl(BifrostParser* self, bfBool32 is_static);
static void          parseFunctionDecl(BifrostParser* self);
static void          parseFunctionExpr(BifrostParser* self, ExprInfo* expr, const bfToken* token);
static void          parseImport(BifrostParser* self);
static void          parseForStatement(BifrostParser* self);
static void          parseExpr(BifrostParser* self, ExprInfo* expr_loc, int prec);
static uint32_t      parserMakeJump(BifrostParser* self);
static uint32_t      parserMakeJumpRev(BifrostParser* self);
static void          parserPatchJump(BifrostParser* self, uint32_t jump_idx, uint32_t cond_var, bfBool32 if_not);
static void          parserPatchJumpRev(BifrostParser* self, uint32_t jump_idx, uint32_t cond_var, bfBool32 if_not);
static VariableInfo  parserVariableFindLocal(const BifrostParser* self, bfStringRange name);
static VariableInfo  parserVariableFind(const BifrostParser* self, bfStringRange name);
static VariableInfo  parserVariableMakeLocal(const BifrostParser* self, bfStringRange name);
static void          parserVariableLoad(BifrostParser* self, VariableInfo variable, uint16_t write_loc);
static void          parserVariableStore(BifrostParser* self, VariableInfo variable, uint32_t read_loc);
static VariableInfo  parserVariableMakeTemp(uint16_t temp_loc);
static ExprInfo      exprMake(uint16_t write_loc, VariableInfo variable);
static ExprInfo      exprMakeTemp(uint16_t temp_loc);
static int           parserCallArgs(BifrostParser* self, uint16_t temp_first, int num_params, bfTokenType end_token);
static void          parserFinishCall(BifrostParser* self, VariableInfo fn, VariableInfo return_loc, uint32_t zero_slot);
static void          parseBlock(BifrostParser* self);
static bfBool32      bfParser_parse(BifrostParser* self);
static void          bfParser_pushBuilder(BifrostParser* self, const char* fn_name, size_t fn_name_len);
static void          bfParser_popBuilder(BifrostParser* self, BifrostObjFn* fn_out, int arity);

// TODO(SR): Make these use the same signature.
typedef void (*IPrefixParseletFn)(BifrostParser* parser, ExprInfo* expr_info, const bfToken* token);
typedef void (*IInfixParseletFn)(BifrostParser* parser, ExprInfo* expr_info, const ExprInfo* lhs, const bfToken* token, int prec);

enum
{
  PREC_NONE,        //
  PREC_ASSIGN,      // = += -= /= *=
  PREC_OR,          // ||
  PREC_AND,         // &&
  PREC_EQUALITY,    // == !=
  PREC_TERNARY,     // ? :
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * / %
  PREC_UNARY,       // - !
  PREC_PREFIX,      // ++x
  PREC_POSTFIX,     // x++
  PREC_CALL         // . () [] :
};

typedef struct
{
  IPrefixParseletFn prefix;
  IInfixParseletFn  infix;
  int               precedence;

} GrammarRule;

#define GRAMMAR_NONE()    \
  {                       \
    NULL, NULL, PREC_NONE \
  }
#define GRAMMAR_PREFIX(fn) \
  {                        \
    fn, NULL, PREC_NONE    \
  }
#define GRAMMAR_INFIX(fn, prec) \
  {                             \
    NULL, fn, prec              \
  }
#define GRAMMAR_BOTH(prefix, infix, prec) \
  {                                       \
    prefix, infix, prec                   \
  }

static const GrammarRule s_Rules[BIFROST_TOKEN_SUPER + 1] =
 {
  GRAMMAR_BOTH(parseGroup, parseCall, PREC_CALL),  // (L_PAREN);
  GRAMMAR_NONE(),                                  // (R_PAREN);
  GRAMMAR_INFIX(parseSubscript, PREC_CALL),        // (L_SQR_BOI);
  GRAMMAR_NONE(),                                  // (R_SQR_BOI);
  GRAMMAR_NONE(),                                  // (L_CURLY);
  GRAMMAR_NONE(),                                  // (R_CURLY);
  GRAMMAR_NONE(),                                  // (HASHTAG);
  GRAMMAR_INFIX(parseMethodCall, PREC_CALL),       // (COLON);
  GRAMMAR_NONE(),                                  // (SEMI_COLON);
  GRAMMAR_NONE(),                                  // (COMMA);
  GRAMMAR_INFIX(parseAssign, PREC_ASSIGN),         // (EQUALS);
  GRAMMAR_INFIX(parseBinOp, PREC_TERM),            // (PLUS);
  GRAMMAR_INFIX(parseBinOp, PREC_TERM),            // (MINUS);
  GRAMMAR_INFIX(parseBinOp, PREC_FACTOR),          // (MULT);
  GRAMMAR_INFIX(parseBinOp, PREC_FACTOR),          // (DIV);
  GRAMMAR_INFIX(parseAssign, PREC_ASSIGN),         // (PLUS_EQUALS);
  GRAMMAR_INFIX(parseAssign, PREC_ASSIGN),         // (MINUS_EQUALS);
  GRAMMAR_NONE(),                                  // (INCREMENT);
  GRAMMAR_NONE(),                                  // (DECREMENT);
  GRAMMAR_INFIX(parseDotOp, PREC_CALL),            // (DOT);
  GRAMMAR_PREFIX(parseVariable),                   // (IDENTIFIER);
  GRAMMAR_NONE(),                                  // (VAR_DECL);
  GRAMMAR_NONE(),                                  // (IMPORT);
  GRAMMAR_PREFIX(parseFunctionExpr),               // (FUNC);
  GRAMMAR_NONE(),                                  // (BIFROST_TOKEN_CLASS);
  GRAMMAR_NONE(),                                  // (BIFROST_TOKEN_PRINT);
  GRAMMAR_NONE(),                                  // (CTRL_IF);
  GRAMMAR_NONE(),                                  // (CTRL_ELSE);
  GRAMMAR_INFIX(parseBinOp, PREC_EQUALITY),        // (CTRL_EE);
  GRAMMAR_INFIX(parseBinOp, PREC_COMPARISON),      // (CTRL_LT);
  GRAMMAR_INFIX(parseBinOp, PREC_COMPARISON),      // (CTRL_GT);
  GRAMMAR_INFIX(parseBinOp, PREC_COMPARISON),      // (CTRL_LE);
  GRAMMAR_INFIX(parseBinOp, PREC_COMPARISON),      // (CTRL_GE);
  GRAMMAR_INFIX(parseBinOp, PREC_OR),              // (CTRL_OR);
  GRAMMAR_INFIX(parseBinOp, PREC_AND),             // (CTRL_AND);
  GRAMMAR_INFIX(parseBinOp, PREC_EQUALITY),        // (CTRL_NE);
  GRAMMAR_NONE(),                                  // (CTRL_WHILE);
  GRAMMAR_NONE(),                                  // (CTRL_FOR);
  GRAMMAR_NONE(),                                  // (CTRL_RETURN);
  GRAMMAR_NONE(),                                  // (CTRL_NEGATE);
  GRAMMAR_PREFIX(parseLiteral),                    // (CONST_STR);
  GRAMMAR_PREFIX(parseLiteral),                    // (CONST_REAL);
  GRAMMAR_PREFIX(parseLiteral),                    // (CONST_BOOL);
  GRAMMAR_PREFIX(parseLiteral),                    // (CONST_NIL);
  GRAMMAR_NONE(),                                  // (EOP);
  GRAMMAR_NONE(),                                  // (BIFROST_TOKEN_CTRL_BREAK)
  GRAMMAR_PREFIX(parseNew),                        // (BIFROST_TOKEN_NEW)
  GRAMMAR_NONE(),                                  // (BIFROST_TOKEN_STATIC)
  GRAMMAR_NONE(),                                  // (BIFROST_TOKEN_AS)
  GRAMMAR_NONE(),                                  // (BIFROST_TOKEN_SUPER)
};

// static_assert(std::size(s_Rules) == (std::size_t(TokenType::EOP) + 1u), "This parse table must match each token.");

static void bfEmitError(BifrostParser* self, const char* format, ...)
{
#define BIRFOST_ERROR_MESSAGE_MAX_LEN 256

  char error_msg[BIRFOST_ERROR_MESSAGE_MAX_LEN];

  va_list args;
  va_start(args, format);

  // #pragma clang diagnostic push
  // #pragma clang diagnostic ignored "-Wformat-nonliteral"
  const int num_chars = vsnprintf(error_msg, BIRFOST_ERROR_MESSAGE_MAX_LEN, format, args);
  va_end(args);
  // #pragma clang diagnostic pop

  if (num_chars < BIRFOST_ERROR_MESSAGE_MAX_LEN)
  {
    const size_t line_no  = self->lexer->current_line_no;
    const size_t line_bgn = self->lexer->line_pos_bgn;
    const size_t line_end = self->lexer->line_pos_end;

    String_sprintf(
     &self->vm->last_error,
     "Line(%zu): %s\nLine(%zu): '%.*s'",
     line_no,
     error_msg,
     line_no,
     line_end - line_bgn,
     self->lexer->source_bgn + line_bgn);

    if (self->vm->params.error_fn)
    {
      self->vm->params.error_fn(self->vm, BIFROST_VM_ERROR_COMPILE, (int)line_no, self->vm->last_error);
    }
  }

  self->has_error = bfTrue;
}

static inline bfBool32 bfParser_eat(BifrostParser* self, bfTokenType type, bfBool32 is_optional, const char* error_msg)
{
  if (self->current_token.type == type)
  {
    self->current_token = bfLexer_nextToken(self->lexer);
    return bfTrue;
  }

  if (!is_optional)
  {
    // const char*  expected  = tokentypeToString(type);
    // const char*  received  = tokentypeToString(self->current_token.type);

    bfEmitError(self, "%s", error_msg);

    while (self->current_token.type != BIFROST_TOKEN_SEMI_COLON && self->current_token.type != EOP)
    {
      self->current_token = bfLexer_nextToken(self->lexer);
    }

    self->has_error = bfTrue;
  }

  return bfFalse;
}

static inline bfBool32 bfParser_match(BifrostParser* self, bfTokenType type)
{
  // TODO(SR):
  //   Report error on "self->current_token.type == EOP"
  return bfParser_eat(self, type, bfTrue, NULL) || self->current_token.type == EOP;
}

static inline bfBool32 bfParser_is(BifrostParser* self, bfTokenType type)
{
  return self->current_token.type == type || self->current_token.type == EOP /*  || self->has_error */;
}

static inline GrammarRule typeToRule(bfTokenType type)
{
  assert((size_t)type < bfCArraySize(s_Rules) && "Invalid token type.");
  return s_Rules[type];
}

#define builder() self->fn_builder

static void parseVarDecl(BifrostParser* self, bfBool32 is_static)
{
  /* GRAMMAR(Shareef):
       var <identifier>;
       var <identifier> = <expr>;
       static var <identifier>;
       static var <identifier> = <expr>;
  */

  const bfStringRange name = self->current_token.as.str_range;

  if (bfParser_eat(self, IDENTIFIER, bfFalse, "Expected identifier after var keyword."))
  {
    if (is_static)
    {
      const uint32_t location = bfVM_xSetVariable(&self->current_module->variables, self->vm, name, VAL_NULL);

      if (bfParser_match(self, EQUALS))
      {
        VariableInfo var;
        var.kind     = V_MODULE;
        var.location = location;

        const uint16_t expr_loc = bfVMFunctionBuilder_pushTemp(builder(), 1);
        ExprInfo       expr     = exprMake(expr_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
        parseExpr(self, &expr, PREC_NONE);
        parserVariableStore(self, var, expr_loc);
        bfVMFunctionBuilder_popTemp(builder(), expr_loc);
      }
    }
    else
    {
      const VariableInfo var = parserVariableMakeLocal(self, name);

      if (bfParser_match(self, EQUALS))
      {
        ExprInfo expr = exprMake(var.location, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
        parseExpr(self, &expr, PREC_NONE);
      }
    }

    bfParser_eat(self, BIFROST_TOKEN_SEMI_COLON, bfFalse, "Expected semi colon after variable declaration.");
  }
}

static void parseExpr(BifrostParser* self, ExprInfo* expr_loc, int prec)
{
  bfToken           token = self->current_token;
  const GrammarRule rule  = typeToRule(token.type);

  if (!rule.prefix)
  {
    bfEmitError(self, "No prefix operator for token: %s", tokentypeToString(token.type));
    return;
  }

  bfParser_match(self, token.type);

  rule.prefix(self, expr_loc, &token);

  while (prec < s_Rules[self->current_token.type].precedence)
  {
    expr_loc->token        = token;
    token                  = self->current_token;
    IInfixParseletFn infix = typeToRule(token.type).infix;

    if (!infix)
    {
      bfEmitError(self, "No infix operator for token: %s", tokentypeToString(token.type));
      return;
    }

    bfParser_match(self, token.type);

    infix(self, expr_loc, expr_loc, &token, s_Rules[token.type].precedence);
  }
}

void bfParser_ctor(BifrostParser* self, struct BifrostVM_t* vm, BifrostLexer* lexer, struct BifrostObjModule_t* current_module)
{
  self->parent           = vm->parser_stack;
  vm->parser_stack       = self;
  self->lexer            = lexer;
  self->current_token    = bfLexer_nextToken(lexer);
  self->fn_builder_stack = Array_new(BifrostVMFunctionBuilder, 2);
  self->has_error        = bfFalse;
  self->current_clz      = NULL;
  self->loop_stack       = NULL;
  self->vm               = vm;
  self->current_module   = current_module;
  bfParser_pushBuilder(self, self->current_module->name, String_length(self->current_module->name));
}

bfBool32 bfParser_compile(BifrostParser* self)
{
  while (bfParser_parse(self))
  {
  }

  return self->has_error;
}

extern void bfVMObject__delete(struct BifrostVM_t* self, BifrostObj* obj);

void bfParser_dtor(BifrostParser* self)
{
  self->vm->parser_stack = self->parent;

  // NOTE(Shareef): Handles the case where a script is recompiled into
  //   The same module.
  // TODO(Shareef): This is probably horrible broken and since other
  //   thinsg probably need to be cleared from the module during a recompile.
  BifrostObjFn* const module_fn = &self->current_module->init_fn;
  if (module_fn->name)
  {
    bfVMObject__delete(self->vm, &module_fn->super);
    module_fn->name = NULL;
  }

  bfParser_popBuilder(self, module_fn, 0);
  Array_delete(&self->fn_builder_stack);
}

void bfParser_pushBuilder(BifrostParser* self, const char* fn_name, size_t fn_name_len)
{
  self->fn_builder = Array_emplace(&self->fn_builder_stack);
  bfVMFunctionBuilder_ctor(self->fn_builder);
  bfVMFunctionBuilder_begin(self->fn_builder, fn_name, fn_name_len);
  self->fn_builder->lexer = self->lexer;
}

#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

bfBool32 bfParser_parse(BifrostParser* self)
{
  switch (self->current_token.type)
  {
    default:
      bfEmitError(self, "Unhandled Token (%s)\n", tokentypeToString(self->current_token.type));
      bfParser_match(self, self->current_token.type);
    case EOP:
    {
      return bfFalse;
    }
    case BIFROST_TOKEN_SEMI_COLON:
    {
      bfParser_match(self, BIFROST_TOKEN_SEMI_COLON);
      break;
    }
    case BIFROST_TOKEN_CTRL_BREAK:
    {
      bfParser_match(self, BIFROST_TOKEN_CTRL_BREAK);
      bfParser_eat(self, BIFROST_TOKEN_SEMI_COLON, bfFalse, "Nothing must follow a 'break' statement.");

      if (self->loop_stack)
      {
        bfVMFunctionBuilder_addInstBreak(builder());
      }
      else
      {
        bfEmitError(self, "break cannot be used outside of loop.");
      }

      // NOTE(Shareef):
      //   Same ending note for [BIFROST_TOKEN_CTRL_RETURN].
      //   Search "@UnreachableCode"
      return bfFalse;
    }
    case BIFROST_TOKEN_CTRL_RETURN:
    {
      bfParser_match(self, BIFROST_TOKEN_CTRL_RETURN);

      const uint16_t expr_loc = bfVMFunctionBuilder_pushTemp(
       builder(),
       1);

      if (!bfParser_is(self, BIFROST_TOKEN_SEMI_COLON))
      {
        ExprInfo ret_expr = exprMake(expr_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
        parseExpr(self, &ret_expr, PREC_NONE);
      }

      bfVMFunctionBuilder_addInstABx(
       builder(),
       BIFROST_VM_OP_RETURN,
       0,
       expr_loc);

      bfVMFunctionBuilder_popTemp(builder(), expr_loc);

      bfParser_match(self, BIFROST_TOKEN_SEMI_COLON);

      while (!bfParser_is(self, BIFROST_TOKEN_R_CURLY))
      {
        bfParser_match(self, self->current_token.type);
      }

      // NOTE(Shareef):
      //   @UnreachableCode
      //   Since nothing can be executed after a return we just
      //   keep going until we hit a closing curly brace.

      // TODO(SR): Check if returning false may work
      //   This optimizes away unreachable code.
      return bfFalse;
    }
    case BIFROST_TOKEN_CLASS:
    {
      bfParser_match(self, BIFROST_TOKEN_CLASS);
      parseClassDecl(self);
      break;
    }
    case BIFROST_TOKEN_CTRL_IF:
    {
      bfParser_match(self, BIFROST_TOKEN_CTRL_IF);

      bfParser_eat(self, BIFROST_TOKEN_L_PAREN, bfFalse, "If statements must have l paren after if keyword.");

      const uint16_t expr_loc = bfVMFunctionBuilder_pushTemp(builder(), 1);

      ExprInfo expr = exprMake(expr_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
      parseExpr(self, &expr, PREC_NONE);

      bfParser_eat(self, BIFROST_TOKEN_R_PAREN, bfFalse, "If statements must have r paren after condition.");

      const uint32_t if_jump = parserMakeJump(self);

      bfVMFunctionBuilder_popTemp(builder(), expr_loc);

      parseBlock(self);

      if (bfParser_match(self, BIFROST_TOKEN_CTRL_ELSE))
      {
        const uint32_t else_jump = parserMakeJump(self);

        // NOTE(Shareef):
        //   [expr_loc] an be used here since the actual
        //   use is where the jump is NOT here.
        parserPatchJump(self, if_jump, expr_loc, bfTrue);

        bfParser_parse(self);

        parserPatchJump(self, else_jump, BIFROST_VM_INVALID_SLOT, bfFalse);
      }
      else
      {
        parserPatchJump(self, if_jump, expr_loc, bfTrue);
      }
      break;
    }
    case BIFROST_TOKEN_CTRL_WHILE:
    {
      /* GRAMMAR(Shareef):
           while (<expr>) { <statement>... }
       */

      bfParser_match(self, BIFROST_TOKEN_CTRL_WHILE);

      LoopInfo loop;
      loopPush(self, &loop);

      const uint16_t expr_loc = bfVMFunctionBuilder_pushTemp(builder(), 1);
      const uint32_t jmp_back = parserMakeJumpRev(self);

      bfParser_eat(self, BIFROST_TOKEN_L_PAREN, bfFalse, "while statements must be followed by a left parenthesis.");
      ExprInfo expr = exprMake(expr_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
      parseExpr(self, &expr, PREC_NONE);
      bfParser_eat(self, BIFROST_TOKEN_R_PAREN, bfFalse, "while statement conditions must end with a right parenthesis.");

      const uint32_t jmp_skip = parserMakeJump(self);

      loopBodyStart(self);
      bfParser_parse(self);
      parserPatchJumpRev(self, jmp_back, BIFROST_VM_INVALID_SLOT, bfFalse);
      parserPatchJump(self, jmp_skip, expr_loc, bfTrue);

      bfVMFunctionBuilder_popTemp(builder(), expr_loc);
      loopPop(self);
      break;
    }
    case BIFROST_TOKEN_STATIC:
    case BIFROST_TOKEN_VAR_DECL:
    {
      const bfBool32 is_static = bfParser_match(self, BIFROST_TOKEN_STATIC);

      bfParser_match(self, BIFROST_TOKEN_VAR_DECL);
      parseVarDecl(self, is_static);
      break;
    }
    case BIFROST_TOKEN_FUNC_DECL:
    {
      bfParser_match(self, BIFROST_TOKEN_FUNC_DECL);
      parseFunctionDecl(self);
      break;
    }
    case IMPORT:
    {
      bfParser_match(self, IMPORT);
      parseImport(self);
      break;
    }
    case CTRL_FOR:
    {
      bfParser_match(self, CTRL_FOR);
      parseForStatement(self);
      break;
    }
    case IDENTIFIER:
    {
      const uint16_t working_loc = bfVMFunctionBuilder_pushTemp(builder(), 1);
      ExprInfo       expr        = exprMake(working_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
      parseExpr(self, &expr, PREC_NONE);
      bfParser_match(self, BIFROST_TOKEN_SEMI_COLON);
      bfVMFunctionBuilder_popTemp(builder(), working_loc);
      break;
    }
    case L_CURLY:
    {
      parseBlock(self);
      break;
    }
  }

  return bfTrue;
}

void bfParser_popBuilder(BifrostParser* self, BifrostObjFn* fn_out, int arity)
{
  bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_RETURN, 0, 0);

  bfVMFunctionBuilder_end(self->fn_builder, fn_out, arity);
  bfVMFunctionBuilder_dtor(self->fn_builder);
  Array_pop(&self->fn_builder_stack);
  self->fn_builder = Array_back(&self->fn_builder_stack);
}

// Recursive Decent Parsers

static void parseFunctionDecl(BifrostParser* self)
{
  const bfBool32 is_local = Array_size(&self->fn_builder_stack) != 1;

  const bfStringRange name_str = parserBeginFunction(self, bfTrue);
  const int           arity    = parserParseFunction(self);
  BifrostObjFn* const fn       = bfVM_createFunction(self->vm, self->current_module);
  const bfVMValue     fn_value = FROM_POINTER(fn);
  parserEndFunction(self, fn, arity);

  if (is_local)
  {
    const VariableInfo fn_var = parserVariableMakeLocal(self, name_str);
    const uint32_t     k_loc  = bfVMFunctionBuilder_addConstant(builder(), fn_value);
    bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, (uint16_t)fn_var.location, k_loc + BIFROST_VM_OP_LOAD_BASIC_CONSTANT);
  }
  else
  {
    bfVM_xSetVariable(&self->current_module->variables, self->vm, name_str, fn_value);
  }
}

static void parseFunctionExpr(BifrostParser* self, ExprInfo* expr, const bfToken* token)
{
  (void)token;

  parserBeginFunction(self, bfFalse);
  const int           arity = parserParseFunction(self);
  BifrostObjFn* const fn    = bfVM_createFunction(self->vm, self->current_module);
  parserEndFunction(self, fn, arity);

  const uint32_t k_loc = bfVMFunctionBuilder_addConstant(builder(), FROM_POINTER(fn));
  bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, expr->write_loc, k_loc + BIFROST_VM_OP_LOAD_BASIC_CONSTANT);
}

extern BifrostObjModule* bfVM_findModule(BifrostVM* self, const char* name, size_t name_len);
extern bfVMValue         bfVM_stackFindVariable(BifrostObjModule* module_obj, const char* variable, size_t variable_len);
extern BifrostObjModule* bfVM_importModule(BifrostVM* self, const char* from, const char* name, size_t name_len);

static void parseImport(BifrostParser* self)
{
  /* GRAMMAR(Shareef):
      import <const-string> for <identifier>, <identifier> (= | as) <identifier>, ...;
      
      TODO(SR): The second version imports all of the modules variables??
      import <const-string>;
  */

  const bfToken       name_token = self->current_token;
  const bfStringRange name_str   = name_token.as.str_range;
  bfParser_eat(self, CONST_STR, bfFalse, "Import statments must be followed by a constant string.");

  BifrostObjModule* imported_module = bfVM_findModule(self->vm, name_str.bgn, bfStringRange_length(&name_str));

  if (!imported_module)
  {
    imported_module = bfVM_importModule(self->vm, self->current_module->name, name_str.bgn, bfStringRange_length(&name_str));

    if (!imported_module)
    {
      bfEmitError(self, "Failed to import module: '%.*s'", bfStringRange_length(&name_str), name_str.bgn);
    }
  }

  if (bfParser_match(self, CTRL_FOR))
  {
    do
    {
      const bfStringRange var_str = self->current_token.as.str_range;
      bfParser_eat(self, BIFROST_TOKEN_IDENTIFIER, bfFalse, "Imported variable name must be an identifier.");

      const bfStringRange src_name = var_str;
      bfStringRange       dst_name = var_str;

      if (bfParser_match(self, BIFROST_TOKEN_EQUALS) || bfParser_match(self, BIFROST_TOKEN_AS))
      {
        dst_name = self->current_token.as.str_range;
        bfParser_eat(self, BIFROST_TOKEN_IDENTIFIER, bfFalse, "Import alias must be an identifier.");
      }

      if (imported_module)
      {
        bfVM_xSetVariable(&self->current_module->variables, self->vm, dst_name, bfVM_stackFindVariable(imported_module, src_name.bgn, bfStringRange_length(&src_name)));
      }
    } while (bfParser_match(self, BIFROST_TOKEN_COMMA));
  }

  bfParser_eat(self, BIFROST_TOKEN_SEMI_COLON, bfFalse, "Import must end with a semi-colon.");
}

static void parseForStatement(BifrostParser* self)
{
  /* GRAMMAR(Shareef):
      for (
        <statement> | <none>;
        <expr>      | <none>;
        <statement> | <none>) {}
      (; | <none>)
  */
  /*
    // This gets compiled into roughly:
    //
    // <statement>;
    //
    // label_cond:
    //   if (<cond>)
    //     goto label_loop;
    //   else
    //     goto label_loop_end;
    //
    // label_inc:
    //   <increment>
    //   goto label_cond;
    //
    // label_loop:
    //   <statements>...;
    //   goto label_inc;
    // label_loop_end:
    //
  */

  bfParser_eat(self, BIFROST_TOKEN_L_PAREN, bfFalse, "Expected '(' after 'for' keyword.");

  bfVMFunctionBuilder_pushScope(builder());
  {
    LoopInfo loop;

    if (!bfParser_match(self, BIFROST_TOKEN_SEMI_COLON))
    {
      bfParser_parse(self);
    }

    const uint32_t inc_to_cond = parserMakeJumpRev(self);
    const uint16_t cond_loc    = bfVMFunctionBuilder_pushTemp(builder(), 1);

    if (!bfParser_is(self, BIFROST_TOKEN_SEMI_COLON))
    {
      ExprInfo cond_expr = exprMake(cond_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
      parseExpr(self, &cond_expr, PREC_NONE);
    }
    else
    {
      bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, cond_loc, BIFROST_VM_OP_LOAD_BASIC_TRUE);
    }

    const uint32_t cond_to_loop = parserMakeJump(self);
    const uint32_t cond_to_end  = parserMakeJump(self);

    bfVMFunctionBuilder_popTemp(builder(), cond_loc);

    bfParser_match(self, BIFROST_TOKEN_SEMI_COLON);

    const uint32_t loop_to_inc = parserMakeJumpRev(self);
    if (!bfParser_match(self, R_PAREN))
    {
      bfParser_parse(self);
      bfParser_eat(self, R_PAREN, bfFalse, "Expected ( list after for loop.");
    }
    parserPatchJumpRev(self, inc_to_cond, BIFROST_VM_INVALID_SLOT, bfFalse);

    parserPatchJump(self, cond_to_loop, cond_loc, bfFalse);
    loopPush(self, &loop);
    loopBodyStart(self);
    parseBlock(self);
    parserPatchJumpRev(self, loop_to_inc, BIFROST_VM_INVALID_SLOT, bfFalse);

    parserPatchJump(self, cond_to_end, cond_loc, bfTrue);
    loopPop(self);
  }
  bfVMFunctionBuilder_popScope(builder());
  bfParser_match(self, BIFROST_TOKEN_SEMI_COLON);
}

// Expr Parsers

static void parseGroup(BifrostParser* self, ExprInfo* expr_info, const bfToken* token)
{
  (void)token;
  parseExpr(self, expr_info, PREC_NONE);
  bfParser_eat(self, R_PAREN, bfFalse, "Missing closing parenthesis for an group expression.");
}

static void parseLiteral(BifrostParser* self, ExprInfo* expr_info, const bfToken* token)
{
  const bfVMValue constexpr_value = parserTokenConstexprValue(self, token);

  if (constexpr_value == VAL_TRUE)
  {
    bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, expr_info->write_loc, BIFROST_VM_OP_LOAD_BASIC_TRUE);
  }
  else if (constexpr_value == VAL_FALSE)
  {
    bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, expr_info->write_loc, BIFROST_VM_OP_LOAD_BASIC_FALSE);
  }
  else if (constexpr_value == VAL_NULL)
  {
    bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, expr_info->write_loc, BIFROST_VM_OP_LOAD_BASIC_NULL);
  }
  else
  {
    const uint32_t konst_loc = bfVMFunctionBuilder_addConstant(builder(), constexpr_value);

    bfVMFunctionBuilder_addInstABx(
     builder(),
     BIFROST_VM_OP_LOAD_BASIC,
     expr_info->write_loc,
     konst_loc + BIFROST_VM_OP_LOAD_BASIC_CONSTANT);
  }
}

static void parseNew(BifrostParser* self, ExprInfo* expr, const bfToken* token)
{
  (void)token;

  const bfStringRange clz_name = self->current_token.as.str_range;

  if (bfParser_eat(self, BIFROST_TOKEN_IDENTIFIER, bfFalse, "'new' must be called on a class name."))
  {
    const VariableInfo clz_var = parserVariableFind(self, clz_name);
    const uint16_t     clz_loc = bfVMFunctionBuilder_pushTemp(builder(), 1);

    parserVariableLoad(self, clz_var, clz_loc);

    bfVMFunctionBuilder_addInstABx(
     builder(),
     BIFROST_VM_OP_NEW_CLZ,
     expr->write_loc,
     clz_loc);

    bfStringRange ctor_name = bfMakeStringRangeC("ctor");

    if (bfParser_match(self, BIFROST_TOKEN_DOT))
    {
      if (bfParser_is(self, BIFROST_TOKEN_IDENTIFIER))
      {
        ctor_name = self->current_token.as.str_range;
      }

      bfParser_eat(self, BIFROST_TOKEN_IDENTIFIER, bfFalse, "Expected the name of the constructor to call.");
    }

    if (bfParser_match(self, BIFROST_TOKEN_L_PAREN))
    {
      const uint16_t ctor_sym = (uint16_t)parserGetSymbol(self, ctor_name);

      bfVMFunctionBuilder_addInstABC(
       builder(),
       BIFROST_VM_OP_LOAD_SYMBOL,
       clz_loc,
       clz_loc,
       ctor_sym);

      const VariableInfo function_var = parserVariableMakeTemp(clz_loc);
      const VariableInfo return_var   = parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT);

      parserFinishCall(self, function_var, return_var, expr->write_loc);
    }

    bfVMFunctionBuilder_popTemp(builder(), clz_loc);
  }
}

static void parseVariable(BifrostParser* self, ExprInfo* expr, const bfToken* token)
{
  const bfStringRange var_name = token->as.str_range;
  VariableInfo        var      = expr->var;

  if (var.location == BIFROST_VM_INVALID_SLOT)
  {
    var = parserVariableFind(self, var_name);
  }

  if (var.location != BIFROST_VM_INVALID_SLOT)
  {
    parserVariableLoad(self, var, expr->write_loc);
    (*expr) = exprMake(expr->write_loc, var);
  }
  else
  {
    bfEmitError(self, "Error invalid var(%.*s)", (int)(var_name.end - var_name.bgn), var_name.bgn);
  }
}

static void parseBinOp(BifrostParser* self, ExprInfo* expr_info, const ExprInfo* lhs, const bfToken* token, int prec)
{
  bfInstructionOp inst   = BIFROST_VM_OP_CMP_EE;
  const char      bin_op = token->as.str[0];

  switch (bin_op)
  {
    case '=': inst = BIFROST_VM_OP_CMP_EE; break;
    case '!': inst = BIFROST_VM_OP_CMP_NE; break;
    case '+': inst = BIFROST_VM_OP_MATH_ADD; break;
    case '-': inst = BIFROST_VM_OP_MATH_SUB; break;
    case '*': inst = BIFROST_VM_OP_MATH_MUL; break;
    case '/': inst = BIFROST_VM_OP_MATH_DIV; break;
    case '%': inst = BIFROST_VM_OP_MATH_MOD; break;
    case '^': inst = BIFROST_VM_OP_MATH_POW; break;
    case '|': inst = BIFROST_VM_OP_CMP_OR; break;
    case '&': inst = BIFROST_VM_OP_CMP_AND; break;
    case '<': inst = token->type == CTRL_LE ? BIFROST_VM_OP_CMP_LE : BIFROST_VM_OP_CMP_LT; break;
    case '>': inst = token->type == CTRL_GE ? BIFROST_VM_OP_CMP_GE : BIFROST_VM_OP_CMP_GT; break;
    default: bfEmitError(self, "Invalid Binary Operator. %s", token->as.str); break;
  }

  const uint16_t rhs_loc  = bfVMFunctionBuilder_pushTemp(builder(), 1);
  ExprInfo       rhs_expr = exprMake(rhs_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
  const uint32_t jmp      = bin_op == '&' || bin_op == '|' ? parserMakeJump(self) : BIFROST_VM_INVALID_SLOT;

  parseExpr(self, &rhs_expr, prec);

  bfVMFunctionBuilder_addInstABC(
   builder(),
   inst,
   expr_info->write_loc,
   lhs->write_loc,
   rhs_loc);

  if (jmp != BIFROST_VM_INVALID_SLOT)
  {
    parserPatchJump(self, jmp, expr_info->write_loc, bin_op == '&');
  }

  bfVMFunctionBuilder_popTemp(builder(), rhs_loc);
}

static void parseSubscript(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec)
{
  (void)lhs;
  (void)token;
  (void)prec;

  // TODO(SR):
  //   Find some way to merge this into the main function call routine.
  const uint16_t subscript_op_loc = bfVMFunctionBuilder_pushTemp(builder(), 3);
  const uint16_t self_loc         = subscript_op_loc + 1;
  const uint16_t temp_first       = subscript_op_loc + 2;
  const uint32_t subscript_sym    = parserGetSymbol(self, bfMakeStringRangeC("[]"));
  int            num_args         = 1;

  parserVariableLoad(self, expr->var, self_loc);

  const uint32_t load_sym_inst = (uint32_t)Array_size(&builder()->instructions);
  bfVMFunctionBuilder_addInstABC(
   builder(),
   BIFROST_VM_OP_LOAD_SYMBOL,
   subscript_op_loc,
   self_loc,
   (uint16_t)subscript_sym);

  bfVMFunctionBuilder_addInstABx(
   builder(),
   BIFROST_VM_OP_STORE_MOVE,
   temp_first,
   self_loc);

  num_args = parserCallArgs(self, temp_first, num_args, BIFROST_TOKEN_R_SQR_BOI);

  bfParser_eat(self, BIFROST_TOKEN_R_SQR_BOI, bfFalse, "Subscript call must end with a closing square boi.");

  if (bfParser_match(self, BIFROST_TOKEN_EQUALS))
  {
    const size_t   subscript_assign_sym = parserGetSymbol(self, bfMakeStringRangeC("[]="));
    bfInstruction* inst                 = builder()->instructions + load_sym_inst;

    bfInstPatchX(inst, OP, BIFROST_VM_OP_LOAD_SYMBOL);
    bfInstPatchX(inst, RC, subscript_assign_sym);

    const uint16_t param_loc  = bfVMFunctionBuilder_pushTemp(builder(), 1);
    ExprInfo       param_expr = exprMakeTemp(param_loc);
    parseExpr(self, &param_expr, PREC_NONE);
    ++num_args;
  }

  bfVMFunctionBuilder_addInstABC(
   builder(),
   BIFROST_VM_OP_CALL_FN,
   temp_first,
   subscript_op_loc,
   num_args);

  bfVMFunctionBuilder_addInstABx(
   builder(),
   BIFROST_VM_OP_STORE_MOVE,
   expr->write_loc,
   temp_first);

  bfVMFunctionBuilder_popTemp(builder(), subscript_op_loc);
}

static void parseDotOp(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec)
{
  // token               = Grammar Rule Association.
  // self->current_token = rhs token
  // expr->token         = lhs token

  static bfBool32 isRightAssoc = bfTrue;

  if (self->current_token.type == IDENTIFIER)
  {
    const bfToken field = self->current_token;
    const size_t  sym   = parserGetSymbol(self, field.as.str_range);

    bfVMFunctionBuilder_addInstABC(
     builder(),
     BIFROST_VM_OP_LOAD_SYMBOL,
     expr->write_loc,
     lhs->write_loc,
     (uint16_t)sym);

    const VariableInfo lhs_var = lhs->var;

    *expr              = exprMakeTemp(expr->write_loc);
    expr->var.location = expr->write_loc;

    parseExpr(self, expr, prec - (isRightAssoc ? 1 : 0));

    if (bfParser_match(self, BIFROST_TOKEN_EQUALS))
    {
      const uint16_t rhs_loc = bfVMFunctionBuilder_pushTemp(builder(), 2);
      const uint16_t var_loc = rhs_loc + 1;

      ExprInfo rhs_expr = exprMakeTemp(rhs_loc);

      parseExpr(self, &rhs_expr, PREC_ASSIGN);

      parserVariableLoad(self, lhs_var, var_loc);

      bfVMFunctionBuilder_addInstABC(
       builder(),
       BIFROST_VM_OP_STORE_SYMBOL,
       var_loc,
       (uint16_t)sym,
       rhs_expr.write_loc);

      bfVMFunctionBuilder_popTemp(builder(), rhs_loc);
    }
  }
  else
  {
    bfEmitError(self, "(%s) ERROR: Cannot use the dot operator on non variables.\n", tokentypeToString(token->type));
  }
}

static void parseAssign(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec)
{
  (void)expr;
  (void)token;

  const uint16_t rhs_loc  = bfVMFunctionBuilder_pushTemp(builder(), 1);
  ExprInfo       rhs_expr = exprMakeTemp(rhs_loc);
  parseExpr(self, &rhs_expr, prec);
  parserVariableStore(self, lhs->var, rhs_loc);
  bfVMFunctionBuilder_popTemp(builder(), rhs_loc);
}

static void parseCall(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec)
{
  (void)expr;
  (void)lhs;
  (void)token;
  (void)prec;

  const uint16_t function_loc      = bfVMFunctionBuilder_pushTemp(builder(), 1);
  const uint16_t real_function_loc = lhs->var.kind == V_LOCAL ? lhs->var.location : function_loc;

  if (lhs->var.kind != V_LOCAL)
  {
    parserVariableLoad(self, lhs->var, function_loc);
  }

  const VariableInfo function_var = parserVariableMakeTemp(real_function_loc);
  const VariableInfo return_var   = parserVariableMakeTemp(expr->write_loc);

  parserFinishCall(self, function_var, return_var, BIFROST_VM_INVALID_SLOT);

  bfVMFunctionBuilder_popTemp(builder(), function_loc);
}

static void parseMethodCall(BifrostParser* self, ExprInfo* expr, const ExprInfo* lhs, const bfToken* token, int prec)
{
  (void)self;
  (void)expr;
  (void)lhs;
  (void)token;
  (void)prec;

  const bfStringRange method_name = self->current_token.as.str_range;

  bfParser_eat(self, IDENTIFIER, bfFalse, "Function call must be done on an identifier.");

  const uint16_t function_loc = bfVMFunctionBuilder_pushTemp(builder(), 2);
  const uint16_t var_loc      = function_loc + 1;
  const size_t   sym          = parserGetSymbol(self, method_name);
  const uint16_t real_var_loc = (lhs->var.kind == V_LOCAL) ? lhs->var.location : var_loc;

  if (lhs->var.kind != V_LOCAL)
  {
    parserVariableLoad(self, lhs->var, var_loc);
  }

  bfVMFunctionBuilder_addInstABC(
   builder(),
   BIFROST_VM_OP_LOAD_SYMBOL,
   function_loc,
   real_var_loc,
   (uint16_t)sym);

  const VariableInfo function_var = parserVariableMakeTemp(function_loc);
  const VariableInfo return_var   = parserVariableMakeTemp(expr->write_loc);

  bfParser_eat(self, BIFROST_TOKEN_L_PAREN, bfFalse, "Function call must start with an open parenthesis.");
  parserFinishCall(self, function_var, return_var, real_var_loc);

  bfVMFunctionBuilder_popTemp(builder(), function_loc);
}

static bfBool32 parserIsConstexpr(const BifrostParser* self)
{
  switch (self->current_token.type)
  {
    case CONST_REAL:
    case CONST_BOOL:
    case CONST_STR:
    case CONST_NIL:
      return bfTrue;
    default:
      return bfFalse;
  }
}

static bfVMValue parserConstexprValue(const BifrostParser* self)
{
  return parserTokenConstexprValue(self, &self->current_token);
}

static bfVMValue parserTokenConstexprValue(const BifrostParser* self, const bfToken* token)
{
  switch (token->type)
  {
    case CONST_REAL:
      return FROM_NUMBER(token->as.num);
    case CONST_BOOL:
      return token->as.str[0] == 't' ? VAL_TRUE : VAL_FALSE;
    case CONST_STR:
      return FROM_POINTER(bfVM_createString(self->vm, token->as.str_range));
    case CONST_NIL:
      return VAL_NULL;
    default:
      break;
  }

  assert(!"parserConstexprValue called on non constexpr token.");
  return 0;
}

static uint32_t parserGetSymbol(const BifrostParser* self, bfStringRange name)
{
  return bfVM_getSymbol(self->vm, name);
}

static bfStringRange parserBeginFunction(BifrostParser* self, bfBool32 require_name)
{
  bfStringRange name_str = bfMakeStringRangeC("__INVALID__");

  if (bfParser_is(self, BIFROST_TOKEN_IDENTIFIER))
  {
    name_str = self->current_token.as.str_range;
    bfParser_eat(self, BIFROST_TOKEN_IDENTIFIER, bfFalse, "Function name expected after 'func' keyword.");
  }
  else if (bfParser_is(self, BIFROST_TOKEN_L_SQR_BOI))
  {
    name_str.bgn = "[]";
    name_str.end = name_str.bgn + 2;

    bfParser_eat(self, BIFROST_TOKEN_L_SQR_BOI, bfFalse, "");
    bfParser_eat(self, BIFROST_TOKEN_R_SQR_BOI, bfFalse, "Closing square bracket must be after opening for function decl.");

    if (bfParser_match(self, BIFROST_TOKEN_EQUALS))
    {
      name_str.bgn = "[]=";
      name_str.end = name_str.bgn + 3;
    }
  }
  else if (!require_name)
  {
    name_str.bgn = name_str.end = NULL;
  }
  else
  {
    bfEmitError(self, "Function name or \"[]\" or \"[]=\" expected after 'func' keyword.");
  }

  bfParser_pushBuilder(self, name_str.bgn, bfStringRange_length(&name_str));
  return name_str;
}

static int parserParseFunction(BifrostParser* self)
{
  /* GRAMMAR(Shareef):
      func <identifier>(<identifier>,...) {}
      func <identifier>() {}
  */

  int arity = 0;
  bfParser_eat(self, BIFROST_TOKEN_L_PAREN, bfFalse, "Expected parameter list after function name.");

  while (!bfParser_is(self, BIFROST_TOKEN_R_PAREN))
  {
    const bfStringRange param_str = self->current_token.as.str_range;
    bfVMFunctionBuilder_declVariable(builder(), param_str.bgn, bfStringRange_length(&param_str));

    bfParser_eat(self, IDENTIFIER, bfFalse, "Parameter names must be a word and not a keyword.");
    bfParser_eat(self, COMMA, bfTrue, "The last comma isn't needed but this allows a func (a b c) syntax.");

    ++arity;
  }

  bfParser_eat(self, R_PAREN, bfFalse, "@TODO Error message");
  parseBlock(self);
  bfParser_match(self, BIFROST_TOKEN_SEMI_COLON);
  return arity;
}

static void parserEndFunction(BifrostParser* self, BifrostObjFn* out, int arity)
{
  bfParser_popBuilder(self, out, arity);
}

static void parseClassDecl(BifrostParser* self)
{
  /* GRAMMAR(Shareef):
      class <identifier> : <identifier> { <class-decls>... || <none> };
      class <identifier> { <class-decls>... || <none> };
  */

  const bfToken       name_token = self->current_token;
  const bfStringRange name_str   = name_token.as.str_range;

  bfParser_eat(
   self,
   BIFROST_TOKEN_IDENTIFIER,
   bfFalse,
   "Class name expected after 'class' keyword");

  bfParser_eat(
   self,
   BIFROST_TOKEN_L_CURLY,
   bfFalse,
   "Class definition must start with a curly brace.");

  BifrostObjClass* clz = bfVM_createClass(self->vm, self->current_module, name_str, 0u);

  bfGCPushRoot(self->vm, &clz->super);
  bfVM_xSetVariable(&self->current_module->variables, self->vm, name_str, FROM_POINTER(clz));

  self->current_clz = clz;

  while (!bfParser_is(self, BIFROST_TOKEN_R_CURLY))
  {
    if (bfParser_match(self, BIFROST_TOKEN_VAR_DECL))
    {
      parseClassVarDecl(self, clz, bfFalse);
    }
    else if (bfParser_match(self, BIFROST_TOKEN_FUNC_DECL))
    {
      parseClassFunc(self, clz, bfFalse);
    }
    else if (bfParser_match(self, BIFROST_TOKEN_STATIC))
    {
      if (bfParser_match(self, BIFROST_TOKEN_FUNC_DECL))
      {
        parseClassFunc(self, clz, bfTrue);
      }
      else if (bfParser_match(self, BIFROST_TOKEN_VAR_DECL))
      {
        parseClassVarDecl(self, clz, bfTrue);
      }
      else
      {
        bfEmitError(self, "'static' keyword must be followed by either a function or variable declaration.");
      }
    }
    else
    {
      bfEmitError(self, "Invalid declaration in class. Currently only 'var' and 'func' are supported.");
      bfParser_parse(self);
    }
  }

  bfGCPopRoot(self->vm);

  self->current_clz = NULL;

  bfParser_eat(self, BIFROST_TOKEN_R_CURLY, bfFalse, "Class definition must end with a curly brace.");
  bfParser_eat(self, BIFROST_TOKEN_SEMI_COLON, bfFalse, "Class definition must have a semi colon at the end.");
}

static void parseClassVarDecl(BifrostParser* self, BifrostObjClass* clz, bfBool32 is_static)
{
  /* GRAMMAR(Shareef):
      var <identifier> = <constexpr>;
      var <identifier>;
  */

  const bfStringRange name_str = self->current_token.as.str_range;

  bfParser_eat(self, BIFROST_TOKEN_IDENTIFIER, bfFalse, "Expected name after var keyword.");

  bfVMValue initial_value = VAL_NULL;

  if (bfParser_match(self, BIFROST_TOKEN_EQUALS))
  {
    if (parserIsConstexpr(self))
    {
      initial_value = parserConstexprValue(self);
      bfParser_match(self, self->current_token.type);
    }
    else
    {
      bfEmitError(self, "Variable initializer must be a constant expression.");
    }
  }

  if (is_static)
  {
    bfVM_xSetVariable(&clz->symbols, self->vm, name_str, initial_value);
  }
  else
  {
    const size_t     symbol   = parserGetSymbol(self, name_str);
    BifrostVMSymbol* var_init = Array_emplace(&clz->field_initializers);
    var_init->name            = self->vm->symbols[symbol];
    var_init->value           = initial_value;
  }

  bfParser_eat(self, BIFROST_TOKEN_SEMI_COLON, bfFalse, "Expected semi-colon after variable declaration.");
}

static void parseClassFunc(BifrostParser* self, BifrostObjClass* clz, bfBool32 is_static)
{
  const bfStringRange name_str = parserBeginFunction(self, bfTrue);
  int                 arity    = !is_static;

  if (!is_static)
  {
    bfVMFunctionBuilder_declVariable(builder(), "self", 4);
  }

  arity += parserParseFunction(self);

  // TODO(Shareef): This same line is used in 3 (or more) places and should be put in a helper.
  BifrostObjFn* fn = bfVM_createFunction(self->vm, self->current_module);
  bfVM_xSetVariable(&clz->symbols, self->vm, name_str, FROM_POINTER(fn));
  parserEndFunction(self, fn, arity);
}

static uint32_t parserMakeJump(BifrostParser* self)
{
  const uint32_t jump_idx = (uint32_t)Array_size(&self->fn_builder->instructions);
  bfVMFunctionBuilder_addInstAsBx(self->fn_builder, BIFROST_VM_OP_JUMP, 0, 0);
  return jump_idx;
}

static uint32_t parserMakeJumpRev(BifrostParser* self)
{
  return (uint32_t)Array_size(&self->fn_builder->instructions);
}

static inline void parserPatchJumpHelper(BifrostParser* self, uint32_t jump_idx, uint32_t cond_var, int jump_amt, bfBool32 if_not)
{
  bfInstruction* const inst = self->fn_builder->instructions + jump_idx;

  if (cond_var == BIFROST_VM_INVALID_SLOT)
  {
    *inst = BIFROST_MAKE_INST_OP_AsBx(
     BIFROST_VM_OP_JUMP,
     0,
     jump_amt);
  }
  else
  {
    *inst = BIFROST_MAKE_INST_OP_AsBx(
     (if_not ? BIFROST_VM_OP_JUMP_IF_NOT : BIFROST_VM_OP_JUMP_IF),
     cond_var,
     jump_amt);
  }
}

static void parserPatchJump(BifrostParser* self, uint32_t jump_idx, uint32_t cond_var, bfBool32 if_not)
{
  const uint32_t current_loc = (uint32_t)Array_size(&self->fn_builder->instructions);
  parserPatchJumpHelper(self, jump_idx, cond_var, (int)current_loc - (int)jump_idx, if_not);
}

static void parserPatchJumpRev(BifrostParser* self, uint32_t jump_idx, uint32_t cond_var, bfBool32 if_not)
{
  const uint32_t current_loc = (uint32_t)Array_size(&self->fn_builder->instructions);
  bfVMFunctionBuilder_addInstAsBx(self->fn_builder, BIFROST_VM_OP_JUMP, 0, 0);
  parserPatchJumpHelper(self, current_loc, cond_var, (int)jump_idx - (int)current_loc, if_not);
}

static VariableInfo parserVariableFindLocal(const BifrostParser* self, bfStringRange name)
{
  VariableInfo var;
  var.kind     = V_LOCAL;
  var.location = (uint16_t)(bfVMFunctionBuilder_getVariable(self->fn_builder, name.bgn, bfStringRange_length(&name)) & 0xFFFF);

  return var;
}

static VariableInfo parserVariableFind(const BifrostParser* self, bfStringRange name)
{
  VariableInfo var = parserVariableFindLocal(self, name);

  if (var.location == BIFROST_VM_INVALID_SLOT)
  {
    var.kind     = V_MODULE;
    var.location = (uint16_t)parserGetSymbol(self, name);
  }

  return var;
}

static VariableInfo parserVariableMakeLocal(const BifrostParser* self, bfStringRange name)
{
  VariableInfo var;

  var.kind     = V_LOCAL;
  var.location = (uint16_t)bfVMFunctionBuilder_declVariable(builder(), name.bgn, bfStringRange_length(&name));

  return var;
}

static void parserVariableLoad(BifrostParser* self, VariableInfo variable, uint16_t write_loc)
{
  assert(variable.location != BIFROST_VM_INVALID_SLOT);
  assert(write_loc != BIFROST_VM_INVALID_SLOT);

  switch (variable.kind)
  {
    case V_LOCAL:
    {
      /* @Optimization:
           Removal of a redundant store.
       */
      if (write_loc != variable.location)
      {
        bfVMFunctionBuilder_addInstABx(
         builder(),
         BIFROST_VM_OP_STORE_MOVE,
         write_loc,
         variable.location);
      }
      break;
    }
    case V_MODULE:
    {
      const uint32_t module_expr = bfVMFunctionBuilder_pushTemp(builder(), 1);

      bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, module_expr, BIFROST_VM_OP_LOAD_BASIC_CURRENT_MODULE);
      bfVMFunctionBuilder_addInstABC(
       builder(),
       BIFROST_VM_OP_LOAD_SYMBOL,
       write_loc,
       module_expr,
       variable.location);

      bfVMFunctionBuilder_popTemp(builder(), module_expr);
      break;
    }
    default:
    {
      assert(!"parserLoadVariable invalid variable type.");
      break;
    }
  }
}

static void parserVariableStore(BifrostParser* self, VariableInfo variable, uint32_t read_loc)
{
  assert(variable.location != BIFROST_VM_INVALID_SLOT);
  assert(read_loc != BIFROST_VM_INVALID_SLOT);

  switch (variable.kind)
  {
    case V_LOCAL:
    {
      bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_STORE_MOVE, variable.location, read_loc);
      break;
    }
    case V_MODULE:
    {
      const uint32_t module_expr = bfVMFunctionBuilder_pushTemp(builder(), 1);

      bfVMFunctionBuilder_addInstABx(builder(), BIFROST_VM_OP_LOAD_BASIC, module_expr, BIFROST_VM_OP_LOAD_BASIC_CURRENT_MODULE);
      bfVMFunctionBuilder_addInstABC(
       builder(),
       BIFROST_VM_OP_STORE_SYMBOL,
       module_expr,
       variable.location,
       read_loc);

      bfVMFunctionBuilder_popTemp(builder(), module_expr);
      break;
    }
    default:
    {
      assert(!"parserLoadVariable invalid variable type.");
      break;
    }
  }
}

static VariableInfo parserVariableMakeTemp(uint16_t temp_loc)
{
  VariableInfo ret;
  ret.location = temp_loc;
  ret.kind     = V_LOCAL;
  return ret;
}

static ExprInfo exprMake(uint16_t write_loc, VariableInfo variable)
{
  ExprInfo ret;
  ret.write_loc = write_loc;
  ret.token     = BIFROST_TOKEN_MAKE_STR(EOP, "EOP");
  ret.var       = variable;
  return ret;
}

static ExprInfo exprMakeTemp(uint16_t temp_loc)
{
  return exprMake(temp_loc, parserVariableMakeTemp(BIFROST_VM_INVALID_SLOT));
}

static int parserCallArgs(BifrostParser* self, uint16_t temp_first, int num_params, bfTokenType end_token)
{
  if (!bfParser_is(self, end_token))
  {
    do
    {
      const uint16_t param_loc  = num_params == 0 ? temp_first : bfVMFunctionBuilder_pushTemp(builder(), 1);
      ExprInfo       param_expr = exprMakeTemp(param_loc);

      parseExpr(self, &param_expr, PREC_NONE);

      ++num_params;
    } while (bfParser_match(self, BIFROST_TOKEN_COMMA));
  }

  return num_params;
}

static void parserFinishCall(BifrostParser* self, VariableInfo fn, VariableInfo return_loc, uint32_t zero_slot)
{
  const bfBool32 is_local_fn  = fn.kind == V_LOCAL;
  const uint16_t function_loc = is_local_fn ? fn.location : bfVMFunctionBuilder_pushTemp(builder(), 1);

  if (!is_local_fn)
  {
    parserVariableLoad(self, fn, function_loc);
  }

  int            num_params = 0;
  const uint16_t temp_first = bfVMFunctionBuilder_pushTemp(builder(), 1);

  if (zero_slot != BIFROST_VM_INVALID_SLOT)
  {
    bfVMFunctionBuilder_addInstABx(
     builder(),
     BIFROST_VM_OP_STORE_MOVE,
     temp_first,
     zero_slot);
    ++num_params;
  }

  num_params = parserCallArgs(self, temp_first, num_params, BIFROST_TOKEN_R_PAREN);
  bfParser_eat(self, BIFROST_TOKEN_R_PAREN, bfFalse, "Function call must end with a closing parenthesis.");

  bfVMFunctionBuilder_addInstABC(
   builder(),
   BIFROST_VM_OP_CALL_FN,
   temp_first,
   function_loc,
   num_params);

  if (return_loc.location != BIFROST_VM_INVALID_SLOT)
  {
    parserVariableStore(self, return_loc, temp_first);
  }

  bfVMFunctionBuilder_popTemp(builder(), (is_local_fn ? temp_first : function_loc));
}

static void parseBlock(BifrostParser* self)
{
  // TODO(SR): Add one liner blocks?
  bfParser_eat(self, BIFROST_TOKEN_L_CURLY, bfFalse, "Block must start with an opening curly boi.");

  bfVMFunctionBuilder_pushScope(builder());

  while (!bfParser_is(self, BIFROST_TOKEN_R_CURLY))
  {
    if (!bfParser_parse(self))
    {
      break;
    }
  }

  bfVMFunctionBuilder_popScope(builder());

  bfParser_eat(self, BIFROST_TOKEN_R_CURLY, bfFalse, "Block must end with an closing curly boi.");
}
