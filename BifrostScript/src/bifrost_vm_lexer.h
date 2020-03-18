/******************************************************************************/
/*!
 * @file   bifrost_vm_lexer.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Tokenizing helpers for strings.
 *
 * @version 0.0.1
 * @date    2019-06-08
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
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
  BIFROST_TOKEN_L_PAREN     = 0,  /*!< (                                     */
  BIFROST_TOKEN_R_PAREN     = 1,  /*!< )                                     */
  BIFROST_TOKEN_L_SQR_BOI   = 2,  /*!< [                                     */
  BIFROST_TOKEN_R_SQR_BOI   = 3,  /*!< ]                                     */
  BIFROST_TOKEN_L_CURLY     = 4,  /*!< {                                     */
  BIFROST_TOKEN_R_CURLY     = 5,  /*!< }                                     */
  BIFROST_TOKEN_COLON       = 7,  /*!< :                                     */
  BIFROST_TOKEN_SEMI_COLON  = 8,  /*!< ;                                     */
  BIFROST_TOKEN_COMMA       = 9,  /*!< ,                                     */
  BIFROST_TOKEN_EQUALS      = 10, /*!< =                                     */
  BIFROST_TOKEN_DOT         = 19, /*!< .                                     */
  BIFROST_TOKEN_IDENTIFIER  = 20, /*!< abcdefghijklmnopqrstuvwxyz_0123456789 */
  BIFROST_TOKEN_VAR_DECL    = 21, /*!< var                                   */
  BIFROST_TOKEN_FUNC_DECL   = 23, /*!< func                                  */
  BIFROST_TOKEN_CTRL_IF     = 25, /*!< if                                    */
  BIFROST_TOKEN_CTRL_ELSE   = 26, /*!< else                                  */
  BIFROST_TOKEN_CTRL_WHILE  = 36, /*!< while                                 */
  BIFROST_TOKEN_CTRL_RETURN = 38, /*!< return                                */
  BIFROST_TOKEN_CTRL_BREAK  = 45, /*!< break                                 */
  BIFROST_TOKEN_NEW         = 46, /*!< new                                   */
  BIFROST_TOKEN_STATIC      = 47, /*!< static                                */
  BIFROST_TOKEN_AS          = 48, /*!< as                                    */
  BIFROST_TOKEN_SUPER       = 49, /*!< super                                 */

  // Deprecated names
  R_PAREN   = BIFROST_TOKEN_R_PAREN,
  L_SQR_BOI = BIFROST_TOKEN_L_SQR_BOI,
  R_SQR_BOI = BIFROST_TOKEN_R_SQR_BOI,
  L_CURLY   = BIFROST_TOKEN_L_CURLY,
  R_CURLY   = BIFROST_TOKEN_R_CURLY,
  HASHTAG   = 6,                    // #
  COLON     = BIFROST_TOKEN_COLON,  // @BIFROST_TOKEN_COLON
  COMMA     = BIFROST_TOKEN_COMMA,  // @BIFROST_TOKEN_COMMA
  EQUALS    = 10,                   // @BIFROST_TOKEN_EQUALS
  PLUS      = 11,                   // +
  MINUS     = 12,                   // -
  MULT,                             // *
  DIV,                              // /
  PLUS_EQUALS,                      // +=
  MINUS_EQUALS,                     // -=
  INCREMENT,                        // ++
  DECREMENT,                        // --
  DOT,                              // @BIFROST_TOKEN_DOT
  IDENTIFIER,                       // @BIFROST_TOKEN_IDENTIFIER
  VAR_DECL,                         // var
  IMPORT,                           // import
  FUNC,                             // func
  BIFROST_TOKEN_CLASS,              // class
  CTRL_IF,                          // @BIFROST_TOKEN_CTRL_IF
  CTRL_ELSE,                        // @BIFROST_TOKEN_CTRL_ELSE
  CTRL_EE,                          // ==
  CTRL_LT,                          // <
  CTRL_GT,                          // >
  CTRL_LE,                          // <=
  CTRL_GE,                          // >=
  CTRL_OR,                          // ||
  CTRL_AND,                         // &&
  CTRL_NE,                          // !=
  CTRL_FOR    = 37,                 // for
  CTRL_RETURN = 38,                 // return
  CTRL_NEGATE = 39,                 // !
  CONST_STR   = 40,                 // "<...>"
  CONST_REAL  = 41,                 // 01234567890.0123456789
  CONST_BOOL  = 42,                 // true, false
  CONST_NIL   = 43,                 // nil
  EOP         = 44,                 // End of Program

  // basics, keywords, meta, literals, literals, flow control
  // TODO(SR): Tokens: '/=', '*=', '%', '%=', '|', '&', '~', '>>', '<<'

} bfTokenType;

// TODO(Shareef): Consider passing by copy.
// This is in this file because of the "size_t".
static size_t bfStringRange_length(const bfStringRange* self)
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

// TODO(SR): Move To Debug Header
const char* tokentypeToString(bfTokenType t);
void        printToken(const bfToken* token);
#if __cplusplus
}
#endif

#endif /* BIFROST_VM_LEXER_H */