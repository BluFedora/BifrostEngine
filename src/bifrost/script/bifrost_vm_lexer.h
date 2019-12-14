#ifndef BIFROST_VM_LEXER_H
#define BIFROST_VM_LEXER_H

#include "bifrost_vm_value.h" /* bfVMNumberT */
#include <stddef.h>           /* size_t      */

#if __cplusplus
extern "C" {
#endif
/*!
 * @brief
 *    These are all of the token types that are used by the lexer.
 */
typedef enum
{
  R_PAREN   = 1,        // @BIFROST_TOKEN_R_PAREN
  L_SQR_BOI = 2,        // @BIFROST_TOKEN_L_SQR_BOI
  R_SQR_BOI = 3,        // @BIFROST_TOKEN_R_SQR_BOI
  L_CURLY   = 4,        // @BIFROST_TOKEN_L_CURLY
  R_CURLY   = 5,        // @BIFROST_TOKEN_R_CURLY
  HASHTAG   = 6,        // #
  COLON     = 7,        // :
  COMMA     = 9,        // @BIFROST_TOKEN_COMMA
  EQUALS    = 10,       // @BIFROST_TOKEN_EQUALS
  PLUS      = 11,       // +
  MINUS     = 12,       // -
  MULT,                 // *
  DIV,                  // /
  PLUS_EQUALS,          // +=
  MINUS_EQUALS,         // -=
  INCREMENT,            // ++
  DECREMENT,            // --
  DOT,                  // @BIFROST_TOKEN_DOT
  IDENTIFIER,           // @BIFROST_TOKEN_IDENTIFIER
  VAR_DECL,             // var
  IMPORT,               // import
  FUNC,                 // func
  BIFROST_TOKEN_CLASS,  // class
  BIFROST_TOKEN_PRINT,  // print
  CTRL_IF,              // @BIFROST_TOKEN_CTRL_IF
  CTRL_ELSE,            // @BIFROST_TOKEN_CTRL_ELSE
  CTRL_EE,              // ==
  CTRL_LT,              // <
  CTRL_GT,              // >
  CTRL_LE,              // <=
  CTRL_GE,              // >=
  CTRL_OR,              // ||
  CTRL_AND,             // &&
  CTRL_NE,              // !=
  CTRL_FOR    = 37,     // for
  CTRL_RETURN = 38,     // return
  CTRL_NEGATE = 39,     // !
  CONST_STR   = 40,     // "<...>"
  CONST_REAL  = 41,     // 01234567890.0123456789
  CONST_BOOL  = 42,     // true, false
  CONST_NIL   = 43,     // nil
  EOP         = 44,     // End of Program

  // basics, keywords, meta, literals, literals, flow control

  // TODO(SR): Tokens: '/=', '*=', '%', '%=', '|', '&', '~', '>>', '<<'

  BIFROST_TOKEN_L_PAREN     = 0,           /* (                                     */
  BIFROST_TOKEN_R_PAREN     = R_PAREN,     /* )                                     */
  BIFROST_TOKEN_L_SQR_BOI   = L_SQR_BOI,   /* [                                     */
  BIFROST_TOKEN_R_SQR_BOI   = R_SQR_BOI,   /* ]                                     */
  BIFROST_TOKEN_L_CURLY     = L_CURLY,     /* {                                     */
  BIFROST_TOKEN_R_CURLY     = R_CURLY,     /* }                                     */
  BIFROST_TOKEN_SEMI_COLON  = 8,           /* ;                                     */
  BIFROST_TOKEN_EQUALS      = EQUALS,      /* =                                     */
  BIFROST_TOKEN_DOT         = DOT,         /* .                                     */
  BIFROST_TOKEN_COMMA       = COMMA,       /* ,                                     */
  BIFROST_TOKEN_IDENTIFIER  = IDENTIFIER,  /* abcdefghijklmnopqrstuvwxyz_0123456789 */
  BIFROST_TOKEN_VAR_DECL    = VAR_DECL,    /* var                                   */
  BIFROST_TOKEN_FUNC_DECL   = FUNC,        /* func                                  */
  BIFROST_TOKEN_CTRL_WHILE  = 36,          /* while                                 */
  BIFROST_TOKEN_CTRL_IF     = CTRL_IF,     /* if                                    */
  BIFROST_TOKEN_CTRL_ELSE   = CTRL_ELSE,   /* else                                  */
  BIFROST_TOKEN_CTRL_RETURN = CTRL_RETURN, /* return                                */
  BIFROST_TOKEN_CTRL_BREAK  = EOP + 1,     /* break                                 */
  BIFROST_TOKEN_NEW         = EOP + 2,     /* new                                   */
  BIFROST_TOKEN_STATIC      = EOP + 3,     /* static                                */
  BIFROST_TOKEN_AS          = EOP + 4,     /* as                                    */
  BIFROST_TOKEN_SUPER       = EOP + 5,     /* super                                 */

} bfTokenType;

// TODO(Shareef): Consider passing by copy.
// This is in this file because of the "size_t".
static inline size_t bfStringRange_length(const bfStringRange* self)
{
  return self->end - self->bgn;
}

/*!
 * @brief
 *    An individual token for a bts program.
 */
typedef struct
{
  bfTokenType type;

  union
  {
    bfStringRange str_range;
    const char*   str;
    bfVMNumberT   num;

  } as;

} bfToken;

typedef struct
{
  const char* keyword;
  size_t      length;
  bfToken     value;

} bfKeyword;

struct BifrostVM_t;

typedef struct
{
  const char*         source;
  size_t              length;
  const bfKeyword*    keywords;
  size_t              num_keywords;
  struct BifrostVM_t* vm;

} BifrostLexerParams;

typedef struct BifrostLexer_t
{
  const char*         source_bgn;
  const char*         source_end;
  const bfKeyword*    keywords;
  size_t              num_keywords;
  size_t              cursor;
  size_t              current_line_no;
  size_t              line_pos_bgn;
  size_t              line_pos_end;
  struct BifrostVM_t* vm;

} BifrostLexer;

BifrostLexer bfLexer_make(const BifrostLexerParams* params);
void         bfLexer_reset(BifrostLexer* self);
bfToken      bfLexer_nextToken(BifrostLexer* self);
char         bfLexer_peek(const BifrostLexer* self, size_t amt);
const char*  bfLexer_peekStr(const BifrostLexer* self, size_t amt);
bfBool32     bfLexer_isWhitespace(char c);
bfBool32     bfLexer_isNewline(char c);
void         bfLexer_skipWhile(BifrostLexer* self, bfBool32 (*condition)(char c));
void         bfLexer_skipWhitespace(BifrostLexer* self);
void         bfLexer_skipLineComment(BifrostLexer* self);
void         bfLexer_skipBlockComment(BifrostLexer* self);
void         bfLexer_advance(BifrostLexer* self, size_t amt);
bfBool32     bfLexer_isDigit(char c);
bfBool32     bfLexer_isFollowedByDigit(BifrostLexer* self, char c, char m);
bfToken      bfLexer_parseNumber(BifrostLexer* self);
bfBool32     bfLexer_isID(char c);
bfToken      bfLexer_parseID(BifrostLexer* self);
bfToken      bfLexer_parseString(BifrostLexer* self);

#define BIFROST_TOKEN_MAKE_STR(t, s) \
  (bfToken)                          \
  {                                  \
    .type = t, .as = {.str = s }     \
  }
#define BIFROST_TOKEN_MAKE_STR2(t, s, e)    \
  (bfToken)                                 \
  {                                         \
    .type = t, .as = {.str_range = {s, e} } \
  }
#define BIFROST_TOKEN_MAKE_NUM(t, v) \
  (bfToken)                          \
  {                                  \
    .type = t, .as = {.num = v }     \
  }

// TODO(SR): Move To Debug HEadere
const char* tokentypeToString(bfTokenType t);
void        printToken(const bfToken* token);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_LEXER_H */