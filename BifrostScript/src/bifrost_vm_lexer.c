/******************************************************************************/
/*!
 * @file   bifrost_vm_lexer.c
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
#include "bifrost_vm_lexer.h"

#include <ctype.h>  /* isdigit, isalpha, isspace */
#include <stdlib.h> /* strtof                    */
#include <string.h> /* strncmp                   */

#define BTS_COMMENT_CHARACTER '/'

BifrostLexer bfLexer_make(const BifrostLexerParams* params)
{
  BifrostLexer self;
  self.source_bgn   = params->source;
  self.source_end   = params->source + params->length;
  self.keywords     = params->keywords;
  self.num_keywords = params->num_keywords;
  self.vm           = params->vm;
  self.do_comments  = params->do_comments;
  bfLexer_reset(&self);
  return self;
}

void bfLexer_reset(BifrostLexer* self)
{
  self->cursor          = 0;
  self->current_line_no = 1;
  self->line_pos_bgn    = 0;
  self->line_pos_end    = 0;
  bfLexer_advance(self, 0);
}

bfStringRange bfLexer_currentLine(BifrostLexer* self)
{
  return (bfStringRange){
   .bgn = self->source_bgn + self->line_pos_bgn,
   .end = self->source_bgn + self->line_pos_end,
  };
}

bfToken bfLexer_nextToken(BifrostLexer* self)
{
  char current_char = bfLexer_peek(self, 0);

  while (current_char != '\0')
  {
    current_char = bfLexer_peek(self, 0);

    if (bfLexer_isWhitespace(current_char))
    {
      bfLexer_skipWhitespace(self);
      continue;
    }

    if (current_char == BTS_COMMENT_CHARACTER)
    {
      const char next_char = bfLexer_peek(self, 1);

      if (next_char == BTS_COMMENT_CHARACTER && self->do_comments)
      {
        bfLexer_skipLineComment(self);
      }
      else if (next_char == '*' && self->do_comments)
      {
        bfLexer_skipBlockComment(self);
      }
      else
      {
        bfLexer_advance(self, 1);
        return BIFROST_TOKEN_MAKE_STR(DIV, "/");
      }
      continue;
    }

    if (bfLexer_isDigit(current_char) || bfLexer_isFollowedByDigit(self, current_char, '.'))
    {
      return bfLexer_parseNumber(self);
    }

    if (bfLexer_isID(current_char))
    {
      return bfLexer_parseID(self);
    }

    if (current_char == '"')
    {
      return bfLexer_parseString(self);
    }

    bfLexer_advance(self, 1);
    const char next_char = bfLexer_peek(self, 0);

    switch (current_char)
    {
      case '[': return BIFROST_TOKEN_MAKE_STR(L_SQR_BOI, "[");
      case ']': return BIFROST_TOKEN_MAKE_STR(R_SQR_BOI, "]");
      case '(': return BIFROST_TOKEN_MAKE_STR(BIFROST_TOKEN_L_PAREN, "(");
      case ')': return BIFROST_TOKEN_MAKE_STR(R_PAREN, ")");
      case ':': return BIFROST_TOKEN_MAKE_STR(COLON, ":");
      case ';': return BIFROST_TOKEN_MAKE_STR(BIFROST_TOKEN_SEMI_COLON, ";");
      case '{': return BIFROST_TOKEN_MAKE_STR(L_CURLY, "{");
      case '}': return BIFROST_TOKEN_MAKE_STR(R_CURLY, "}");
      case ',': return BIFROST_TOKEN_MAKE_STR(COMMA, ",");
      case '.': return BIFROST_TOKEN_MAKE_STR(DOT, ".");

      case '<':
      {
        if (next_char == '=')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(CTRL_LE, "<=");
        }

        return BIFROST_TOKEN_MAKE_STR(CTRL_LT, "<");
      }
      case '>':
      {
        if (next_char == '=')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(CTRL_GE, ">=");
        }

        return BIFROST_TOKEN_MAKE_STR(CTRL_GT, ">");
      }
      case '=':
      {
        if (next_char == '=')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(CTRL_EE, "==");
        }

        return BIFROST_TOKEN_MAKE_STR(EQUALS, "=");
      }
      case '+':
      {
        if (next_char == '=')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(PLUS_EQUALS, "+=");
        }

        return BIFROST_TOKEN_MAKE_STR(PLUS, "+");
      }
      case '-':
      {
        if (next_char == '=')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(MINUS_EQUALS, "-=");
        }

        return BIFROST_TOKEN_MAKE_STR(MINUS, "-");
      }
      case '*':
      {
        return BIFROST_TOKEN_MAKE_STR(MULT, "*");
      }
      case '/':
      {
        return BIFROST_TOKEN_MAKE_STR(DIV, "/");
      }
      case '!':
      {
        if (next_char == '=')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(CTRL_NE, "!=");
        }

        return BIFROST_TOKEN_MAKE_STR(CTRL_NEGATE, "!");
      }
      case '|':
      {
        if (next_char == '|')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(CTRL_OR, "||");
        }

        return BIFROST_TOKEN_MAKE_STR(CTRL_OR, "|");
      }
      case '&':
      {
        if (next_char == '&')
        {
          bfLexer_advance(self, 1);
          return BIFROST_TOKEN_MAKE_STR(CTRL_AND, "&&");
        }

        return BIFROST_TOKEN_MAKE_STR(CTRL_AND, "&");
      }
      case '#':
      {
        return BIFROST_TOKEN_MAKE_STR(HASHTAG, "#");
      }
      case '@':
      {
        return BIFROST_TOKEN_MAKE_STR(BIFROST_TOKEN_AT_SIGN, "@");
      }
      case '\0':
      {
        break;
      }
      default:
      {
        const bfErrorFn err_fn = self->vm ? self->vm->params.error_fn : NULL;

        if (err_fn)
        {
          const bfStringRange line = bfLexer_currentLine(self);

          char buffer[512];

          const int num_bytes = sprintf(
           buffer,
           "Invalid character ('%c') on line %u \"%.*s\"",
           current_char,
           (unsigned int)self->current_line_no,
           (int)bfStringRange_length(&line),
           line.bgn);

          assert(num_bytes < (int)sizeof(buffer) && "Oops Buffer Overflow");

          err_fn(self->vm, BIFROST_VM_ERROR_LEXER, (int)self->current_line_no, buffer);
        }

        bfLexer_advance(self, 1);
        break;
      }
    }
  }

  return (bfToken){.type = EOP, .as = {.str = "EOP"}};
}

