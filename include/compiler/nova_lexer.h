/**
 * Nova Language Lexer (C Implementation)
 * High-performance tokenization for Nova source code
 *
 * Performance target: 5M+ lines/second
 */

#ifndef NOVA_LEXER_H
#define NOVA_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Token Types
typedef enum {
  // Literals
  TOKEN_INTEGER,
  TOKEN_FLOAT,
  TOKEN_STRING,
  TOKEN_BOOLEAN,
  TOKEN_NULL,

  // Identifiers
  TOKEN_IDENTIFIER,

  // Keywords - Control Flow
  TOKEN_FN,
  TOKEN_DEF,
  TOKEN_RETURN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_ELIF,
  TOKEN_WHILE,
  TOKEN_FOR,
  TOKEN_IN,
  TOKEN_BREAK,
  TOKEN_CONTINUE,
  TOKEN_MATCH,
  TOKEN_CASE,

  // Keywords - Async/Concurrency
  TOKEN_ASYNC,
  TOKEN_AWAIT,
  TOKEN_SPAWN,
  TOKEN_DEFER,
  TOKEN_YIELD,

  // Keywords - Types
  TOKEN_CLASS,
  TOKEN_STRUCT,
  TOKEN_ENUM,
  TOKEN_TRAIT,
  TOKEN_IMPL,
  TOKEN_TYPE,

  // Keywords - Variables
  TOKEN_LET,
  TOKEN_VAR,
  TOKEN_MUT,
  TOKEN_CONST,
  TOKEN_STATIC,

  // Keywords - Modifiers
  TOKEN_PUB,
  TOKEN_PRIV,
  TOKEN_UNSAFE,
  TOKEN_EXTERN,

  // Keywords - Special
  TOKEN_USE,
  TOKEN_IMPORT,
  TOKEN_FROM,
  TOKEN_AS,
  TOKEN_WITH,
  TOKEN_TRY,
  TOKEN_CATCH,
  TOKEN_THROW,
  TOKEN_FINALLY,
  TOKEN_VERIFY,
  TOKEN_REQUIRES,
  TOKEN_ENSURES,
  TOKEN_FORALL,
  TOKEN_EXISTS,
  TOKEN_PROOF,
  TOKEN_LIFETIME,  // 'a, 'static (Rust-style lifetimes)

  // Web UI - Decorators
  TOKEN_COMPONENT, // @component
  TOKEN_STYLE,     // @style
  TOKEN_THEME,     // @theme

  // Web UI - HTML/JSX
  TOKEN_HTML_TAG_OPEN,       // <div
  TOKEN_HTML_TAG_CLOSE,      // >
  TOKEN_HTML_TAG_SELF_CLOSE, // />
  TOKEN_HTML_CLOSE_TAG,      // </div>
  TOKEN_JSX_EXPR_START,      // { (in JSX context)
  TOKEN_JSX_EXPR_END,        // } (in JSX context)

  // Web UI - CSS
  TOKEN_CSS_TEMPLATE, // css`...`
  TOKEN_STYLED,       // styled

  // Operators - Arithmetic
  TOKEN_PLUS,    // +
  TOKEN_MINUS,   // -
  TOKEN_STAR,    // *
  TOKEN_SLASH,   // /
  TOKEN_PERCENT, // %
  TOKEN_POWER,   // **

  // Operators - Comparison
  TOKEN_EQ, // ==
  TOKEN_NE, // !=
  TOKEN_LT, // <
  TOKEN_LE, // <=
  TOKEN_GT, // >
  TOKEN_GE, // >=

  // Operators - Logical
  TOKEN_AND, // &&
  TOKEN_OR,  // ||
  TOKEN_NOT, // not / !

  // Operators - Bitwise
  TOKEN_BIT_AND,    // &
  TOKEN_BIT_OR,     // |
  TOKEN_BIT_XOR,    // ^
  TOKEN_BIT_NOT,    // ~
  TOKEN_BIT_LSHIFT, // <<
  TOKEN_BIT_RSHIFT, // >>

  // Operators - Assignment
  TOKEN_ASSIGN,   // =
  TOKEN_PLUS_EQ,  // +=
  TOKEN_MINUS_EQ, // -=
  TOKEN_STAR_EQ,  // *=
  TOKEN_SLASH_EQ, // /=

  // Delimiters
  TOKEN_LPAREN,    // (
  TOKEN_RPAREN,    // )
  TOKEN_LBRACE,    // {
  TOKEN_RBRACE,    // }
  TOKEN_LBRACKET,  // [
  TOKEN_RBRACKET,  // ]
  TOKEN_COMMA,     // ,
  TOKEN_DOT,       // .
  TOKEN_COLON,     // :
  TOKEN_COLON_COLON, // ::
  TOKEN_SEMICOLON, // ;
  TOKEN_ARROW,     // ->
  TOKEN_FAT_ARROW, // =>
  TOKEN_QUESTION,  // ?
  TOKEN_PIPE,      // |

  // Special
  TOKEN_NEWLINE,
  TOKEN_EOF,
  TOKEN_INDENT,      // Python-style indentation increase
  TOKEN_DEDENT,      // Python-style indentation decrease
  TOKEN_HASH,        // #
  TOKEN_MACRO_RULES, // macro_rules
  TOKEN_SELF,        // self

  TOKEN_ERROR
} TokenType;

// Token structure
typedef struct {
  TokenType type;
  char *value; // Lexeme (owned, must be freed)
  size_t line;
  size_t column;
  size_t length;
} Token;

// Lexer structure
typedef struct {
  const char *source; // Source code (not owned)
  size_t source_len;
  size_t pos;    // Current position
  size_t line;   // Current line (1-indexed)
  size_t column; // Current column (1-indexed)
  char current;  // Current character
  bool is_eof;

  // Web UI state
  bool in_html_mode; // Currently lexing HTML
  bool in_css_mode;  // Currently lexing CSS
  int jsx_depth;     // Nesting depth of JSX expressions
} Lexer;

// Lexer API
Lexer *lexer_create(const char *source, size_t length);
void lexer_destroy(Lexer *lexer);

Token *lexer_next_token(Lexer *lexer);
void token_destroy(Token *token);

// Create token (public for Python lexer)
Token *make_token(TokenType type, const char *value, size_t line,
                  size_t column);

// Token utilities
const char *token_type_to_string(TokenType type);
void token_print(const Token *token);

// Keyword lookup
bool is_keyword(const char *str, TokenType *out_type);

#endif // NOVA_LEXER_H

// Main tokenization function
Token **lexer_tokenize(Lexer *lex, size_t *token_count);
