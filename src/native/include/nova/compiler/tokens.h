/**
 * @file tokens.h
 * @brief Token types and structures for Nova v10 lexer
 *
 * ═══════════════════════════════════════════════════════════════════════════
 *  Nova v10 "Intent-First" Token Catalogue
 *  Includes all v10 keywords: data, cases, rules, yield, check, each,
 *  flow, spawn, match, given, open, alias, @extern, on, etc.
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_COMPILER_TOKENS_H
#define NOVA_COMPILER_TOKENS_H

#include <stddef.h>

/**
 * Token types — Nova v10 complete set
 */
typedef enum {
  /* ── Literals ───────────────────────────────────────────────────────── */
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_IDENTIFIER,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_NULL,
  TOKEN_LIT_INT,
  TOKEN_LIT_FLOAT,
  TOKEN_LIT_STR,
  TOKEN_LIT_CHAR,
  TOKEN_LIT_TRUE,
  TOKEN_LIT_FALSE,
  TOKEN_LIT_UNIT,

  /* ── Legacy keyword aliases (retained for bootstrap compat) ───────── */
  TOKEN_IDENT,

  /* ── Core keywords ─────────────────────────────────────────────────── */
  TOKEN_KEYWORD_FN,
  TOKEN_KEYWORD_LET,
  TOKEN_KEYWORD_CONST,
  TOKEN_KEYWORD_VAR,
  TOKEN_KEYWORD_IF,
  TOKEN_KEYWORD_CHECK, /* Nova-style conditional */
  TOKEN_KEYWORD_ELSE,
  TOKEN_KEYWORD_WHILE,
  TOKEN_KEYWORD_FOR,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_BREAK,
  TOKEN_KEYWORD_CONTINUE,
  TOKEN_KEYWORD_STRUCT,
  TOKEN_KEYWORD_ENUM,
  TOKEN_KEYWORD_IMPL,
  TOKEN_KEYWORD_TRAIT,
  TOKEN_KEYWORD_TYPE,
  TOKEN_KEYWORD_IMPORT,
  TOKEN_KEYWORD_EXPORT,
  TOKEN_KEYWORD_PUB,
  TOKEN_KEYWORD_ASYNC,
  TOKEN_KEYWORD_AWAIT,
  TOKEN_KEYWORD_MATCH,

  /* ── Nova v10 "Intent-First" keywords ──────────────────────────────── */
  TOKEN_KEYWORD_DATA,  /* replaces struct in Nova syntax */
  TOKEN_KEYWORD_CASES, /* replaces enum  in Nova syntax */
  TOKEN_KEYWORD_RULES, /* replaces trait  in Nova syntax */
  TOKEN_KEYWORD_YIELD, /* replaces return in Nova syntax */
  TOKEN_KEYWORD_EACH,  /* replaces for-in in Nova syntax */
  TOKEN_KEYWORD_FLOW,  /* async stream / generator type  */
  TOKEN_KEYWORD_SPAWN, /* lightweight task creation      */
  TOKEN_KEYWORD_GIVEN, /* guard clause in match arms     */
  TOKEN_KEYWORD_OPEN,  /* public visibility modifier     */
  TOKEN_KEYWORD_ALIAS, /* type alias declaration         */
  TOKEN_KEYWORD_ON,    /* event handler binding          */
  TOKEN_KEYWORD_USE,
  TOKEN_KEYWORD_MOD,
  TOKEN_KEYWORD_AS,
  TOKEN_KEYWORD_IN,
  TOKEN_KEYWORD_LOOP,

  /* ── Nova specialty keywords ───────────────────────────────────────── */
  TOKEN_KEYWORD_ACTOR,
  TOKEN_KEYWORD_HEAP,
  TOKEN_KEYWORD_FREE,
  TOKEN_KEYWORD_NEW,
  TOKEN_KEYWORD_SHAPE,
  TOKEN_KEYWORD_EXPOSE,
  TOKEN_KEYWORD_SKILL,
  TOKEN_KEYWORD_UNIT,
  TOKEN_KEYWORD_QTY,
  TOKEN_KEYWORD_DIMS,
  TOKEN_KEYWORD_TENSOR,
  TOKEN_KEYWORD_KERNEL,
  TOKEN_KEYWORD_CRATE,
  TOKEN_KEYWORD_SELF,
  TOKEN_KEYWORD_BASE,
  TOKEN_KEYWORD_SUPER,
  TOKEN_KEYWORD_BRING,
  TOKEN_KEYWORD_APPLY,
  TOKEN_KEYWORD_DERIVES,
  TOKEN_KEYWORD_KIND,
  TOKEN_KEYWORD_MUT,
  TOKEN_KEYWORD_REQUIRE,
  TOKEN_KEYWORD_ENSURE,
  TOKEN_KEYWORD_TRY,
  TOKEN_KEYWORD_CATCH,

  /* ── Operators ─────────────────────────────────────────────────────── */
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_STAR_STAR, /* ** exponentiation */
  TOKEN_SLASH,
  TOKEN_PERCENT,
  TOKEN_EQ,                  /* =  assignment     */
  TOKEN_EQ_EQ,               /* == equality       */
  TOKEN_BANG,                /* !  logical not    */
  TOKEN_BANG_EQ,             /* != inequality     */
  TOKEN_LT,                  /* <  less than      */
  TOKEN_LT_EQ,               /* <= less or equal  */
  TOKEN_GT,                  /* >  greater than   */
  TOKEN_GT_EQ,               /* >= greater or eq  */
  TOKEN_AMPERSAND,           /* &  bitwise and    */
  TOKEN_AMPERSAND_AMPERSAND, /* && logical and    */
  TOKEN_PIPE,                /* |  bitwise or     */
  TOKEN_PIPE_PIPE,           /* || logical or     */
  TOKEN_CARET,               /* ^  bitwise xor    */
  TOKEN_TILDE,               /* ~  bitwise not    */
  TOKEN_LEFT_SHIFT,          /* << shift left     */
  TOKEN_RIGHT_SHIFT,         /* >> shift right    */
  TOKEN_ARROW,               /* -> return type    */
  TOKEN_FAT_ARROW,           /* => match arm      */
  TOKEN_DOT,                 /* .  field access   */
  TOKEN_DOT_DOT,             /* .. range          */
  TOKEN_QUESTION,            /* ?  optional unwr  */
  TOKEN_COLON,               /* :  type annotation*/
  TOKEN_COLON_COLON,         /* :: namespace path */
  TOKEN_DOUBLE_COLON,        /* :: (alias)        */
  TOKEN_HASH,                /* #  attribute pfx  */
  TOKEN_AT,                  /* @  extern marker  */

  /* ── Delimiters ────────────────────────────────────────────────────── */
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_COMMA,
  TOKEN_SEMICOLON,

  /* ── Special ───────────────────────────────────────────────────────── */
  TOKEN_NEWLINE,
  TOKEN_EOF,
  TOKEN_ERROR,
  TOKEN_OPERATOR,

  /* ── Sentinel for array sizing ─────────────────────────────────────── */
  TOKEN_TYPE_COUNT
} nova_token_type_t;

/**
 * Token structure — carries lexeme, position, and any embedded literal
 */
typedef struct {
  nova_token_type_t type;
  const char *start;
  size_t length;
  int line;
  int column;
} nova_token_t;

/**
 * Get string representation of token type (debug)
 */
const char *nova_token_type_name(nova_token_type_t type);

#endif /* NOVA_COMPILER_TOKENS_H */