char bfLexer_peek(const BifrostLexer* self, size_t amt)
{
  return *bfLexer_peekStr(self, amt);
}

const char* bfLexer_peekStr(const BifrostLexer* self, size_t amt)
{
  const char* target_str = self->source_bgn + self->cursor + amt;

  if (target_str < self->source_end)
  {
    return target_str;
  }

  return self->source_end;
}

bfBool32 bfLexer_isWhitespace(char c)
{
  /* if char == ' ', '\t', '\n', '\v', '\f', '\r */
  return isspace(c);
}

bfBool32 bfLexer_isNewline(char c)
{
  return c == '\n' || c == '\r' || c == '\0';
}

void bfLexer_skipWhile(BifrostLexer* self, bfBool32 (*condition)(char c))
{
  while (condition(bfLexer_peek(self, 0)) && self->source_bgn + self->cursor < self->source_end)
  {
    bfLexer_advance(self, 1);
  }
}

void bfLexer_skipWhitespace(BifrostLexer* self)
{
  bfLexer_skipWhile(self, &bfLexer_isWhitespace);
}

static bfBool32 bfLexer_isNotNewline(char c)
{
  return !bfLexer_isNewline(c);
}

void bfLexer_skipLineComment(BifrostLexer* self)
{
  bfLexer_advance(self, 2); /* // */
  bfLexer_skipWhile(self, &bfLexer_isNotNewline);
}

