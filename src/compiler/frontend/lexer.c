/**
 * @file lexer.c
 * @brief Lexical analyzer implementation for Nova language
 */

#include "compiler/lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// INTERNAL LEXER STRUCTURE
// ══════════════════════════════════════════════════════════════════════════════

struct nova_lexer {
  const char *source;
  size_t source_length;
  size_t current;
  int line;
  int column;
};

// ══════════════════════════════════════════════════════════════════════════════
// KEYWORD MAP
// ══════════════════════════════════════════════════════════════════════════════

typedef struct {
  const char *keyword;
  nova_token_type_t type;
} KeywordEntry;

static KeywordEntry keywords[] = {
    // Basic keywords from lexer.h
    {"fn", TOKEN_KEYWORD_FN},
    {"let", TOKEN_KEYWORD_LET},
    {"const", TOKEN_KEYWORD_CONST},
    {"var", TOKEN_KEYWORD_VAR},
    {"if", TOKEN_KEYWORD_IF},
    {"check", TOKEN_KEYWORD_CHECK},
    {"else", TOKEN_KEYWORD_ELSE},
    {"while", TOKEN_KEYWORD_WHILE},
    {"for", TOKEN_KEYWORD_FOR},
    {"return", TOKEN_KEYWORD_RETURN},
    {"yield", TOKEN_KEYWORD_YIELD}, // Nova uses 'yield' instead of 'return'
    {"break", TOKEN_KEYWORD_BREAK},
    {"continue", TOKEN_KEYWORD_CONTINUE},
    {"struct", TOKEN_KEYWORD_STRUCT},
    {"enum", TOKEN_KEYWORD_ENUM},
    {"cases", TOKEN_KEYWORD_CASES},
    {"data", TOKEN_KEYWORD_DATA},
    {"rules", TOKEN_KEYWORD_RULES},
    {"rules", TOKEN_KEYWORD_TRAIT}, /* legacy alias */
    {"impl", TOKEN_KEYWORD_IMPL},
    {"trait", TOKEN_KEYWORD_TRAIT},
    {"type", TOKEN_KEYWORD_TYPE},
    {"import", TOKEN_KEYWORD_IMPORT},
    {"export", TOKEN_KEYWORD_EXPORT},
    {"pub", TOKEN_KEYWORD_PUB},
    {"async", TOKEN_KEYWORD_ASYNC},
    {"await", TOKEN_KEYWORD_AWAIT},
    {"actor", TOKEN_KEYWORD_ACTOR},
    {"heap", TOKEN_KEYWORD_HEAP},
    {"free", TOKEN_KEYWORD_FREE},
    {"new", TOKEN_KEYWORD_NEW},
    {"shape", TOKEN_KEYWORD_SHAPE},
    {"mod", TOKEN_KEYWORD_MOD},
    {"as", TOKEN_KEYWORD_AS},
    {"expose", TOKEN_KEYWORD_EXPOSE},
    {"skill", TOKEN_KEYWORD_SKILL},
    {"use", TOKEN_KEYWORD_USE},
    {"match", TOKEN_KEYWORD_MATCH},
    {"each", TOKEN_KEYWORD_EACH},
    {"in", TOKEN_KEYWORD_IN},
    {"unit", TOKEN_KEYWORD_UNIT},
    {"qty", TOKEN_KEYWORD_QTY},
    {"dims", TOKEN_KEYWORD_DIMS},
    {"tensor", TOKEN_KEYWORD_TENSOR},
    {"kernel", TOKEN_KEYWORD_KERNEL},
    {"open", TOKEN_KEYWORD_OPEN},
    {"crate", TOKEN_KEYWORD_CRATE},
    {"self", TOKEN_KEYWORD_SELF},
    {"base", TOKEN_KEYWORD_BASE},
    {"super", TOKEN_KEYWORD_SUPER},
    {"bring", TOKEN_KEYWORD_BRING},
    {"apply", TOKEN_KEYWORD_APPLY},
    {"derives", TOKEN_KEYWORD_DERIVES},
    {"kind", TOKEN_KEYWORD_KIND},
    {"mut", TOKEN_KEYWORD_MUT},
    {"require", TOKEN_KEYWORD_REQUIRE},
    {"ensure", TOKEN_KEYWORD_ENSURE},
    {"try", TOKEN_KEYWORD_TRY},
    {"catch", TOKEN_KEYWORD_CATCH},
    {"foreign", TOKEN_KEYWORD_FOREIGN},
    {"on", TOKEN_KEYWORD_ON},
    {"loop", TOKEN_KEYWORD_LOOP},
    {"flow", TOKEN_KEYWORD_FLOW},
    {"spawn", TOKEN_KEYWORD_SPAWN},
    {"given", TOKEN_KEYWORD_GIVEN},
    {"alias", TOKEN_KEYWORD_ALIAS},
    {"space", TOKEN_KEYWORD_SPACE},
    {"component", TOKEN_KEYWORD_COMPONENT},
    {"view", TOKEN_KEYWORD_VIEW},
    {"parallel", TOKEN_KEYWORD_PARALLEL},
    {"sync", TOKEN_KEYWORD_SYNC},
    {"thread", TOKEN_KEYWORD_THREAD},
    {"block", TOKEN_KEYWORD_BLOCK},
    {"grad", TOKEN_KEYWORD_GRAD},
    {"diff", TOKEN_KEYWORD_DIFF},
    {"integral", TOKEN_KEYWORD_INTEGRAL},
    {"sum", TOKEN_KEYWORD_SUM},
    {"prod", TOKEN_KEYWORD_PROD},
    {"limit", TOKEN_KEYWORD_LIMIT},
    {"solve", TOKEN_KEYWORD_SOLVE},

    {"true", TOKEN_LIT_TRUE},
    {"false", TOKEN_LIT_FALSE},
    {NULL, TOKEN_EOF} // Sentinel
};

