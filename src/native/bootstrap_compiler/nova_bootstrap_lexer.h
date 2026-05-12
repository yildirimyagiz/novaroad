/**
 * @file nova_bootstrap_lexer.h
 * @brief Bootstrap lexer header
 */

#ifndef NOVA_BOOTSTRAP_LEXER_H
#define NOVA_BOOTSTRAP_LEXER_H

typedef enum {
    // Meta
    TOK_EOF,
    TOK_ERROR,
    
    // Literals
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    
    // Nova Core Keywords (data/type system)
    TOK_FN,        // fn
    TOK_DATA,      // data (Nova's struct)
    TOK_CASES,     // cases (Nova's enum)
    TOK_RULES,     // rules (Nova's trait)
    TOK_SKILL,     // skill (Nova's impl)
    TOK_FLOW,      // flow (reactive pipeline)
    TOK_SPACE,     // space (module)
    TOK_GIVEN,     // given (compile-time const)
    TOK_TAG,       // tag (newtype wrapper)
    TOK_ALIAS,     // alias (type alias)
    
    // Variables & Visibility
    TOK_LET,       // let (immutable)
    TOK_VAR,       // var (mutable)
    TOK_OPEN,      // open (public)
    TOK_EXPOSE,    // expose (export)
    TOK_DERIVES,   // derives (trait derivation)
    
    // Control Flow (Nova-specific)
    TOK_CHECK,     // check (Nova's if)
    TOK_ELSE,      // else
    TOK_MATCH,     // match
    TOK_EACH,      // each (Nova's for)
    TOK_FOR,       // for (skill target)
    TOK_IN,        // in
    TOK_LOOP,      // loop (infinite)
    TOK_WHILE,     // while
    TOK_YIELD,     // yield (Nova's return)
    TOK_ABORT,     // abort (Nova's break)
    TOK_NEXT,      // next (Nova's continue)
    
    // Module System
    TOK_USE,       // use (import)
    TOK_FROM,      // from
    TOK_AS,        // as
    
    // Async/Concurrency
    TOK_ASYNC,     // async
    TOK_AWAIT,     // await
    TOK_SPAWN,     // spawn
    
    // Error Handling
    TOK_TRY,       // try
    TOK_CATCH,     // catch
    
    // Contracts (Design by Contract)
    TOK_REQUIRE,   // require (precondition)
    TOK_ENSURE,    // ensure (postcondition)
    TOK_ASSERT,    // assert (runtime check)
    
    // Delimiters
    TOK_LPAREN,    // (
    TOK_RPAREN,    // )
    TOK_LBRACE,    // {
    TOK_RBRACE,    // }
    TOK_LBRACKET,  // [
    TOK_RBRACKET,  // ]
    TOK_SEMICOLON, // ;
    TOK_COLON,     // :
    TOK_COMMA,     // ,
    TOK_DOT,       // .
    
    // Operators
    TOK_PLUS,         // +
    TOK_MINUS,        // -
    TOK_STAR,         // *
    TOK_SLASH,        // /
    TOK_PERCENT,      // %
    TOK_EQUAL,        // =
    TOK_EQUAL_EQUAL,  // ==
    TOK_BANG,         // !
    TOK_BANG_EQUAL,   // !=
    TOK_LESS,         // <
    TOK_LESS_EQUAL,   // <=
    TOK_GREATER,      // >
    TOK_GREATER_EQUAL,// >=
    TOK_ARROW,        // ->
    TOK_FAT_ARROW,    // =>
    TOK_PIPE,         // |
    TOK_AMPERSAND,    // &
    TOK_DOUBLE_COLON  // ::
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
    int column;
} Token;

typedef struct {
    const char *source;
    const char *current;
    int line;
    int column;
} Lexer;

Lexer *lexer_create(const char *source);
void lexer_destroy(Lexer *lex);
Token lexer_next_token(Lexer *lex);
const char *token_type_name(TokenType type);

#endif /* NOVA_BOOTSTRAP_LEXER_H */