void bfLexer_skipBlockComment(BifrostLexer* self)
{
  const size_t line_no = self->current_line_no;
  bfLexer_advance(self, 2); /* / * */

  while (bfLexer_peek(self, 0) != '*' || bfLexer_peek(self, 1) != '/')
  {
    if (bfLexer_peek(self, 0) == '\0')
    {
      const bfErrorFn err_fn = self->vm->params.error_fn;

      if (err_fn)
      {
        char buffer[64];

        const int num_bytes = sprintf(
         buffer,
         "Unfinished block comment starting on line(%zu)",
         line_no);

        assert(num_bytes < (int)sizeof(buffer) && "Oops Buffer Overflow");

        err_fn(self->vm, BIFROST_VM_ERROR_LEXER, (int)self->current_line_no, buffer);
      }
      break;
    }

    bfLexer_advance(self, 1);
  }

  bfLexer_advance(self, 2); /* * / */
}

void bfLexer_advance(BifrostLexer* self, size_t amt)
{
  self->cursor += amt;

  /* NOTE(Shareef):
      On windows a newline is '\r\n' rather than just the
      Unix '\n' so we need to skip another character
      to make sure the line count is correct.
  */

  const char curr = bfLexer_peek(self, 0);

  if (bfLexer_isNewline(curr) || amt == 0)
  {
    const size_t source_length = self->source_end - self->source_bgn;

    ++self->current_line_no;
    self->line_pos_bgn = self->cursor + (curr == '\n');
    self->line_pos_end = self->line_pos_bgn + (curr == '\n');

    while (!bfLexer_isNewline(self->source_bgn[self->line_pos_end]) && self->line_pos_end < source_length)
    {
      ++self->line_pos_end;
    }

    self->line_pos_end = self->line_pos_end < source_length ? self->line_pos_end + 1 : source_length;
  }
}

void bfLexer_advanceLine(BifrostLexer* self)
{
  bfLexer_skipWhile(self, &bfLexer_isNotNewline);
}

bfBool32 bfLexer_isDigit(char c)
{
  return isdigit(c);
}

bfBool32 bfLexer_isFollowedByDigit(BifrostLexer* self, char c, char m)
{
  return c == m && bfLexer_isDigit(bfLexer_peek(self, 1));
}

bfToken bfLexer_parseNumber(BifrostLexer* self)
{
  const char*     bgn   = bfLexer_peekStr(self, 0);
  char*           end   = NULL;
  const bfFloat64 value = strtof(bgn, &end);
  bfLexer_advance(self, end - bgn);

  const char current = bfLexer_peek(self, 0);

  if (current == 'f' || current == 'F')
  {
    bfLexer_advance(self, 1);
  }

  return BIFROST_TOKEN_MAKE_NUM(CONST_REAL, value);
}

bfBool32 bfLexer_isID(char c)
{
  return isalpha(c) || c == '_' || bfLexer_isDigit(c);
}

bfToken bfLexer_parseID(BifrostLexer* self)
{
  const char* bgn = bfLexer_peekStr(self, 0);
  bfLexer_skipWhile(self, &bfLexer_isID);
  const char*  end          = bfLexer_peekStr(self, 0);
  const size_t length       = end - bgn;
  const size_t num_keywords = self->num_keywords;

  for (size_t i = 0; i < num_keywords; ++i)
  {
    const bfKeyword* keyword = self->keywords + i;

    if (keyword->length == length && strncmp(keyword->keyword, bgn, length) == 0)
    {
      return keyword->value;
    }
  }

  return BIFROST_TOKEN_MAKE_STR_RANGE(IDENTIFIER, bgn, bgn + length);
}

static bfBool32 bfLexer_isNotQuote(char c)
{
  return c != '\"';
}

// Not handling the escape sequences in the lexer anymore.
// it is now the parsers' job. (this simplifies the case where
// if I wanted to add some special language specific sequences EX: Variable based).
// This also makes it so I can make this Lexer non allocating which is awesome.
bfToken bfLexer_parseString(BifrostLexer* self)
{
  bfLexer_advance(self, 1);  // '"'

  const char* bgn = bfLexer_peekStr(self, 0);

  while (bfLexer_isNotQuote(bfLexer_peek(self, 0)) && self->source_bgn + self->cursor < self->source_end)
  {
    bfLexer_advance(self, 1);

    if (bfLexer_peek(self, 0) == '\\' && bfLexer_peek(self, 1) == '\"')
    {
      bfLexer_advance(self, 2);
    }
  }
  
  const char* end = bfLexer_peekStr(self, 0);

  bfLexer_advance(self, 1);  // '"'

  return BIFROST_TOKEN_MAKE_STR_RANGE(CONST_STR, bgn, end);
}

