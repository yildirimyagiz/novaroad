/**
 * @file lexer.h
 * @brief Lexical analyzer (tokenizer) — Nova v10 Intent-First token catalogue
 *
 * ═══════════════════════════════════════════════════════════════════════════
 *  All v10 keywords: data, cases, rules, yield, check, each, flow, spawn,
 *  match, given, open, alias, on, loop, etc.
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_LEXER_H
#define NOVA_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 *  TOKEN TYPE ENUMERATION — Nova v10.0 Complete Set
 * ═══════════════════════════════════════════════════════════════════════════
 */
typedef enum {
  TOKEN_EOF,
  TOKEN_ERROR,

  /* ── Identifiers and Literals ─────────────────────────────────────────── */
  TOKEN_IDENT,
  TOKEN_LIT_INT,
  TOKEN_LIT_FLOAT,
  TOKEN_LIT_STR,
  TOKEN_LIT_CHAR,
  TOKEN_LIT_TRUE,
  TOKEN_LIT_FALSE,
  TOKEN_LIT_UNIT, /* Unit literal: 5.kg, 9.81.m/s² */

  /* ── Legacy tokens (bootstrap compat) ─────────────────────────────────── */
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_CHAR,

  /* ── Core keywords ────────────────────────────────────────────────────── */
  TOKEN_KEYWORD_FN,
  TOKEN_KEYWORD_LET,
  TOKEN_KEYWORD_CONST,
  TOKEN_KEYWORD_VAR,
  TOKEN_KEYWORD_IF,
  TOKEN_KEYWORD_CHECK, /* Nova-style conditional guard */
  TOKEN_KEYWORD_ELSE,
  TOKEN_KEYWORD_WHILE,
  TOKEN_KEYWORD_FOR,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_BREAK,
  TOKEN_KEYWORD_CONTINUE,
  TOKEN_KEYWORD_LOOP, /* v10: infinite loop construct */

  /* ── Legacy declaration keywords ──────────────────────────────────────── */
  TOKEN_KEYWORD_STRUCT,
  TOKEN_KEYWORD_ENUM,
  TOKEN_KEYWORD_TRAIT,

  /* ── Nova v10 "Intent-First" declaration keywords ─────────────────────── */
  TOKEN_KEYWORD_DATA,  /* replaces struct in Nova syntax       */
  TOKEN_KEYWORD_CASES, /* replaces enum  in Nova syntax        */
  TOKEN_KEYWORD_RULES, /* replaces trait  in Nova syntax       */
  TOKEN_KEYWORD_YIELD, /* replaces return in Nova syntax       */
  TOKEN_KEYWORD_EACH,  /* replaces for-in in Nova syntax       */
  TOKEN_KEYWORD_FLOW,  /* async stream / generator return type */
  TOKEN_KEYWORD_SPAWN, /* lightweight concurrent task creation */
  TOKEN_KEYWORD_GIVEN, /* guard clause in match arms           */
  TOKEN_KEYWORD_OPEN,  /* public visibility / extensibility    */
  TOKEN_KEYWORD_ALIAS, /* type alias declaration               */
  TOKEN_KEYWORD_ON,    /* event handler binding keyword        */

  /* ── Module & visibility ──────────────────────────────────────────────── */
  TOKEN_KEYWORD_IMPL,
  TOKEN_KEYWORD_TYPE,
  TOKEN_KEYWORD_IMPORT,
  TOKEN_KEYWORD_EXPORT,
  TOKEN_KEYWORD_PUB,
  TOKEN_KEYWORD_MOD,
  TOKEN_KEYWORD_AS,
  TOKEN_KEYWORD_USE,
  TOKEN_KEYWORD_MATCH,
  TOKEN_KEYWORD_IN,
  TOKEN_KEYWORD_EXPOSE,
  TOKEN_KEYWORD_CRATE,
  TOKEN_KEYWORD_SELF,
  TOKEN_KEYWORD_BASE,
  TOKEN_KEYWORD_SUPER,

  /* ── Concurrency ──────────────────────────────────────────────────────── */
  TOKEN_KEYWORD_ASYNC,
  TOKEN_KEYWORD_AWAIT,
  TOKEN_KEYWORD_ACTOR,

  /* ── Domain-specific / physics / AI keywords ──────────────────────────── */
  TOKEN_KEYWORD_HEAP,
  TOKEN_KEYWORD_FREE,
  TOKEN_KEYWORD_NEW,
  TOKEN_KEYWORD_SHAPE,
  TOKEN_KEYWORD_SKILL,
  TOKEN_KEYWORD_UNIT,
  TOKEN_KEYWORD_QTY,
  TOKEN_KEYWORD_DIMS, /* dimensional analysis declaration */
  TOKEN_KEYWORD_TENSOR,
  TOKEN_KEYWORD_KERNEL,
  TOKEN_KEYWORD_BRING,
  TOKEN_KEYWORD_APPLY,
  TOKEN_KEYWORD_DERIVES,
  TOKEN_KEYWORD_KIND,
  TOKEN_KEYWORD_MUT,
  TOKEN_KEYWORD_REQUIRE,
  TOKEN_KEYWORD_ENSURE,
  TOKEN_KEYWORD_TRY,
  TOKEN_KEYWORD_CATCH,
  TOKEN_KEYWORD_FOREIGN,
  TOKEN_KEYWORD_COMPONENT, /* UI component declaration */
  TOKEN_KEYWORD_VIEW,      /* UI view block */
  TOKEN_KEYWORD_PARALLEL,  /* GPU parallel execution */
  TOKEN_KEYWORD_SYNC,      /* GPU thread synchronization */
  TOKEN_KEYWORD_THREAD,    /* GPU thread config */
  TOKEN_KEYWORD_BLOCK,     /* GPU block config */
  TOKEN_KEYWORD_GRAD,      /* Mathematical gradient */
  TOKEN_KEYWORD_DIFF,      /* Mathematical derivative */
  TOKEN_KEYWORD_INTEGRAL,  /* Mathematical integral */
  TOKEN_KEYWORD_SUM,       /* Mathematical sum */
  TOKEN_KEYWORD_PROD,      /* Mathematical product */
  TOKEN_KEYWORD_LIMIT,     /* Mathematical limit */
  TOKEN_KEYWORD_SOLVE,     /* Mathematical solver */
  TOKEN_KEYWORD_SPACE,     /* Namespace / Module */

  /* ── Operators ────────────────────────────────────────────────────────── */
  TOKEN_PLUS,                /* +  */
  TOKEN_MINUS,               /* -  */
  TOKEN_STAR,                /* *  */
  TOKEN_STAR_STAR,           /* ** exponentiation */
  TOKEN_SLASH,               /* /  */
  TOKEN_PERCENT,             /* %  */
  TOKEN_EQ,                  /* =  assignment */
  TOKEN_EQ_EQ,               /* == equality   */
  TOKEN_BANG,                /* !  logical not */
  TOKEN_BANG_EQ,             /* != inequality  */
  TOKEN_LT,                  /* <  */
  TOKEN_LT_EQ,               /* <= */
  TOKEN_GT,                  /* >  */
  TOKEN_GT_EQ,               /* >= */
  TOKEN_AMPERSAND,           /* &  bitwise and  */
  TOKEN_AMPERSAND_AMPERSAND, /* && logical and  */
  TOKEN_AMP_AMP,             /* && alias (legacy compat) */
  TOKEN_PIPE,                /* |  bitwise or   */
  TOKEN_PIPE_PIPE,           /* || logical or   */
  TOKEN_CARET,               /* ^  xor / power  */
  TOKEN_TILDE,               /* ~  bitwise not  */

  /* ── Delimiters ───────────────────────────────────────────────────────── */
  TOKEN_LPAREN,      /* (  */
  TOKEN_RPAREN,      /* )  */
  TOKEN_LBRACE,      /* {  */
  TOKEN_RBRACE,      /* }  */
  TOKEN_LBRACKET,    /* [  */
  TOKEN_RBRACKET,    /* ]  */
  TOKEN_SEMICOLON,   /* ;  */
  TOKEN_COMMA,       /* ,  */
  TOKEN_DOT,         /* .  */
  TOKEN_DOT_DOT,     /* .. range        */
  TOKEN_COLON,       /* :  type annot   */
  TOKEN_COLON_COLON, /* :: namespace     */
  TOKEN_ARROW,       /* -> return type  */
  TOKEN_FAT_ARROW,   /* => match arm    */
  TOKEN_QUESTION,    /* ?  optional     */
  TOKEN_HASH,        /* #  attribute    */
  TOKEN_AT,          /* @  extern/attr  */

  /* ── Sentinel ─────────────────────────────────────────────────────────── */
  TOKEN_TYPE_COUNT
} nova_token_type_t;

/* ═══════════════════════════════════════════════════════════════════════════
 *  TOKEN STRUCTURE
 * ═══════════════════════════════════════════════════════════════════════════
 */
typedef struct {
  nova_token_type_t type;
  const char *start;
  size_t length;
  int line;
  int column;
  union {
    int64_t integer;
    double floating;
    const char *string;
  } value;
} nova_token_t;

/* ═══════════════════════════════════════════════════════════════════════════
 *  LEXER API
 * ═══════════════════════════════════════════════════════════════════════════
 */
typedef struct nova_lexer nova_lexer_t;

nova_lexer_t *nova_lexer_create(const char *source);
nova_token_t nova_lexer_next(nova_lexer_t *lexer);
nova_token_t nova_lexer_peek(nova_lexer_t *lexer);
const char *nova_token_type_name(nova_token_type_t type);
void nova_lexer_destroy(nova_lexer_t *lexer);

#ifdef __cplusplus
}
#endif
#endif /* NOVA_LEXER_H */