// ══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

static bool is_at_end(nova_lexer_t *lexer) {
  return lexer->current >= lexer->source_length;
}

static char peek(nova_lexer_t *lexer) {
  if (is_at_end(lexer))
    return '\0';
  return lexer->source[lexer->current];
}

static char peek_next(nova_lexer_t *lexer) {
  if (lexer->current + 1 >= lexer->source_length)
    return '\0';
  return lexer->source[lexer->current + 1];
}

static char advance(nova_lexer_t *lexer) {
  char c = peek(lexer);
  lexer->current++;
  lexer->column++;
  if (c == '\n') {
    lexer->line++;
    lexer->column = 1;
  }
  return c;
}

static bool match(nova_lexer_t *lexer, char expected) {
  if (is_at_end(lexer) || peek(lexer) != expected)
    return false;
  advance(lexer);
  return true;
}

static void skip_whitespace(nova_lexer_t *lexer) {
  while (!is_at_end(lexer)) {
    char c = peek(lexer);
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance(lexer);
      break;
    case '\n':
      advance(lexer);
      break;
    case '/':
      if (peek_next(lexer) == '/') {
        // Single-line comment
        while (!is_at_end(lexer) && peek(lexer) != '\n') {
          advance(lexer);
        }
      } else if (peek_next(lexer) == '*') {
        // Multi-line comment
        advance(lexer); // consume /
        advance(lexer); // consume *
        while (!is_at_end(lexer)) {
          if (peek(lexer) == '*' && peek_next(lexer) == '/') {
            advance(lexer); // consume *
            advance(lexer); // consume /
            break;
          }
          advance(lexer);
        }
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static nova_token_type_t check_keyword(nova_lexer_t *lexer, size_t start,
                                       size_t length, const char *rest,
                                       nova_token_type_t type) {
  if (lexer->current - start == length &&
      memcmp(&lexer->source[start], rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENT;
}

static nova_token_type_t identifier_type(nova_lexer_t *lexer, size_t start) {
  size_t length = lexer->current - start;
  const char *text = &lexer->source[start];

  // Check keywords
  for (KeywordEntry *entry = keywords; entry->keyword != NULL; entry++) {
    if (length == strlen(entry->keyword) &&
        memcmp(text, entry->keyword, length) == 0) {
      return entry->type;
    }
  }

  return TOKEN_IDENT;
}

static nova_token_t make_token(nova_lexer_t *lexer, nova_token_type_t type,
                               size_t start) {
  nova_token_t token;
  token.type = type;
  token.start = &lexer->source[start];
  token.length = lexer->current - start;
  token.line = lexer->line;
  token.column = lexer->column - (int)token.length;
  return token;
}

static nova_token_t token_error_local(const char *message, int line,
                                      int column) {
  nova_token_t token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = strlen(message);
  token.line = line;
  token.column = column;
  return token;
}

// ══════════════════════════════════════════════════════════════════════════════
// NUMBER SCANNING
// ══════════════════════════════════════════════════════════════════════════════

static nova_token_t scan_number(nova_lexer_t *lexer, size_t start) {
  nova_token_type_t type = TOKEN_LIT_INT;

  /* Handle hex (0x) and binary (0b) literals */
  if (lexer->source[start] == '0' && !is_at_end(lexer)) {
    char next = peek(lexer);
    if (next == 'x' || next == 'X') {
      advance(lexer); /* consume 'x' */
      while (!is_at_end(lexer) &&
             (isxdigit((unsigned char)peek(lexer)) || peek(lexer) == '_')) {
        advance(lexer);
      }
      return make_token(lexer, TOKEN_LIT_INT, start);
    }
    if (next == 'b' || next == 'B') {
      advance(lexer); /* consume 'b' */
      while (!is_at_end(lexer) &&
             (peek(lexer) == '0' || peek(lexer) == '1' || peek(lexer) == '_')) {
        advance(lexer);
      }
      return make_token(lexer, TOKEN_LIT_INT, start);
    }
  }

  /* Integer part */
  while (!is_at_end(lexer) && isdigit(peek(lexer))) {
    advance(lexer);
  }

  /* Fractional part: 3.14 */
  if (!is_at_end(lexer) && peek(lexer) == '.' && isdigit(peek_next(lexer))) {
    type = TOKEN_LIT_FLOAT;
    advance(lexer); /* consume '.' */
    while (!is_at_end(lexer) && isdigit(peek(lexer))) {
      advance(lexer);
    }
  }

  /* Scientific notation: 1.6e-19, 9.8E+2, 3e10
   * Pattern: [eE] [+-]? [0-9]+
   * If 'e'/'E' is found, this is always a float literal. */
  if (!is_at_end(lexer) && (peek(lexer) == 'e' || peek(lexer) == 'E')) {
    char next = peek_next(lexer);
    /* Accept e/E if followed by digit or +/- then digit */
    if (isdigit(next) || next == '+' || next == '-') {
      type = TOKEN_LIT_FLOAT;
      advance(lexer); /* consume 'e' or 'E' */
      if (!is_at_end(lexer) && (peek(lexer) == '+' || peek(lexer) == '-')) {
        advance(lexer); /* consume sign */
      }
      if (!is_at_end(lexer) && isdigit(peek(lexer))) {
        while (!is_at_end(lexer) && isdigit(peek(lexer))) {
          advance(lexer);
        }
      } else {
        /* Malformed exponent like '1e+' — treat as no exponent,
         * back up past the 'e' / sign. Parser will handle. */
        /* Note: we already advanced past 'e' and optional sign.
         * We cannot easily unget here, so we just leave it — strtod
         * will correctly reject it and parse up to the 'e'. */
      }
    }
    /* else: 'e' starts an identifier token after this number — stop here */
  }

  return make_token(lexer, type, start);
}

// ══════════════════════════════════════════════════════════════════════════════
// STRING SCANNING
// ══════════════════════════════════════════════════════════════════════════════

static nova_token_t scan_string(nova_lexer_t *lexer, size_t start) {
  while (!is_at_end(lexer) && peek(lexer) != '"') {
    if (peek(lexer) == '\n') {
      // Unterminated string
      return token_error_local("Unterminated string", lexer->line,
                               lexer->column);
    }
    advance(lexer);
  }

  if (is_at_end(lexer)) {
    return token_error_local("Unterminated string", lexer->line, lexer->column);
  }

  // Consume closing quote
  advance(lexer);

  return make_token(lexer, TOKEN_LIT_STR, start);
}

// ══════════════════════════════════════════════════════════════════════════════
// IDENTIFIER SCANNING
// ══════════════════════════════════════════════════════════════════════════════

/* Check if the current byte starts a Unicode character that is valid
 * in unit identifiers: ², ³, ·, °, Ω, Å, μ, etc.
 * Returns the byte-length of the sequence (2 or 3), or 0 if not a unit char. */
static int unit_unicode_len(const char *p, size_t remaining) {
  unsigned char c0 = (unsigned char)p[0];
  if (remaining < 2)
    return 0;

  unsigned char c1 = (unsigned char)p[1];

  /* 2-byte UTF-8: 0xC2 prefix */
  if (c0 == 0xC2) {
    /* ² = C2 B2, ³ = C2 B3, ¹ = C2 B9,
     * ° = C2 B0, · = C2 B7, μ = C2 B5 (micro) */
    if (c1 == 0xB2 || c1 == 0xB3 || c1 == 0xB9 || c1 == 0xB0 || c1 == 0xB7 ||
        c1 == 0xB5)
      return 2;
  }
  /* Ω = CE A9, μ = CE BC, Å = C3 85 */
  if (c0 == 0xCE && remaining >= 2) {
    if (c1 == 0xA9)
      return 2; /* Ω */
    if (c1 == 0xBC)
      return 2; /* μ (micro sign) */
    if (c1 == 0x94)
      return 2; /* Δ */
    if (c1 == 0xB1)
      return 2; /* α */
    if (c1 == 0xB2)
      return 2; /* β */
    if (c1 == 0xB3)
      return 2; /* γ */
  }
  if (c0 == 0xC3 && remaining >= 2) {
    if (c1 == 0x85)
      return 2; /* Å */
    if (c1 == 0x9C)
      return 2; /* Ü */
  }
  /* 3-byte sequences: E2 xx xx */
  if (c0 == 0xE2 && remaining >= 3) {
    unsigned char c1 = (unsigned char)p[1];
    unsigned char c2 = (unsigned char)p[2];
    /* Only allow specific mathematical/unit symbols (⁻⁰-⁹, ∞, ∑, ∫, ∂, ∇, ×, ÷,
     * →, ←, ⇒), NOT box-drawing characters (E2 94/95) which are used in
     * comments. */
    if (c1 == 0x80 && (c2 == 0xA2 || c2 == 0xA2))
      return 3; // · or ×
    if (c1 == 0x81)
      return 3; // ⁻⁰-⁹ etc
    if (c1 == 0x88 || c1 == 0x89)
      return 3; // Math symbols
    if (c1 == 0x86)
      return 3; // Arrows
    return 0;   // Skip box-drawing characters in identifiers
  }
  return 0;
}

/* Scan a character literal: 'x', '\n', '\\', '\'', etc.
 * start points to the opening single-quote (already consumed by caller). */
static nova_token_t scan_char(nova_lexer_t *lexer, size_t start) {
  if (is_at_end(lexer))
    return token_error_local("Unterminated char literal", lexer->line,
                             lexer->column);

  unsigned char c = (unsigned char)advance(lexer);
  if (c == '\\') {
    /* Escape sequence */
    if (is_at_end(lexer))
      return token_error_local("Unterminated char escape", lexer->line,
                               lexer->column);
    advance(lexer); /* consume escape char */
  } else if (c >= 0x80) {
    /* UTF-8 sequence: advance past continuation bytes */
    int len = 0;
    if ((c & 0xE0) == 0xC0)
      len = 1;
    else if ((c & 0xF0) == 0xE0)
      len = 2;
    else if ((c & 0xF8) == 0xF0)
      len = 3;

    for (int i = 0; i < len; i++) {
      if (!is_at_end(lexer) && (unsigned char)peek(lexer) >= 0x80) {
        advance(lexer);
      }
    }
  }

  if (is_at_end(lexer) || peek(lexer) != '\'')
    return token_error_local("Unterminated char literal", lexer->line,
                             lexer->column);

  advance(lexer); /* consume closing quote */
  return make_token(lexer, TOKEN_LIT_CHAR, start);
}

static nova_token_t scan_identifier(nova_lexer_t *lexer, size_t start) {
  while (!is_at_end(lexer)) {
    char c = peek(lexer);
    if (isalnum((unsigned char)c) || c == '_') {
      advance(lexer);
      continue;
    }
    /* Allow Unicode unit characters: ², ³, ·, °, Ω, Å, μ, etc. */
    size_t remaining = lexer->source_length - lexer->current;
    int ulen = unit_unicode_len(lexer->source + lexer->current, remaining);
    if (ulen > 0) {
      for (int i = 0; i < ulen; i++)
        advance(lexer);
      continue;
    }
    break;
  }

  nova_token_type_t type = identifier_type(lexer, start);
  return make_token(lexer, type, start);
}

// ══════════════════════════════════════════════════════════════════════════════
// MAIN SCANNING
// ══════════════════════════════════════════════════════════════════════════════

static nova_token_t scan_token(nova_lexer_t *lexer) {
  skip_whitespace(lexer);

  size_t start = lexer->current;

  if (is_at_end(lexer)) {
    return make_token(lexer, TOKEN_EOF, start);
  }

  char c = advance(lexer);

  // Single-character tokens and operators
  switch (c) {
  case '(':
    return make_token(lexer, TOKEN_LPAREN, start);
  case ')':
    return make_token(lexer, TOKEN_RPAREN, start);
  case '{':
    return make_token(lexer, TOKEN_LBRACE, start);
  case '}':
    return make_token(lexer, TOKEN_RBRACE, start);
  case '[':
    return make_token(lexer, TOKEN_LBRACKET, start);
  case ']':
    return make_token(lexer, TOKEN_RBRACKET, start);
  case ',':
    return make_token(lexer, TOKEN_COMMA, start);
  case ';':
    return make_token(lexer, TOKEN_SEMICOLON, start);
  case '.':
    return make_token(lexer, match(lexer, '.') ? TOKEN_DOT_DOT : TOKEN_DOT,
                      start);
  case '+':
    return make_token(lexer, TOKEN_PLUS, start);
  case '-':
    return make_token(lexer, match(lexer, '>') ? TOKEN_ARROW : TOKEN_MINUS,
                      start);
  case '*':
    if (!is_at_end(lexer) && peek(lexer) == '*') {
      advance(lexer); /* consume second '*' */
      return make_token(lexer, TOKEN_STAR_STAR, start);
    }
    return make_token(lexer, TOKEN_STAR, start);
  case '/':
    return make_token(lexer, TOKEN_SLASH, start);
  case '%':
    return make_token(lexer, TOKEN_PERCENT, start);
  case '=':
    return make_token(lexer,
                      match(lexer, '=')
                          ? TOKEN_EQ_EQ
                          : (match(lexer, '>') ? TOKEN_FAT_ARROW : TOKEN_EQ),
                      start);
  case '!':
    return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQ : TOKEN_BANG,
                      start);
  case '<':
    return make_token(lexer, match(lexer, '=') ? TOKEN_LT_EQ : TOKEN_LT, start);
  case '>':
    return make_token(lexer, match(lexer, '=') ? TOKEN_GT_EQ : TOKEN_GT, start);
  case '&':
    return make_token(
        lexer, match(lexer, '&') ? TOKEN_AMPERSAND_AMPERSAND : TOKEN_AMPERSAND,
        start);
  case '|':
    return make_token(lexer, match(lexer, '|') ? TOKEN_PIPE_PIPE : TOKEN_PIPE,
                      start);
  /* TOKEN_AMP_AMP handled above via TOKEN_AMPERSAND_AMPERSAND — no duplicate
   * needed */
  case ':':
    return make_token(
        lexer, match(lexer, ':') ? TOKEN_COLON_COLON : TOKEN_COLON, start);
  case '?':
    return make_token(lexer, TOKEN_QUESTION, start);
  case '#':
    return make_token(lexer, TOKEN_HASH, start);
  case '~':
    return make_token(lexer, TOKEN_TILDE, start);
  case '^':
    return make_token(lexer, TOKEN_CARET, start);
  case '@':
    return make_token(lexer, TOKEN_AT, start);
  case '\'':
    return scan_char(lexer, start);
  }

  // Numbers
  if (isdigit(c)) {
    return scan_number(lexer, start);
  }

  // Strings
  if (c == '"') {
    return scan_string(lexer, start);
  }

  // Identifiers and keywords
  if (isalpha((unsigned char)c) || c == '_') {
    return scan_identifier(lexer, start);
  }

  /* Unicode identifier start: °, Ω, Å, μ, ² used as standalone unit names
   * e.g. 100.Ω, 25.°C, 50.μA
   * Back up one byte (already advanced) and re-scan as identifier. */
  {
    /* Un-advance: step back one byte */
    if (lexer->current > 0)
      lexer->current--;
    size_t remaining = lexer->source_length - lexer->current;
    int ulen = unit_unicode_len(lexer->source + lexer->current, remaining);
    if (ulen > 0) {
      for (int i = 0; i < ulen; i++)
        advance(lexer);
      return scan_identifier(lexer, start);
    }
    /* Not a known Unicode unit char — re-advance and report error */
    advance(lexer);
  }

  // Unknown character
  return token_error_local("Unexpected character", lexer->line, lexer->column);
}

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ══════════════════════════════════════════════════════════════════════════════

nova_lexer_t *nova_lexer_create(const char *source) {
  nova_lexer_t *lexer = (nova_lexer_t *)malloc(sizeof(nova_lexer_t));
  if (!lexer)
    return NULL;

  lexer->source = source;
  lexer->source_length = strlen(source);
  lexer->current = 0;
  lexer->line = 1;
  lexer->column = 1;

  return lexer;
}

nova_token_t nova_lexer_next(nova_lexer_t *lexer) { return scan_token(lexer); }

nova_token_t nova_lexer_peek(nova_lexer_t *lexer) {
  size_t saved_current = lexer->current;
  int saved_line = lexer->line;
  int saved_column = lexer->column;

  nova_token_t token = scan_token(lexer);

  // Restore position
  lexer->current = saved_current;
  lexer->line = saved_line;
  lexer->column = saved_column;

  return token;
}

void nova_lexer_destroy(nova_lexer_t *lexer) { free(lexer); }
const char *nova_token_type_name(nova_token_type_t type) {
  switch (type) {
  case TOKEN_EOF:
    return "EOF";
  case TOKEN_ERROR:
    return "ERROR";
  case TOKEN_IDENT:
    return "IDENT";
  case TOKEN_LIT_INT:
    return "LIT_INT";
  case TOKEN_LIT_FLOAT:
    return "LIT_FLOAT";
  case TOKEN_LIT_STR:
    return "LIT_STR";
  case TOKEN_LIT_CHAR:
    return "LIT_CHAR";
  case TOKEN_LIT_TRUE:
    return "LIT_TRUE";
  case TOKEN_LIT_FALSE:
    return "LIT_FALSE";
  case TOKEN_LIT_UNIT:
    return "LIT_UNIT";
  case TOKEN_IDENTIFIER:
    return "IDENTIFIER";
  case TOKEN_NUMBER:
    return "NUMBER";
  case TOKEN_STRING:
    return "STRING";
  case TOKEN_CHAR:
    return "CHAR";
  case TOKEN_KEYWORD_FN:
    return "FN";
  case TOKEN_KEYWORD_LET:
    return "LET";
  case TOKEN_KEYWORD_CONST:
    return "CONST";
  case TOKEN_KEYWORD_VAR:
    return "VAR";
  case TOKEN_KEYWORD_IF:
    return "IF";
  case TOKEN_KEYWORD_CHECK:
    return "CHECK";
  case TOKEN_KEYWORD_ELSE:
    return "ELSE";
  case TOKEN_KEYWORD_WHILE:
    return "WHILE";
  case TOKEN_KEYWORD_FOR:
    return "FOR";
  case TOKEN_KEYWORD_RETURN:
    return "RETURN";
  case TOKEN_KEYWORD_BREAK:
    return "BREAK";
  case TOKEN_KEYWORD_CONTINUE:
    return "CONTINUE";
  case TOKEN_KEYWORD_LOOP:
    return "LOOP";
  case TOKEN_KEYWORD_STRUCT:
    return "STRUCT";
  case TOKEN_KEYWORD_ENUM:
    return "ENUM";
  case TOKEN_KEYWORD_TRAIT:
    return "TRAIT";
  case TOKEN_KEYWORD_DATA:
    return "DATA";
  case TOKEN_KEYWORD_CASES:
    return "CASES";
  case TOKEN_KEYWORD_RULES:
    return "RULES";
  case TOKEN_KEYWORD_YIELD:
    return "YIELD";
  case TOKEN_KEYWORD_EACH:
    return "EACH";
  case TOKEN_KEYWORD_FLOW:
    return "FLOW";
  case TOKEN_KEYWORD_SPAWN:
    return "SPAWN";
  case TOKEN_KEYWORD_GIVEN:
    return "GIVEN";
  case TOKEN_KEYWORD_OPEN:
    return "OPEN";
  case TOKEN_KEYWORD_ALIAS:
    return "ALIAS";
  case TOKEN_KEYWORD_ON:
    return "ON";
  case TOKEN_KEYWORD_IMPL:
    return "IMPL";
  case TOKEN_KEYWORD_TYPE:
    return "TYPE";
  case TOKEN_KEYWORD_IMPORT:
    return "IMPORT";
  case TOKEN_KEYWORD_EXPORT:
    return "EXPORT";
  case TOKEN_KEYWORD_PUB:
    return "PUB";
  case TOKEN_KEYWORD_MOD:
    return "MOD";
  case TOKEN_KEYWORD_AS:
    return "AS";
  case TOKEN_KEYWORD_USE:
    return "USE";
  case TOKEN_KEYWORD_MATCH:
    return "MATCH";
  case TOKEN_KEYWORD_IN:
    return "IN";
  case TOKEN_KEYWORD_EXPOSE:
    return "EXPOSE";
  case TOKEN_KEYWORD_CRATE:
    return "CRATE";
  case TOKEN_KEYWORD_SELF:
    return "SELF";
  case TOKEN_KEYWORD_BASE:
    return "BASE";
  case TOKEN_KEYWORD_SUPER:
    return "SUPER";
  case TOKEN_KEYWORD_ASYNC:
    return "ASYNC";
  case TOKEN_KEYWORD_AWAIT:
    return "AWAIT";
  case TOKEN_KEYWORD_ACTOR:
    return "ACTOR";
  case TOKEN_KEYWORD_HEAP:
    return "HEAP";
  case TOKEN_KEYWORD_FREE:
    return "FREE";
  case TOKEN_KEYWORD_NEW:
    return "NEW";
  case TOKEN_KEYWORD_SHAPE:
    return "SHAPE";
  case TOKEN_KEYWORD_SKILL:
    return "SKILL";
  case TOKEN_KEYWORD_UNIT:
    return "UNIT";
  case TOKEN_KEYWORD_QTY:
    return "QTY";
  case TOKEN_KEYWORD_DIMS:
    return "DIMS";
  case TOKEN_KEYWORD_TENSOR:
    return "TENSOR";
  case TOKEN_KEYWORD_KERNEL:
    return "KERNEL";
  case TOKEN_KEYWORD_BRING:
    return "BRING";
  case TOKEN_KEYWORD_APPLY:
    return "APPLY";
  case TOKEN_KEYWORD_DERIVES:
    return "DERIVES";
  case TOKEN_KEYWORD_KIND:
    return "KIND";
  case TOKEN_KEYWORD_MUT:
    return "MUT";
  case TOKEN_KEYWORD_REQUIRE:
    return "REQUIRE";
  case TOKEN_KEYWORD_ENSURE:
    return "ENSURE";
  case TOKEN_KEYWORD_TRY:
    return "TRY";
  case TOKEN_KEYWORD_CATCH:
    return "CATCH";
  case TOKEN_KEYWORD_FOREIGN:
    return "FOREIGN";
  case TOKEN_KEYWORD_COMPONENT:
    return "COMPONENT";
  case TOKEN_KEYWORD_VIEW:
    return "VIEW";
  case TOKEN_KEYWORD_PARALLEL:
    return "PARALLEL";
  case TOKEN_KEYWORD_SYNC:
    return "SYNC";
  case TOKEN_KEYWORD_THREAD:
    return "THREAD";
  case TOKEN_KEYWORD_BLOCK:
    return "BLOCK";
  case TOKEN_KEYWORD_GRAD:
    return "GRAD";
  case TOKEN_KEYWORD_DIFF:
    return "DIFF";
  case TOKEN_KEYWORD_INTEGRAL:
    return "INTEGRAL";
  case TOKEN_KEYWORD_SUM:
    return "SUM";
  case TOKEN_KEYWORD_PROD:
    return "PROD";
  case TOKEN_KEYWORD_LIMIT:
    return "LIMIT";
  case TOKEN_KEYWORD_SOLVE:
    return "SOLVE";
  case TOKEN_KEYWORD_SPACE:
    return "SPACE";
  case TOKEN_PLUS:
    return "PLUS";
  case TOKEN_MINUS:
    return "MINUS";
  case TOKEN_STAR:
    return "STAR";
  case TOKEN_STAR_STAR:
    return "STAR_STAR";
  case TOKEN_SLASH:
    return "SLASH";
  case TOKEN_PERCENT:
    return "PERCENT";
  case TOKEN_EQ:
    return "EQ";
  case TOKEN_EQ_EQ:
    return "EQ_EQ";
  case TOKEN_BANG:
    return "BANG";
  case TOKEN_BANG_EQ:
    return "BANG_EQ";
  case TOKEN_LT:
    return "LT";
  case TOKEN_LT_EQ:
    return "LT_EQ";
  case TOKEN_GT:
    return "GT";
  case TOKEN_GT_EQ:
    return "GT_EQ";
  case TOKEN_AMPERSAND:
    return "AMPERSAND";
  case TOKEN_AMPERSAND_AMPERSAND:
    return "AMPERSAND_AMPERSAND";
  case TOKEN_AMP_AMP:
    return "AMP_AMP";
  case TOKEN_PIPE:
    return "PIPE";
  case TOKEN_PIPE_PIPE:
    return "PIPE_PIPE";
  case TOKEN_CARET:
    return "CARET";
  case TOKEN_TILDE:
    return "TILDE";
  case TOKEN_LPAREN:
    return "LPAREN";
  case TOKEN_RPAREN:
    return "RPAREN";
  case TOKEN_LBRACE:
    return "LBRACE";
  case TOKEN_RBRACE:
    return "RBRACE";
  case TOKEN_LBRACKET:
    return "LBRACKET";
  case TOKEN_RBRACKET:
    return "RBRACKET";
  case TOKEN_SEMICOLON:
    return "SEMICOLON";
  case TOKEN_COMMA:
    return "COMMA";
  case TOKEN_DOT:
    return "DOT";
  case TOKEN_DOT_DOT:
    return "DOT_DOT";
  case TOKEN_COLON:
    return "COLON";
  case TOKEN_COLON_COLON:
    return "COLON_COLON";
  case TOKEN_ARROW:
    return "ARROW";
  case TOKEN_FAT_ARROW:
    return "FAT_ARROW";
  case TOKEN_QUESTION:
    return "QUESTION";
  case TOKEN_HASH:
    return "HASH";
  case TOKEN_AT:
    return "AT";
  default:
    return "UNKNOWN";
  }
}