void bfLexer_dtor(BifrostLexer* self)
{
  (void)self; /* Great and Non Allocating. */
}

const char* tokentypeToString(bfTokenType t)
{
#define TT_TO_STR(t) \
  case t: return #t

  switch (t)
  {
    TT_TO_STR(BIFROST_TOKEN_SUPER);
    TT_TO_STR(BIFROST_TOKEN_AS);
    TT_TO_STR(BIFROST_TOKEN_STATIC);
    TT_TO_STR(BIFROST_TOKEN_NEW);
    TT_TO_STR(BIFROST_TOKEN_CTRL_BREAK);
    TT_TO_STR(BIFROST_TOKEN_L_PAREN);
    TT_TO_STR(BIFROST_TOKEN_R_PAREN);
    TT_TO_STR(BIFROST_TOKEN_L_SQR_BOI);
    TT_TO_STR(BIFROST_TOKEN_R_SQR_BOI);
    TT_TO_STR(BIFROST_TOKEN_L_CURLY);
    TT_TO_STR(BIFROST_TOKEN_R_CURLY);
    TT_TO_STR(HASHTAG);
    TT_TO_STR(COLON);
    TT_TO_STR(BIFROST_TOKEN_SEMI_COLON);
    TT_TO_STR(BIFROST_TOKEN_COMMA);
    TT_TO_STR(BIFROST_TOKEN_EQUALS);
    TT_TO_STR(PLUS);
    TT_TO_STR(MINUS);
    TT_TO_STR(MULT);
    TT_TO_STR(DIV);
    TT_TO_STR(PLUS_EQUALS);
    TT_TO_STR(MINUS_EQUALS);
    TT_TO_STR(INCREMENT);
    TT_TO_STR(DECREMENT);
    TT_TO_STR(DOT);
    TT_TO_STR(IDENTIFIER);
    TT_TO_STR(VAR_DECL);
    TT_TO_STR(IMPORT);
    TT_TO_STR(FUNC);
    TT_TO_STR(BIFROST_TOKEN_CLASS);
    TT_TO_STR(CTRL_IF);
    TT_TO_STR(CTRL_ELSE);
    TT_TO_STR(CTRL_EE);
    TT_TO_STR(CTRL_LT);
    TT_TO_STR(CTRL_GT);
    TT_TO_STR(CTRL_LE);
    TT_TO_STR(CTRL_GE);
    TT_TO_STR(CTRL_OR);
    TT_TO_STR(CTRL_AND);
    TT_TO_STR(CTRL_NE);
    TT_TO_STR(BIFROST_TOKEN_CTRL_WHILE);
    TT_TO_STR(CTRL_FOR);
    TT_TO_STR(CTRL_RETURN);
    TT_TO_STR(CTRL_NEGATE);
    TT_TO_STR(CONST_STR);
    TT_TO_STR(CONST_REAL);
    TT_TO_STR(CONST_BOOL);
    TT_TO_STR(CONST_NIL);
    TT_TO_STR(EOP);
    default: return "There was a grave mistake";
  }
#undef TT_TO_STR
}

void printToken(const bfToken* token)
{
  const char* const type_str = tokentypeToString(token->type);
  const char* const value    = token->as.str;

  printf("[%30s] => ", type_str);
  if (token->type == CONST_STR || token->type == IDENTIFIER)
  {
    printf("[%.*s]", (int)bfStringRange_length(&token->as.str_range), value);
  }
  else if (token->type == CONST_REAL)
  {
    printf("[%g]", token->as.num);
  }
  else
  {
    printf("[%s]", value);
  }

  printf("\n");
}
