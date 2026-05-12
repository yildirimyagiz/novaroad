/**
 * @file parser.c
 * @brief Parser implementation for Nova language
 *

 */

#include "compiler/parser.h"
#include "compiler/ast.h"
#include "runtime/nova_execution_fabric.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ══════════════════════════════════════════════════════════════════════════════
// INTERNAL PARSER STRUCTURE
// ══════════════════════════════════════════════════════════════════════════════

struct nova_parser {
  nova_lexer_t *lexer;
  nova_token_t current;
  nova_token_t previous;
  char *error_message;
  bool inhibit_struct_init; // Prevents ident { } being parsed as struct init
                            // (for match/if/while)
  char *current_type_prefix;
};

// ══════════════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

static nova_location_t token_location(nova_token_t tok) {
  return nova_location(tok.line, tok.column);
}

static void advance(nova_parser_t *parser) {
  parser->previous = parser->current;
  parser->current = nova_lexer_next(parser->lexer);
}

static bool check(nova_parser_t *parser, nova_token_type_t type) {
  return parser->current.type == type;
}

static nova_token_type_t peek_next(nova_parser_t *parser) {
  return nova_lexer_peek(parser->lexer).type;
}

static bool match(nova_parser_t *parser, nova_token_type_t type) {
  if (check(parser, type)) {
    advance(parser);
    return true;
  }
  return false;
}

static void skip_block(nova_parser_t *parser) {
  if (!match(parser, TOKEN_LBRACE))
    return;
  int depth = 1;
  while (depth > 0 && !check(parser, TOKEN_EOF)) {
    if (check(parser, TOKEN_LBRACE)) {
      depth++;
      advance(parser);
    } else if (check(parser, TOKEN_RBRACE)) {
      depth--;
      advance(parser);
    } else if (check(parser, TOKEN_LIT_STR)) {
      advance(parser); // Skip strings entirely
    } else {
      advance(parser);
    }
  }
}

static void set_error(nova_parser_t *parser, const char *message) {
  if (parser->error_message)
    free(parser->error_message);
  char buf[512];
  snprintf(buf, sizeof(buf), "%s (at '%*.*s')", message,
           (int)parser->current.length, (int)parser->current.length,
           parser->current.start);
  parser->error_message = strdup(buf);
}

static nova_expr_t *parse_error(nova_parser_t *parser, const char *message) {
  set_error(parser, message);
  return NULL;
}

static nova_stmt_t *stmt_error(nova_parser_t *parser, const char *message) {
  set_error(parser, message);
  return NULL;
}

/**
 * match_ident_or_contextual_keyword:
 * Accept TOKEN_IDENT OR certain contextual keywords that are valid as
 * identifiers in certain positions (e.g., struct field names, variable names).
 * Keywords like 'data', 'kind', 'unit', etc. should be usable as identifiers
 * in non-statement-leading positions.
 */
static bool is_contextual_keyword(nova_token_type_t type) {
  switch (type) {
  case TOKEN_KEYWORD_DATA:
  case TOKEN_KEYWORD_KIND:
  case TOKEN_KEYWORD_UNIT:
  case TOKEN_KEYWORD_QTY:
  case TOKEN_KEYWORD_TENSOR:
  case TOKEN_KEYWORD_KERNEL:
  case TOKEN_KEYWORD_SKILL:
  case TOKEN_KEYWORD_OPEN:
  case TOKEN_KEYWORD_BASE:
  case TOKEN_KEYWORD_EACH:
  /* TOKEN_KEYWORD_IN is NOT contextual — it's used as a unit conversion
   * operator in expression context: `mass in kg`. Making it contextual would
   * cause parse_primary to consume 'in' as a variable name instead of an
   * operator. */
  case TOKEN_KEYWORD_USE:
  case TOKEN_KEYWORD_MATCH:
  case TOKEN_KEYWORD_APPLY:
  case TOKEN_KEYWORD_DERIVES:
  case TOKEN_KEYWORD_EXPOSE:
  case TOKEN_KEYWORD_BRING:
  case TOKEN_KEYWORD_ACTOR:
  case TOKEN_KEYWORD_CRATE:
  case TOKEN_KEYWORD_SELF:
  case TOKEN_KEYWORD_SUPER:
  case TOKEN_KEYWORD_MUT:
  case TOKEN_KEYWORD_CHECK:
  case TOKEN_KEYWORD_ON:
  case TOKEN_KEYWORD_TRY:
  case TOKEN_KEYWORD_CATCH:
  case TOKEN_KEYWORD_AWAIT:
  case TOKEN_KEYWORD_COMPONENT:
  case TOKEN_KEYWORD_VIEW:
  case TOKEN_KEYWORD_SPACE:
  case TOKEN_KEYWORD_PARALLEL:
  case TOKEN_KEYWORD_SYNC:
  case TOKEN_KEYWORD_GRAD:
  case TOKEN_KEYWORD_DIFF:
  case TOKEN_KEYWORD_INTEGRAL:
  case TOKEN_KEYWORD_SUM:
  case TOKEN_KEYWORD_PROD:
  case TOKEN_KEYWORD_LIMIT:
  case TOKEN_KEYWORD_SOLVE:
  /* Keywords that are commonly used as function/method names */
  case TOKEN_KEYWORD_PUB:
  case TOKEN_KEYWORD_SHAPE:
  case TOKEN_KEYWORD_NEW:
  case TOKEN_KEYWORD_FREE:
  case TOKEN_KEYWORD_TYPE:
  case TOKEN_KEYWORD_IMPL:
  case TOKEN_KEYWORD_FN:
  case TOKEN_KEYWORD_LET:
  case TOKEN_KEYWORD_CONST:
  case TOKEN_KEYWORD_VAR:
  case TOKEN_KEYWORD_RETURN:
  case TOKEN_KEYWORD_YIELD:
  case TOKEN_KEYWORD_LOOP:
    return true;
  default:
    return false;
  }
}

static bool match_ident_or_contextual_keyword(nova_parser_t *parser) {
  if (check(parser, TOKEN_IDENT) ||
      is_contextual_keyword(parser->current.type)) {
    advance(parser);
    return true;
  }
  return false;
}

// ══════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

static nova_expr_t *parse_expression(nova_parser_t *parser);
static nova_stmt_t *parse_statement(nova_parser_t *parser);
static nova_stmt_t *parse_variable_declaration(nova_parser_t *parser);
static nova_stmt_t *parse_function_declaration(nova_parser_t *parser);
static nova_stmt_t *parse_check_statement(nova_parser_t *parser);
static nova_stmt_t *parse_yield_statement(nova_parser_t *parser);
static nova_stmt_t *parse_return_statement(nova_parser_t *parser);
static nova_stmt_t *parse_while_statement(nova_parser_t *parser);
static nova_stmt_t *parse_for_statement(nova_parser_t *parser);
static nova_stmt_t *parse_loop_statement(nova_parser_t *parser);
static nova_stmt_t *parse_block(nova_parser_t *parser);
static nova_stmt_t *parse_struct_declaration(nova_parser_t *parser);
static nova_stmt_t *parse_enum_declaration(nova_parser_t *parser);
static nova_stmt_t *parse_module_declaration(nova_parser_t *parser);
static nova_stmt_t *parse_import_declaration(nova_parser_t *parser);
static nova_stmt_t *parse_public_declaration(nova_parser_t *parser);
static nova_stmt_t *parse_each_statement(nova_parser_t *parser);
static nova_stmt_t *parse_spawn_statement(nova_parser_t *parser);
static nova_stmt_t *parse_foreign_block(nova_parser_t *parser);
static nova_type_t *parse_type(nova_parser_t *parser);
static nova_expr_t *parse_postfix(nova_parser_t *parser);
static nova_expr_t *parse_heap_expression(nova_parser_t *parser);
static nova_pattern_t *parse_pattern(nova_parser_t *parser);
static nova_expr_t *parse_match_expression(nova_parser_t *parser);

// ══════════════════════════════════════════════════════════════════════════════
// EXPRESSION PARSING
//
//  Öncelik zinciri (düşükten yükseğe):
//   expression → assignment
//   assignment → comparison ( "=" assignment )?
//   comparison → term ( ("<"|">"|"<="|">="|"=="|"!=") term )*
//   term       → factor  ( ("+"|"-") factor )*
//   factor     → unary   ( ("*"|"/")  unary )*
//   unary      → ("-"|"&"|"*") unary | postfix
//   postfix    → primary ( "(" args ")" | "." IDENT )*
//   primary    → literal | IDENT | struct_init | "(" expr ")"
// ══════════════════════════════════════════════════════════════════════════════

static nova_expr_t *parse_primary(nova_parser_t *parser) {

  if (match(parser, TOKEN_LIT_INT)) {
    long long value = strtoll(parser->previous.start, NULL, 0);
    nova_location_t int_loc = token_location(parser->previous);
    if (check(parser, TOKEN_DOT)) {
      nova_token_t peek = nova_lexer_peek(parser->lexer);
      if (peek.type == TOKEN_IDENT) {
        advance(parser);
        advance(parser);
        char unit_buf[128];
        size_t unit_len = parser->previous.length;
        if (unit_len >= sizeof(unit_buf))
          unit_len = sizeof(unit_buf) - 1;
        memcpy(unit_buf, parser->previous.start, unit_len);
        unit_buf[unit_len] = '\0';
        while (check(parser, TOKEN_SLASH)) {
          nova_token_t after_slash = nova_lexer_peek(parser->lexer);
          if (after_slash.type != TOKEN_IDENT)
            break;
          advance(parser);
          advance(parser);
          size_t rem = sizeof(unit_buf) - strlen(unit_buf) - 1;
          strncat(unit_buf, "/", rem);
          rem = sizeof(unit_buf) - strlen(unit_buf) - 1;
          size_t plen = parser->previous.length;
          if (plen > rem)
            plen = rem;
          strncat(unit_buf, parser->previous.start, plen);
        }
        return nova_expr_unit_literal((double)value, unit_buf, int_loc);
      }
    }
    return nova_expr_lit_int(value, int_loc);
  }

  if (match(parser, TOKEN_LIT_FLOAT)) {
    double value = strtod(parser->previous.start, NULL);
    nova_location_t float_loc = token_location(parser->previous);
    if (check(parser, TOKEN_DOT)) {
      nova_token_t peek = nova_lexer_peek(parser->lexer);
      if (peek.type == TOKEN_IDENT) {
        advance(parser);
        advance(parser);
        char unit_buf[128];
        size_t unit_len = parser->previous.length;
        if (unit_len >= sizeof(unit_buf))
          unit_len = sizeof(unit_buf) - 1;
        memcpy(unit_buf, parser->previous.start, unit_len);
        unit_buf[unit_len] = '\0';
        while (check(parser, TOKEN_SLASH)) {
          nova_token_t after_slash = nova_lexer_peek(parser->lexer);
          if (after_slash.type != TOKEN_IDENT)
            break;
          advance(parser);
          advance(parser);
          size_t rem = sizeof(unit_buf) - strlen(unit_buf) - 1;
          strncat(unit_buf, "/", rem);
          rem = sizeof(unit_buf) - strlen(unit_buf) - 1;
          size_t plen = parser->previous.length;
          if (plen > rem)
            plen = rem;
          strncat(unit_buf, parser->previous.start, plen);
        }
        return nova_expr_unit_literal(value, unit_buf, float_loc);
      }
    }
    return nova_expr_lit_float(value, float_loc);
  }

  if (match(parser, TOKEN_LIT_STR)) {
    size_t len = parser->previous.length - 2;
    char *str = strndup(parser->previous.start + 1, len);
    nova_location_t loc = token_location(parser->previous);
    nova_expr_t **parts = NULL;
    size_t part_count = 0, i = 0, start = 0;
    while (i < len) {
      if (str[i] == '{') {
        if (i > start) {
          char *literal = strndup(str + start, i - start);
          parts = realloc(parts, sizeof(nova_expr_t *) * (part_count + 1));
          parts[part_count++] = nova_expr_lit_str(literal, loc);
          free(literal);
        }
        size_t expr_start = i + 1;
        size_t depth = 1;
        i++;
        while (i < len && depth > 0) {
          if (str[i] == '{')
            depth++;
          else if (str[i] == '}')
            depth--;
          i++;
        }
        if (depth == 0) {
          size_t expr_len = i - expr_start - 1;
          char *expr_str = strndup(str + expr_start, expr_len);
          parts = realloc(parts, sizeof(nova_expr_t *) * (part_count + 1));
          /* Empty {} or format specifiers like {:?} {:.1} — treat as literal
           * placeholder */
          if (expr_len == 0 || expr_str[0] == ':') {
            parts[part_count++] = nova_expr_lit_str("{}", loc);
          } else {
            parts[part_count++] = nova_expr_ident(expr_str, loc);
          }
          free(expr_str);
          start = i;
        } else {
          free(str);
          for (size_t j = 0; j < part_count; j++)
            nova_expr_free(parts[j]);
          free(parts);
          return parse_error(parser, "Unclosed '{' in string interpolation");
        }
      } else {
        i++;
      }
    }
    if (start < len) {
      char *literal = strndup(str + start, len - start);
      parts = realloc(parts, sizeof(nova_expr_t *) * (part_count + 1));
      parts[part_count++] = nova_expr_lit_str(literal, loc);
      free(literal);
    }
    free(str);
    if (part_count == 0)
      return nova_expr_lit_str("", loc);
    else if (part_count == 1) {
      nova_expr_t *res = parts[0];
      free(parts);
      return res;
    }
    nova_expr_t *res = parts[0];
    for (size_t j = 1; j < part_count; j++)
      res = nova_expr_binary(res, "+", parts[j], loc);
    free(parts);
    return res;
  }

  if (match(parser, TOKEN_LIT_TRUE))
    return nova_expr_lit_bool(true, token_location(parser->previous));
  if (match(parser, TOKEN_LIT_FALSE))
    return nova_expr_lit_bool(false, token_location(parser->previous));

  if (match(parser, TOKEN_LIT_CHAR)) {
    /* Char literal: 'x', '\n', or multi-byte '°' */
    const char *s = parser->previous.start + 1; /* skip opening quote */
    long long codepoint = 0;
    if (s[0] == '\\') {
      /* Basic escape handling */
      switch (s[1]) {
      case 'n':
        codepoint = '\n';
        break;
      case 't':
        codepoint = '\t';
        break;
      case 'r':
        codepoint = '\r';
        break;
      case '\\':
        codepoint = '\\';
        break;
      case '\'':
        codepoint = '\'';
        break;
      case '0':
        codepoint = '\0';
        break;
      default:
        codepoint = (unsigned char)s[1];
        break;
      }
    } else if ((unsigned char)s[0] >= 0x80) {
      /* Simple UTF-8 decoding for common cases like ° */
      unsigned char c0 = (unsigned char)s[0];
      if ((c0 & 0xE0) == 0xC0) {
        codepoint = ((c0 & 0x1F) << 6) | ((unsigned char)s[1] & 0x3F);
      } else if ((c0 & 0xF0) == 0xE0) {
        codepoint = ((c0 & 0x0F) << 12) | (((unsigned char)s[1] & 0x3F) << 6) |
                    ((unsigned char)s[2] & 0x3F);
      } else {
        codepoint = c0; /* fallback */
      }
    } else {
      codepoint = (unsigned char)s[0];
    }
    return nova_expr_lit_int(codepoint, token_location(parser->previous));
  }

  if (match_ident_or_contextual_keyword(parser)) {
    nova_token_t ident_tok = parser->previous;
    nova_location_t ident_loc = token_location(ident_tok);
    char *ident_name = strndup(ident_tok.start, ident_tok.length);

    if (match(parser, TOKEN_COLON_COLON)) {
      if (!match_ident_or_contextual_keyword(parser)) {
        if (parser->current.type >= TOKEN_KEYWORD_FN &&
            parser->current.type <= TOKEN_KEYWORD_SPACE)
          advance(parser);
        else {
          free(ident_name);
          return parse_error(parser, "Expected name after '::'");
        }
      }
      char *variant_name =
          strndup(parser->previous.start, parser->previous.length);
      nova_expr_t *lhs = nova_expr_ident(ident_name, ident_loc);
      nova_expr_t *res =
          nova_expr_namespaced_access(lhs, variant_name, ident_loc);
      free(ident_name);
      free(variant_name);
      return res;
    }

    if (!parser->inhibit_struct_init && match(parser, TOKEN_LBRACE)) {
      nova_expr_t *init_expr = nova_expr_struct_init(ident_name, ident_loc);
      while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        if (!match_ident_or_contextual_keyword(parser)) {
          nova_expr_free(init_expr);
          free(ident_name);
          return parse_error(parser, "Expected field name");
        }
        char *fname = strndup(parser->previous.start, parser->previous.length);
        nova_expr_t *fval = NULL;
        if (match(parser, TOKEN_COLON)) {
          fval = parse_expression(parser);
          if (!fval) {
            nova_expr_free(init_expr);
            free(ident_name);
            free(fname);
            return NULL;
          }
        } else
          fval = nova_expr_ident(fname, token_location(parser->previous));
        init_expr->data.struct_init.fields =
            realloc(init_expr->data.struct_init.fields,
                    sizeof(*init_expr->data.struct_init.fields) *
                        (init_expr->data.struct_init.field_count + 1));
        size_t idx = init_expr->data.struct_init.field_count++;
        init_expr->data.struct_init.fields[idx].name = fname;
        init_expr->data.struct_init.fields[idx].value = fval;
        if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACE))
          break;
      }
      match(parser, TOKEN_RBRACE);
      free(ident_name);
      return init_expr;
    }
    nova_expr_t *expr = nova_expr_ident(ident_name, ident_loc);
    free(ident_name);
    if (match(parser, TOKEN_LT)) {
      int depth = 1;
      while (depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LT))
          depth++;
        else if (match(parser, TOKEN_GT))
          depth--;
        else
          advance(parser);
      }
    }
    return expr;
  }

  if (match(parser, TOKEN_LPAREN)) {
    if (match(parser, TOKEN_RPAREN))
      return nova_expr_array_lit(NULL, 0, token_location(parser->previous));
    nova_expr_t *first = parse_expression(parser);
    if (!first)
      return NULL;
    if (!check(parser, TOKEN_COMMA)) {
      if (!match(parser, TOKEN_RPAREN)) {
        nova_expr_free(first);
        return parse_error(parser, "Expected ')'");
      }
      return first;
    }
    nova_expr_t **elements = malloc(sizeof(nova_expr_t *) * 8);
    size_t cap = 8, count = 0;
    elements[count++] = first;
    while (match(parser, TOKEN_COMMA)) {
      if (check(parser, TOKEN_RPAREN))
        break;
      nova_expr_t *elem = parse_expression(parser);
      if (!elem) {
        for (size_t i = 0; i < count; i++)
          nova_expr_free(elements[i]);
        free(elements);
        return NULL;
      }
      if (count >= cap) {
        cap *= 2;
        elements = realloc(elements, sizeof(nova_expr_t *) * cap);
      }
      elements[count++] = elem;
    }
    if (!match(parser, TOKEN_RPAREN)) {
      for (size_t i = 0; i < count; i++)
        nova_expr_free(elements[i]);
      free(elements);
      return parse_error(parser, "Expected ')'");
    }
    return nova_expr_array_lit(elements, count,
                               token_location(parser->previous));
  }

  if (match(parser, TOKEN_LBRACKET)) {
    nova_location_t loc = token_location(parser->previous);
    nova_expr_t **elements = NULL;
    size_t count = 0, cap = 0;
    while (!check(parser, TOKEN_RBRACKET) && !check(parser, TOKEN_EOF)) {
      nova_expr_t *elem = parse_expression(parser);
      if (!elem) {
        for (size_t i = 0; i < count; i++)
          nova_expr_free(elements[i]);
        if (elements)
          free(elements);
        return NULL;
      }
      if (count >= cap) {
        cap = cap == 0 ? 4 : cap * 2;
        elements = realloc(elements, sizeof(nova_expr_t *) * cap);
      }
      elements[count++] = elem;
      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACKET))
        break;
    }
    if (!match(parser, TOKEN_RBRACKET)) {
      for (size_t i = 0; i < count; i++)
        nova_expr_free(elements[i]);
      free(elements);
      return parse_error(parser, "Expected ']'");
    }
    return nova_expr_array_lit(elements, count, loc);
  }

  if (match(parser, TOKEN_KEYWORD_MATCH))
    return parse_match_expression(parser);
  if (check(parser, TOKEN_KEYWORD_IF) || check(parser, TOKEN_KEYWORD_WHILE) ||
      check(parser, TOKEN_KEYWORD_FOR))
    return parse_error(parser, "Keywords unsupported in expression");

  fprintf(stderr,
          "nova: Expected expression in parse_primary at line %d (token %d)\n",
          parser->current.line, parser->current.type);
  return parse_error(parser, "Expected expression in parse_primary");
}

/* postfix → primary ( "(" args ")" | "." IDENT ( "(" args ")" )? )*
 * Düzeltme: field access while döngüsünde — a.b.c zincirleme çalışır. */
static nova_expr_t *parse_postfix(nova_parser_t *parser) {
  nova_expr_t *expr = parse_primary(parser);
  if (!expr)
    return NULL;

  for (;;) {
    /* [Fix] Macro call suffix: identifier! (args), identifier! [args],
     * identifier! {args} For bootstrap, we strip ! and treat it as a regular
     * call. */
    if (match(parser, TOKEN_BANG)) {
      if (check(parser, TOKEN_LBRACKET)) {
        /* Macro with [ ] -> e.g. vec![1, 2, 3]
         * Parse as a call where the argument is an array literal. */
        nova_expr_t *array_lit = parse_primary(parser);
        if (!array_lit)
          return NULL;
        nova_expr_t **args = malloc(sizeof(nova_expr_t *));
        args[0] = array_lit;
        expr = nova_expr_call(expr, args, 1, token_location(parser->previous));
        continue;
      }
      if (check(parser, TOKEN_LBRACE)) {
        /* Macro with { } -> e.g. thread! { ... }
         * Parse as a call where the argument is a block expression. */
        nova_location_t b_loc = token_location(parser->current);
        nova_stmt_t *blk = parse_block(parser);
        if (!blk)
          return NULL;
        nova_expr_t *blk_expr = nova_expr_block(blk, b_loc);
        nova_expr_t **args = malloc(sizeof(nova_expr_t *));
        args[0] = blk_expr;
        expr = nova_expr_call(expr, args, 1, token_location(parser->previous));
        continue;
      }
      continue;
    }

    /* Fonksiyon çağrısı */
    if (match(parser, TOKEN_LPAREN)) {
      nova_expr_t **args = NULL;
      size_t arg_count = 0;
      size_t arg_capacity = 0;

      if (!check(parser, TOKEN_RPAREN)) {
        while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
          nova_expr_t *arg = parse_expression(parser);
          if (!arg) {
            for (size_t i = 0; i < arg_count; i++)
              nova_expr_free(args[i]);
            if (args)
              free(args);
            nova_expr_free(expr);
            return NULL;
          }
          if (arg_count >= arg_capacity) {
            arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
            args = realloc(args, sizeof(nova_expr_t *) * arg_capacity);
          }
          args[arg_count++] = arg;
          if (!match(parser, TOKEN_COMMA))
            break;
        }
      }

      if (!match(parser, TOKEN_RPAREN)) {
        for (size_t i = 0; i < arg_count; i++)
          nova_expr_free(args[i]);
        free(args);
        nova_expr_free(expr);
        return parse_error(parser, "Expected ')' after arguments");
      }

      expr = nova_expr_call(expr, args, arg_count,
                            token_location(parser->previous));
      continue;
    }

    /* Array indexing: arr[index] */
    if (match(parser, TOKEN_LBRACKET)) {
      nova_expr_t *index = parse_expression(parser);
      if (!index) {
        nova_expr_free(expr);
        return NULL;
      }
      if (!match(parser, TOKEN_RBRACKET)) {
        nova_expr_free(expr);
        nova_expr_free(index);
        return parse_error(parser, "Expected ']' after array index");
      }
      expr = nova_expr_index(expr, index, token_location(parser->previous));
      continue;
    }

    /* Alan erişimi / metot çağrısı */
    if (match(parser, TOKEN_DOT)) {
      // Tuple indexing: tuple.0, tuple.1, etc.
      if (match(parser, TOKEN_LIT_INT)) {
        long long index = strtoll(parser->previous.start, NULL, 10);
        nova_expr_t *index_expr =
            nova_expr_lit_int(index, token_location(parser->previous));
        expr =
            nova_expr_index(expr, index_expr, token_location(parser->previous));
        continue;
      }

      if (!match_ident_or_contextual_keyword(parser)) {
        nova_expr_free(expr);
        return parse_error(parser,
                           "Expected field name or tuple index after '.'");
      }
      char *field_name =
          strndup(parser->previous.start, parser->previous.length);

      if (strcmp(field_name, "len") == 0 && match(parser, TOKEN_LPAREN)) {
        if (!match(parser, TOKEN_RPAREN)) {
          free(field_name);
          nova_expr_free(expr);
          return parse_error(parser, "Expected ')' after len()");
        }
        free(field_name);
        expr = nova_expr_string_len(expr, token_location(parser->previous));
        continue;
      }

      if (strcmp(field_name, "slice") == 0) {
        if (!match(parser, TOKEN_LPAREN)) {
          free(field_name);
          nova_expr_free(expr);
          return parse_error(parser, "Expected '(' after slice");
        }
        nova_expr_t *start = parse_expression(parser);
        if (!start) {
          free(field_name);
          nova_expr_free(expr);
          return NULL;
        }
        if (!match(parser, TOKEN_COMMA)) {
          free(field_name);
          nova_expr_free(expr);
          nova_expr_free(start);
          return parse_error(parser, "Expected ',' in slice arguments");
        }
        nova_expr_t *end = parse_expression(parser);
        if (!end) {
          free(field_name);
          nova_expr_free(expr);
          nova_expr_free(start);
          return NULL;
        }
        if (!match(parser, TOKEN_RPAREN)) {
          free(field_name);
          nova_expr_free(expr);
          nova_expr_free(start);
          nova_expr_free(end);
          return parse_error(parser, "Expected ')' after slice arguments");
        }
        free(field_name);
        expr = nova_expr_string_slice(expr, start, end,
                                      token_location(parser->previous));
        continue;
      }

      if (strcmp(field_name, "await") == 0) {
        free(field_name);
        expr = nova_expr_await(expr, token_location(parser->previous));
        continue;
      }

      expr = nova_expr_field_access(expr, field_name,
                                    token_location(parser->previous));
      free(field_name);
      continue;
    }

    /* Namespaced access: object::member */
    if (match(parser, TOKEN_COLON_COLON)) {
      /* Handle ::* (glob) */
      if (match(parser, TOKEN_STAR)) {
        char *member_name = strdup("*");
        expr = nova_expr_namespaced_access(expr, member_name,
                                           token_location(parser->previous));
        free(member_name);
        continue;
      }
      /* Handle ::{...} (grouped import) */
      if (match(parser, TOKEN_LBRACE)) {
        int depth = 1;
        while (depth > 0 && !check(parser, TOKEN_EOF)) {
          if (match(parser, TOKEN_LBRACE))
            depth++;
          else if (match(parser, TOKEN_RBRACE))
            depth--;
          else
            advance(parser);
        }
        char *member_name = strdup("{}");
        expr = nova_expr_namespaced_access(expr, member_name,
                                           token_location(parser->previous));
        free(member_name);
        continue;
      }
      if (!match_ident_or_contextual_keyword(parser)) {
        // Allow keywords as members (common in Nova v10)
        if (parser->current.type >= TOKEN_KEYWORD_FN &&
            parser->current.type <= TOKEN_KEYWORD_MUT) {
          advance(parser);
        } else {
          nova_expr_free(expr);
          return parse_error(parser, "Expected member name after '::'");
        }
      }
      char *member_name =
          strndup(parser->previous.start, parser->previous.length);
      expr = nova_expr_namespaced_access(expr, member_name,
                                         token_location(parser->previous));
      free(member_name);
      continue;
    }

    /* Type cast: expr as Type */
    if (match(parser, TOKEN_KEYWORD_AS)) {
      nova_type_t *target_type = parse_type(parser);
      if (!target_type) {
        nova_expr_free(expr);
        return NULL;
      }
      expr =
          nova_expr_cast(expr, target_type, token_location(parser->previous));
      continue;
    }

    /* Try/unwrap operator: expr? — Stage 0: treat as no-op (pass through) */
    if (match(parser, TOKEN_QUESTION)) {
      /* Just consume '?' and continue — the expression is unchanged */
      continue;
    }

    break;
  }

  return expr;
}

/* unary → ("-" | "&" | "*") unary | postfix
 * Düzeltme: operatör lokasyonu operand parse edilmeden önce yakalanır. */
static nova_expr_t *parse_unary(nova_parser_t *parser) {
  if (match(parser, TOKEN_MINUS)) {
    nova_token_t op_tok = parser->previous;
    nova_expr_t *operand = parse_unary(parser);
    if (!operand)
      return NULL;

    nova_location_t loc = token_location(op_tok);
    nova_expr_t *zero = nova_expr_lit_int(0, loc);
    return nova_expr_binary(zero, "-", operand, loc);
  }

  if (match(parser, TOKEN_AMPERSAND)) {
    nova_token_t op_tok = parser->previous;
    /* Nova v10: Optional modifiers like &var, &mut, &let */
    while (match(parser, TOKEN_KEYWORD_VAR) ||
           match(parser, TOKEN_KEYWORD_MUT) ||
           match(parser, TOKEN_KEYWORD_LET)) {
    }
    nova_expr_t *operand = parse_unary(parser);
    if (!operand)
      return NULL;
    return nova_expr_addr_of(operand, token_location(op_tok));
  }

  if (match(parser, TOKEN_STAR)) {
    nova_token_t op_tok = parser->previous;
    /* Nova v10: Optional modifiers like *var, *mut, *const (keyword or ident)
     */
    while (match(parser, TOKEN_KEYWORD_VAR) ||
           match(parser, TOKEN_KEYWORD_MUT) ||
           match(parser, TOKEN_KEYWORD_LET) ||
           match(parser, TOKEN_KEYWORD_CONST)) {
    }
    nova_expr_t *operand = parse_unary(parser);
    if (!operand)
      return NULL;
    return nova_expr_deref(operand, token_location(op_tok));
  }

  if (check(parser, TOKEN_KEYWORD_HEAP)) {
    nova_token_t next = nova_lexer_peek(parser->lexer);
    if (next.type == TOKEN_KEYWORD_NEW) {
      advance(parser); // consume HEAP
      return parse_heap_expression(parser);
    }
  }

  if (match(parser, TOKEN_KEYWORD_AWAIT)) {
    nova_token_t op_tok = parser->previous;
    nova_expr_t *operand = parse_unary(parser);
    if (!operand)
      return NULL;
    return nova_expr_await(operand, token_location(op_tok));
  }

  if (match(parser, TOKEN_BANG) || match(parser, TOKEN_TILDE)) {
    nova_token_t op_tok = parser->previous;
    nova_expr_t *operand = parse_unary(parser);
    if (!operand)
      return NULL;
    return nova_expr_unary(operand, op_tok.type == TOKEN_BANG ? "!" : "~",
                           token_location(op_tok));
  }

  return parse_postfix(parser);
}

/* power → unary ("**" power)?   (right-associative) */
static nova_expr_t *parse_power(nova_parser_t *parser) {
  nova_expr_t *base = parse_unary(parser);
  if (!base)
    return NULL;

  if (match(parser, TOKEN_STAR_STAR)) {
    nova_expr_t *exp = parse_power(parser); /* right-assoc recursive */
    if (!exp) {
      nova_expr_free(base);
      return NULL;
    }
    return nova_expr_binary(base, "**", exp, token_location(parser->previous));
  }
  return base;
}

/* factor → power ( ("*" | "/") power )* */
static nova_expr_t *parse_factor(nova_parser_t *parser) {
  nova_expr_t *left = parse_power(parser);
  if (!left)
    return NULL;

  while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) ||
         match(parser, TOKEN_PERCENT)) {
    nova_token_t op_tok = parser->previous;
    const char *op_str = (op_tok.type == TOKEN_STAR)    ? "*"
                         : (op_tok.type == TOKEN_SLASH) ? "/"
                                                        : "%";

    nova_expr_t *right = parse_unary(parser);
    if (!right) {
      nova_expr_free(left);
      return NULL;
    }

    left = nova_expr_binary(left, op_str, right, token_location(op_tok));
  }
  return left;
}

/* term → factor ( ("+" | "-") factor )* */
static nova_expr_t *parse_term(nova_parser_t *parser) {
  nova_expr_t *left = parse_factor(parser);
  if (!left)
    return NULL;

  while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
    nova_token_t op_tok = parser->previous;
    const char *op_str = (op_tok.type == TOKEN_PLUS) ? "+" : "-";

    nova_expr_t *right = parse_factor(parser);
    if (!right) {
      nova_expr_free(left);
      return NULL;
    }

    left = nova_expr_binary(left, op_str, right, token_location(op_tok));
  }
  return left;
}

/* comparison → term ( relational_op term | "in" IDENT )* */
static nova_expr_t *parse_comparison(nova_parser_t *parser) {
  nova_expr_t *left = parse_term(parser);
  if (!left)
    return NULL;

  /* Unit conversion: expr in target_unit
   * e.g. 10.0.km in m  → converts 10 km to meters
   *      mass in g      → converts mass qty to grams
   * TOKEN_KEYWORD_IN is also used by for loops; here it's only matched
   * when it appears in expression context (parse_factor already consumed left).
   */
  /* Unit conversion: `expr in unit_name`
   * peek ahead — if current==TOKEN_KEYWORD_IN AND peek next token is IDENT,
   * then consume both and build EXPR_BINARY("in", left, ident). */
  while (check(parser, TOKEN_KEYWORD_IN)) {
    nova_token_t op_tok = parser->current;
    /* Use nova_lexer_peek to inspect token AFTER 'in' without advancing */
    nova_token_t next = nova_lexer_peek(parser->lexer);
    if (next.type != TOKEN_IDENT)
      break;         /* not a unit name — leave 'in' for for-loops */
    advance(parser); /* consume 'in' */
    nova_location_t op_loc = token_location(op_tok);
    /* Now parser->current == next IDENT token (unit name) — parse it */
    nova_expr_t *right = parse_primary(parser); /* just the ident */
    if (!right) {
      nova_expr_free(left);
      return NULL;
    }
    left = nova_expr_binary(left, "in", right, op_loc);
  }

  while (match(parser, TOKEN_GT) || match(parser, TOKEN_LT) ||
         match(parser, TOKEN_GT_EQ) || match(parser, TOKEN_LT_EQ) ||
         match(parser, TOKEN_EQ_EQ) || match(parser, TOKEN_BANG_EQ) ||
         match(parser, TOKEN_TILDE)) {

    nova_token_t op_tok = parser->previous;
    nova_token_type_t op = op_tok.type;

    nova_expr_t *right = parse_term(parser);
    if (!right) {
      nova_expr_free(left);
      return NULL;
    }

    const char *op_str;
    switch (op) {
    case TOKEN_GT:
      op_str = ">";
      break;
    case TOKEN_LT:
      op_str = "<";
      break;
    case TOKEN_GT_EQ:
      op_str = ">=";
      break;
    case TOKEN_LT_EQ:
      op_str = "<=";
      break;
    case TOKEN_EQ_EQ:
      op_str = "==";
      break;
    case TOKEN_BANG_EQ:
      op_str = "!=";
      break;
    case TOKEN_TILDE:
      op_str = "~"; /* approximate equality for unit quantities */
      break;
    default:
      op_str = "?";
      break;
    }

    left = nova_expr_binary(left, op_str, right, token_location(op_tok));
  }
  return left;
}

/* logical → comparison ( ("&&" | "||") comparison )* */
static nova_expr_t *parse_logical(nova_parser_t *parser) {
  nova_expr_t *left = parse_comparison(parser);
  if (!left)
    return NULL;

  while (match(parser, TOKEN_AMPERSAND_AMPERSAND) ||
         match(parser, TOKEN_PIPE_PIPE)) {
    nova_token_t op_tok = parser->previous;
    const char *op_str = (op_tok.type == TOKEN_PIPE_PIPE) ? "||" : "&&";
    nova_expr_t *right = parse_comparison(parser);
    if (!right) {
      nova_expr_free(left);
      return NULL;
    }
    left = nova_expr_binary(left, op_str, right, token_location(op_tok));
  }
  return left;
}

/* assignment → logical ( "=" assignment )?
 * Not: AST assign node name-tabanlıysa yalnız IDENT güvenli.
 * Deref/field assignment için AST'de "target expression" desteği gerekir. */
static nova_expr_t *parse_assignment(nova_parser_t *parser) {
  nova_expr_t *expr = parse_logical(parser);
  if (!expr)
    return NULL;

  if (match(parser, TOKEN_EQ)) {
    nova_expr_t *value = parse_assignment(parser);
    if (!value) {
      nova_expr_free(expr);
      return NULL;
    }

    if (expr->kind == EXPR_IDENT) {
      /* Simple variable assignment */
      nova_expr_t *assign = calloc(1, sizeof(nova_expr_t));
      memset(assign, 0, sizeof(nova_expr_t));
      assign->kind = EXPR_ASSIGN;
      assign->span = expr->span;
      assign->data.assign.name = strdup(expr->data.ident);
      assign->data.assign.value = value;

      nova_expr_free(expr);
      return assign;
    }

    if (expr->kind == EXPR_FIELD_ACCESS || expr->kind == EXPR_DEREF ||
        expr->kind == EXPR_INDEX) {
      /* Field/deref assignment: treat as expression statement.
       * For now, wrap as a binary "=" expression to avoid losing
       * information; the codegen/semantic will handle it. */
      return nova_expr_binary(
          expr, "=", value,
          expr->span.start == 0
              ? (nova_location_t){0, 0}
              : nova_location((int)expr->span.line, (int)expr->span.col));
    }

    /* Unsupported assignment target */
    nova_expr_free(expr);
    nova_expr_free(value);
    return parse_error(parser, "Invalid assignment target");
  }

  return expr;
}

static nova_expr_t *parse_expression(nova_parser_t *parser) {
  return parse_assignment(parser);
}

// ══════════════════════════════════════════════════════════════════════════════
// TYPE PARSING
// ══════════════════════════════════════════════════════════════════════════════

static nova_type_t *parse_type(nova_parser_t *parser) {
  /* Handle qty<T, dim> - Unit Algebra */
  if (match(parser, TOKEN_KEYWORD_QTY)) {
    if (!match(parser, TOKEN_LT)) {
      return NULL; // Expected '<' after 'qty'
    }

    // Parse scalar type (e.g., f64)
    nova_type_t *scalar_type = parse_type(parser);
    if (!scalar_type) {
      return NULL;
    }

    if (!match(parser, TOKEN_COMMA)) {
      nova_type_free(scalar_type);
      return NULL; // Expected ',' after scalar type
    }

    // Parse dimension expression (kg, m/s, kg·m/s², etc.)
    // Collect all tokens until we hit '>'
    char dim_buffer[256];
    int dim_idx = 0;

    // Read first unit identifier
    if (!match(parser, TOKEN_IDENT)) {
      nova_type_free(scalar_type);
      return NULL; // Expected dimension name
    }

    // Copy first identifier
    size_t len = parser->previous.length;
    if (len >= 256)
      len = 255;
    memcpy(dim_buffer, parser->previous.start, len);
    dim_idx = (int)len;

    // Continue parsing operators and units until '>'
    while (!check(parser, TOKEN_GT) && !check(parser, TOKEN_EOF)) {
      if (match(parser, TOKEN_SLASH)) {
        // Division: m/s
        if (dim_idx < 255)
          dim_buffer[dim_idx++] = '/';

        if (!match(parser, TOKEN_IDENT)) {
          nova_type_free(scalar_type);
          return NULL; // Expected unit after '/'
        }

        len = parser->previous.length;
        if (dim_idx + len >= 256)
          len = 255 - dim_idx;
        memcpy(dim_buffer + dim_idx, parser->previous.start, len);
        dim_idx += len;
      } else if (match(parser, TOKEN_STAR)) {
        // Multiplication: kg*m
        if (dim_idx < 255)
          dim_buffer[dim_idx++] = '*';

        if (!match(parser, TOKEN_IDENT)) {
          nova_type_free(scalar_type);
          return NULL; // Expected unit after '*'
        }

        len = parser->previous.length;
        if (dim_idx + len >= 256)
          len = 255 - dim_idx;
        memcpy(dim_buffer + dim_idx, parser->previous.start, len);
        dim_idx += len;
      } else if (match(parser, TOKEN_LIT_INT)) {
        // Superscript number directly after unit: m2 → m^2
        if (dim_idx < 255)
          dim_buffer[dim_idx++] = '^';

        len = parser->previous.length;
        if (dim_idx + len >= 256)
          len = 255 - dim_idx;
        memcpy(dim_buffer + dim_idx, parser->previous.start, len);
        dim_idx += len;
      } else {
        break; // Unknown token, stop parsing
      }
    }

    dim_buffer[dim_idx] = '\0';
    char *dim_name = strdup(dim_buffer);

    if (!match(parser, TOKEN_GT)) {
      nova_type_free(scalar_type);
      free(dim_name);
      return NULL; // Expected '>' after dimension
    }

    // Create qty type
    nova_type_t *qty_type = nova_type_qty(scalar_type, dim_name);
    free(dim_name);

    return qty_type;
  }

  if (match(parser, TOKEN_STAR) || match(parser, TOKEN_STAR_STAR)) {
    /* Handle *mut T, *var T, *let T and *const T */
    while (match(parser, TOKEN_KEYWORD_MUT) ||
           match(parser, TOKEN_KEYWORD_VAR) ||
           match(parser, TOKEN_KEYWORD_LET) ||
           match(parser, TOKEN_KEYWORD_CONST)) {
    }
    if (check(parser, TOKEN_IDENT) && parser->current.length == 5 &&
        strncmp(parser->current.start, "const", 5) == 0) {
      advance(parser);
    }
    nova_type_t *pointee = parse_type(parser);
    if (!pointee)
      return NULL;
    return nova_type_pointer(pointee);
  }

  /* Handle reference types: &T, &mut T, &var T */
  if (match(parser, TOKEN_AMPERSAND)) {
    while (match(parser, TOKEN_KEYWORD_MUT) ||
           match(parser, TOKEN_KEYWORD_VAR) ||
           match(parser, TOKEN_KEYWORD_LET)) {
    }
    nova_type_t *referent = parse_type(parser);
    if (!referent)
      return NULL;
    return nova_type_pointer(referent); /* treat refs as pointers for now */
  }

  /* Handle tuple types: (T1, T2) */
  if (match(parser, TOKEN_LPAREN)) {
    /* skip until matching ) */
    int depth = 1;
    while (depth > 0 && !check(parser, TOKEN_EOF)) {
      if (match(parser, TOKEN_LPAREN))
        depth++;
      else if (match(parser, TOKEN_RPAREN))
        depth--;
      else
        advance(parser);
    }
    return nova_type_data("tuple");
  }

  /* Handle fn types: fn(T) -> R */
  if (match(parser, TOKEN_KEYWORD_FN)) {
    if (match(parser, TOKEN_LPAREN)) {
      int depth = 1;
      while (depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LPAREN))
          depth++;
        else if (match(parser, TOKEN_RPAREN))
          depth--;
        else
          advance(parser);
      }
    }
    /* Optional return type */
    if (match(parser, TOKEN_ARROW)) {
      nova_type_t *ret = parse_type(parser);
      nova_type_free(ret); /* discard for now */
    }
    return nova_type_data("fn");
  }

  if (match(parser, TOKEN_LBRACKET)) {
    nova_type_t *element_type = parse_type(parser);
    if (!element_type)
      return NULL;
    size_t size = 0;
    if (match(parser, TOKEN_SEMICOLON)) {
      /* Fixed-size array: [T; N] */
      if (!match(parser, TOKEN_LIT_INT)) {
        nova_type_free(element_type);
        return NULL;
      }
      size = strtoul(parser->previous.start, NULL, 10);
    }
    if (!match(parser, TOKEN_RBRACKET)) {
      nova_type_free(element_type);
      return NULL;
    }
    return nova_type_array(element_type, size);
  }

  if (match(parser, TOKEN_IDENT)) {
    char *type_name = strndup(parser->previous.start, parser->previous.length);
    nova_type_t *type = NULL;

    if (strcmp(type_name, "void") == 0)
      type = nova_type_void();
    else if (strcmp(type_name, "bool") == 0)
      type = nova_type_bool();
    else if (strcmp(type_name, "i32") == 0)
      type = nova_type_i32();
    else if (strcmp(type_name, "i64") == 0)
      type = nova_type_i64();
    else if (strcmp(type_name, "f32") == 0)
      type = nova_type_f32();
    else if (strcmp(type_name, "f64") == 0)
      type = nova_type_f64();
    else if (strcmp(type_name, "usize") == 0)
      type = nova_type_usize();
    else if (strcmp(type_name, "u8") == 0)
      type = nova_type_u8();
    else if (strcmp(type_name, "str") == 0)
      type = nova_type_str();
    else if (strcmp(type_name, "String") == 0)
      type = nova_type_str();
    else
      type = nova_type_data(type_name);

    free(type_name);

    /* Nova v10: Skip generics like <T, E> for now so parser can continue.
     * The C backend doesn't handle templates yet, but we need to parse. */
    if (match(parser, TOKEN_LT)) {
      int depth = 1;
      while (depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LT))
          depth++;
        else if (match(parser, TOKEN_GT))
          depth--;
        else
          advance(parser);
      }
    }

    return type;
  }

  return NULL;
}

// ══════════════════════════════════════════════════════════════════════════════
// STATEMENT PARSING
// ══════════════════════════════════════════════════════════════════════════════

/* Heap yönetim deyimleri — parse_unary'den buraya taşındı. */
static nova_expr_t *parse_heap_expression(nova_parser_t *parser) {
  /* TOKEN_KEYWORD_HEAP zaten tüketildi. Şimdi 'new' bekliyoruz. */
  if (!match(parser, TOKEN_KEYWORD_NEW))
    return parse_error(parser, "Expected 'new' after 'heap'");

  nova_type_t *type = parse_type(parser);
  if (!type)
    return parse_error(parser, "Expected type after 'heap new'");

  nova_expr_t **args = NULL;
  size_t arg_count = 0;

  if (match(parser, TOKEN_LPAREN)) {
    while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
      nova_expr_t *arg = parse_expression(parser);
      if (!arg) {
        nova_type_free(type);
        for (size_t i = 0; i < arg_count; i++)
          nova_expr_free(args[i]);
        if (args)
          free(args);
        return NULL;
      }
      args = realloc(args, sizeof(nova_expr_t *) * (arg_count + 1));
      args[arg_count++] = arg;
      if (!match(parser, TOKEN_COMMA))
        break;
    }
    if (!match(parser, TOKEN_RPAREN)) {
      nova_type_free(type);
      for (size_t i = 0; i < arg_count; i++)
        nova_expr_free(args[i]);
      free(args);
      return parse_error(parser, "Expected ')' after constructor arguments");
    }
  }

  return nova_expr_heap_new(type, args, arg_count,
                            token_location(parser->previous));
}

static nova_stmt_t *parse_heap_statement(nova_parser_t *parser) {
  /* TOKEN_KEYWORD_HEAP zaten tüketildi. Sadece 'free' burada işlenir. */
  if (match(parser, TOKEN_KEYWORD_FREE)) {
    nova_expr_t *ptr = parse_expression(parser);
    if (!ptr)
      return NULL;
    if (!match(parser, TOKEN_SEMICOLON)) {
      nova_expr_free(ptr);
      return stmt_error(parser, "Expected ';' after heap free");
    }
    return nova_stmt_heap_free(ptr, token_location(parser->previous));
  }

  return stmt_error(parser,
                    "Expected 'free' after 'heap' in statement context");
}

static nova_stmt_t *parse_statement(nova_parser_t *parser) {
  if (match(parser, TOKEN_AT)) {
    /* skip attributes like @extern(...) */
    if (!match(parser, TOKEN_IDENT)) {
      parse_error(parser, "Expected attribute name after '@'");
      return NULL;
    }
    if (match(parser, TOKEN_LPAREN)) {
      while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
        advance(parser);
      }
      match(parser, TOKEN_RPAREN);
    }
    return parse_statement(parser);
  }

  if (match(parser, TOKEN_LBRACE))
    return parse_block(parser);
  if (match(parser, TOKEN_KEYWORD_VAR) || match(parser, TOKEN_KEYWORD_LET) ||
      match(parser, TOKEN_KEYWORD_CONST))
    return parse_variable_declaration(parser);
  if (match(parser, TOKEN_KEYWORD_FN))
    return parse_function_declaration(parser);
  if (match(parser, TOKEN_KEYWORD_CHECK) || match(parser, TOKEN_KEYWORD_IF))
    return parse_check_statement(parser);

  /* Skip modifiers for Stage 0 compatibility */
  if (match(parser, TOKEN_KEYWORD_PUB) || match(parser, TOKEN_KEYWORD_OPEN) ||
      match(parser, TOKEN_KEYWORD_EXPOSE) ||
      match(parser, TOKEN_KEYWORD_ASYNC)) {
    return parse_statement(parser);
  }

  /* skip 'verified' for now */
  if (check(parser, TOKEN_IDENT) && parser->current.length == 8 &&
      strncmp(parser->current.start, "verified", 8) == 0) {
    advance(parser);
    if (match(parser, TOKEN_LBRACE)) {
      int depth = 1;
      while (depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LBRACE))
          depth++;
        else if (match(parser, TOKEN_RBRACE))
          depth--;
        else
          advance(parser);
      }
    }
    return nova_stmt_block(NULL, 0, token_location(parser->previous));
  }
  if (match(parser, TOKEN_KEYWORD_WHILE))
    return parse_while_statement(parser);
  if (match(parser, TOKEN_KEYWORD_FOR))
    return parse_for_statement(parser);
  if (match(parser, TOKEN_KEYWORD_LOOP))
    return parse_loop_statement(parser);
  if (match(parser, TOKEN_KEYWORD_SHAPE) || match(parser, TOKEN_KEYWORD_DATA) ||
      match(parser, TOKEN_KEYWORD_STRUCT))
    return parse_struct_declaration(parser);

  if (match(parser, TOKEN_KEYWORD_KIND) || match(parser, TOKEN_KEYWORD_ENUM) ||
      match(parser, TOKEN_KEYWORD_CASES))
    return parse_enum_declaration(parser);
  if (match(parser, TOKEN_KEYWORD_MOD) || match(parser, TOKEN_KEYWORD_SPACE))
    return parse_module_declaration(parser);

  /* GPU and UI stubs for Stage 0 */
  if (match(parser, TOKEN_KEYWORD_SYNC)) {
    match(parser, TOKEN_SEMICOLON);
    return nova_stmt_block(NULL, 0, token_location(parser->previous));
  }

  if (match(parser, TOKEN_KEYWORD_PARALLEL) ||
      match(parser, TOKEN_KEYWORD_VIEW)) {
    if (match(parser, TOKEN_LPAREN)) {
      while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF))
        advance(parser);
      match(parser, TOKEN_RPAREN);
    }
    nova_stmt_t *body = parse_block(parser);
    return body;
  }

  if (match(parser, TOKEN_KEYWORD_COMPONENT)) {
    /* Component acts like a struct in Stage 0 */
    return parse_struct_declaration(parser);
  }

  /* try { ... } catch e { ... } — error handling block */
  if (match(parser, TOKEN_KEYWORD_TRY)) {
    nova_location_t try_loc = token_location(parser->previous);
    if (!match(parser, TOKEN_LBRACE))
      return stmt_error(parser, "Expected '{' after 'try'");

    nova_stmt_t *try_body = parse_block(parser);
    if (!try_body)
      return NULL;

    char *ex_name = NULL;
    nova_stmt_t *catch_body = NULL;

    if (match(parser, TOKEN_KEYWORD_CATCH)) {
      if (check(parser, TOKEN_IDENT)) {
        advance(parser);
        ex_name = strndup(parser->previous.start, parser->previous.length);
      }

      if (!match(parser, TOKEN_LBRACE)) {
        nova_stmt_free(try_body);
        if (ex_name)
          free(ex_name);
        return stmt_error(parser, "Expected '{' after 'catch'");
      }

      catch_body = parse_block(parser);
      if (!catch_body) {
        nova_stmt_free(try_body);
        if (ex_name)
          free(ex_name);
        return NULL;
      }
    }

    nova_stmt_t *stmt =
        nova_stmt_try_catch(try_body, catch_body, ex_name, try_loc);
    if (ex_name)
      free(ex_name);
    return stmt;
  }

  /* unit Mass = kg | g | lb | oz;
   * Unit family declaration — parse and emit as a unit-type statement.
   * For now: consume the entire declaration, treat as a no-op stmt. */
  /* dims Force = Mass · Length · Time^(-2);
   * Dimensional analysis declaration — consume and treat as no-op. */
  if (match(parser, TOKEN_KEYWORD_DIMS)) {
    if (!match(parser, TOKEN_IDENT)) {
      parse_error(parser, "Expected dimension name after 'dims'");
      return NULL;
    }
    const char *dim_name =
        strndup(parser->previous.start, parser->previous.length);
    if (!match(parser, TOKEN_EQ)) {
      parse_error(parser, "Expected '=' after dimension name");
      free((void *)dim_name);
      return NULL;
    }
    /* Consume everything until ';' — handles ·, ^, (, ), idents, numbers */
    int depth = 0;
    while (!check(parser, TOKEN_EOF)) {
      if (check(parser, TOKEN_LPAREN)) {
        depth++;
        advance(parser);
      } else if (check(parser, TOKEN_RPAREN)) {
        depth--;
        advance(parser);
      } else if (check(parser, TOKEN_SEMICOLON) && depth == 0) {
        break;
      } else if ((check(parser, TOKEN_RBRACE) || check(parser, TOKEN_EOF)) &&
                 depth == 0) {
        break;
      } else {
        advance(parser);
      }
    }
    match(parser, TOKEN_SEMICOLON);
    nova_location_t loc = {0};
    nova_expr_t *dummy = nova_expr_lit_str(dim_name, loc);
    free((void *)dim_name);
    return nova_stmt_expr(dummy, loc);
  }
  if (match(parser, TOKEN_KEYWORD_UNIT)) {
    /* Expect unit name identifier */
    if (!match(parser, TOKEN_IDENT)) {
      parse_error(parser, "Expected unit family name after 'unit'");
      return NULL;
    }
    const char *unit_name =
        strndup(parser->previous.start, parser->previous.length);
    /* Expect '=' */
    if (!match(parser, TOKEN_EQ)) {
      parse_error(parser, "Expected '=' after unit family name");
      free((void *)unit_name);
      return NULL;
    }
    /* Consume all unit identifiers separated by '|' until ';' or '}' or EOF */
    while (!check(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_RBRACE) &&
           !check(parser, TOKEN_EOF)) {
      if (check(parser, TOKEN_IDENT)) {
        advance(parser); /* unit name */
      } else if (check(parser, TOKEN_PIPE)) {
        advance(parser); /* '|' separator */
      } else {
        /* Unicode unit chars like °C — advance past them */
        size_t remaining = parser->lexer ? 0 : 0;
        (void)remaining;
        advance(parser);
      }
    }
    match(parser, TOKEN_SEMICOLON);
    /* Emit as an expression statement wrapping a string literal (no-op) */
    nova_location_t loc = {0};
    nova_expr_t *dummy = nova_expr_lit_str(unit_name, loc);
    free((void *)unit_name);
    nova_stmt_t *s = nova_stmt_expr(dummy, loc);
    return s;
  }
  if (match(parser, TOKEN_KEYWORD_IMPORT) || match(parser, TOKEN_KEYWORD_USE))
    return parse_import_declaration(parser);
  if (match(parser, TOKEN_KEYWORD_PUB) || match(parser, TOKEN_KEYWORD_EXPOSE) ||
      match(parser, TOKEN_KEYWORD_OPEN) || match(parser, TOKEN_KEYWORD_BRING)) {
    /* Visibility modifier: pub fn / pub let / pub shape
     * Parse the inner declaration and mark it as public. */
    nova_stmt_t *inner = parse_statement(parser);
    if (!inner)
      return NULL;
    /* Set is_public flag on supported declaration kinds */
    if (inner->kind == STMT_FN) {
      inner->data.fn_stmt.is_public = true;
    } else if (inner->kind == STMT_VAR_DECL) {
      inner->data.var_decl.is_public = true;
    }
    return inner;
  }
  /* Nova v10: 'type' keyword for type aliases - skip for now */
  if (match(parser, TOKEN_KEYWORD_TYPE) || match(parser, TOKEN_KEYWORD_ALIAS)) {
    /* type/alias Alias = SomeType; -- skip until semicolon */
    while (!check(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_EOF)) {
      advance(parser);
    }
    match(parser, TOKEN_SEMICOLON);
    return nova_stmt_block(NULL, 0, token_location(parser->previous));
  }
  /* Nova v10: 'mut' keyword - should only appear inside var/let declarations,
   * not at statement level */
  if (check(parser, TOKEN_KEYWORD_MUT)) {
    /* This is an error - mut must come after var/let, not before */
    return stmt_error(parser, "'mut' keyword must appear after 'var' or 'let'");
  }
  /* Nova v10: #[...] attributes - skip the attribute and parse the next decl */
  if (match(parser, TOKEN_HASH)) {
    if (match(parser, TOKEN_LBRACKET)) {
      int depth = 1;
      while (depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LBRACKET))
          depth++;
        else if (match(parser, TOKEN_RBRACKET))
          depth--;
        else
          advance(parser);
      }
    }
    return parse_statement(parser);
  }
  if (match(parser, TOKEN_KEYWORD_ENUM) || match(parser, TOKEN_KEYWORD_CASES) ||
      match(parser, TOKEN_KEYWORD_KIND)) {
    return parse_enum_declaration(parser);
  }

  if (match(parser, TOKEN_KEYWORD_APPLY) || match(parser, TOKEN_KEYWORD_IMPL)) {
    char *type_name = NULL;
    if (match(parser, TOKEN_IDENT)) {
      type_name = strndup(parser->previous.start, parser->previous.length);
    }
    while (!check(parser, TOKEN_LBRACE) && !check(parser, TOKEN_EOF)) {
      advance(parser);
    }
    if (match(parser, TOKEN_LBRACE)) {
      char *old_prefix = parser->current_type_prefix;
      parser->current_type_prefix = type_name;
      nova_stmt_t **inner_stmts = NULL;
      size_t inner_count = 0;
      size_t inner_cap = 0;
      while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        nova_stmt_t *s = parse_statement(parser);
        if (s) {
          if (inner_count >= inner_cap) {
            inner_cap = inner_cap == 0 ? 8 : inner_cap * 2;
            inner_stmts =
                realloc(inner_stmts, sizeof(nova_stmt_t *) * inner_cap);
          }
          inner_stmts[inner_count++] = s;
        } else {
          /* Error recovery: skip the problematic token to avoid infinite loop
           */
          if (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            advance(parser);
          }
        }
      }
      match(parser, TOKEN_RBRACE);
      parser->current_type_prefix = old_prefix;
      if (type_name)
        free(type_name);
      return nova_stmt_block(inner_stmts, inner_count,
                             token_location(parser->previous));
    }
    if (type_name)
      free(type_name);
    return nova_stmt_block(NULL, 0, token_location(parser->previous));
  }

  if (match(parser, TOKEN_KEYWORD_SKILL) ||
      match(parser, TOKEN_KEYWORD_RULES) ||
      match(parser, TOKEN_KEYWORD_TRAIT)) {
    /* skill/trait — skip everything until { then balance braces */
    while (!check(parser, TOKEN_LBRACE) && !check(parser, TOKEN_EOF)) {
      advance(parser);
    }
    if (check(parser, TOKEN_LBRACE)) {
      skip_block(parser);
    }
    /* Return an empty block or null to signify it's handled/skipped */
    return nova_stmt_block(NULL, 0, token_location(parser->previous));
  }
  /* Nova v10: 'for' loops - skip for now */
  if (match(parser, TOKEN_KEYWORD_FOR)) {
    while (!check(parser, TOKEN_LBRACE) && !check(parser, TOKEN_EOF)) {
      advance(parser);
    }
    if (match(parser, TOKEN_LBRACE)) {
      int depth = 1;
      while (depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LBRACE))
          depth++;
        else if (match(parser, TOKEN_RBRACE))
          depth--;
        else
          advance(parser);
      }
    }
    return nova_stmt_block(NULL, 0, token_location(parser->previous));
  }
  /* Nova v10: 'macro' declarations — skip for now */
  if (check(parser, TOKEN_IDENT) && parser->current.length == 5 &&
      strncmp(parser->current.start, "macro", 5) == 0) {
    advance(parser); /* consume 'macro' */
    /* Skip everything until matching } or ; */
    if (check(parser, TOKEN_LBRACE)) {
      skip_block(parser);
    } else {
      while (!check(parser, TOKEN_LBRACE) && !check(parser, TOKEN_SEMICOLON) &&
             !check(parser, TOKEN_EOF)) {
        advance(parser);
      }
      if (check(parser, TOKEN_LBRACE)) {
        skip_block(parser);
      } else {
        match(parser, TOKEN_SEMICOLON);
      }
    }
    return nova_stmt_block(NULL, 0, token_location(parser->previous));
  }
  /* Nova v10: 'match' expression/statement */
  if (match(parser, TOKEN_KEYWORD_MATCH)) {
    nova_expr_t *expr = parse_match_expression(parser);
    if (!expr)
      return NULL;
    return nova_stmt_expr(expr, token_location(parser->previous));
  }
  if (match(parser, TOKEN_KEYWORD_EACH)) {
    return parse_each_statement(parser);
  }
  /* Nova v10: 'foreign "C"' / 'extern' blocks */
  if (match(parser, TOKEN_KEYWORD_FOREIGN)) {
    return parse_foreign_block(parser);
  }
  if (match(parser, TOKEN_KEYWORD_SPAWN)) {
    return parse_spawn_statement(parser);
  }
  if (check(parser, TOKEN_KEYWORD_HEAP)) {
    nova_token_t next = nova_lexer_peek(parser->lexer);
    if (next.type == TOKEN_KEYWORD_FREE) {
      advance(parser);
      return parse_heap_statement(parser);
    }
    /* It's heap new ... handle as expression statement */
  }

  if (match(parser, TOKEN_KEYWORD_BREAK)) {
    if (!match(parser, TOKEN_SEMICOLON))
      return stmt_error(parser, "Expected ';' after 'break'");
    nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
    stmt->kind = STMT_BREAK;
    stmt->span = nova_span_from_location(token_location(parser->previous));
    return stmt;
  }

  if (match(parser, TOKEN_KEYWORD_CONTINUE)) {
    if (!match(parser, TOKEN_SEMICOLON))
      return stmt_error(parser, "Expected ';' after 'continue'");
    nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
    stmt->kind = STMT_CONTINUE;
    stmt->span = nova_span_from_location(token_location(parser->previous));
    return stmt;
  }

  if (match(parser, TOKEN_KEYWORD_YIELD))
    return parse_yield_statement(parser);
  if (match(parser, TOKEN_KEYWORD_RETURN))
    return parse_return_statement(parser);

  nova_expr_t *expr = parse_expression(parser);
  if (!expr)
    return NULL;
  match(parser, TOKEN_SEMICOLON);
  return nova_stmt_expr(expr, token_location(parser->previous));
}

/* var x = 42;  /  var x: *Point = &p;  /  let mut x = 42;
 * Düzeltme: free(name) aktif edildi. Supports 'mut' keyword. */
static nova_stmt_t *parse_variable_declaration(nova_parser_t *parser) {
  /* Handle optional 'mut' keyword before variable name */
  bool is_mutable = false;
  if (match(parser, TOKEN_KEYWORD_MUT)) {
    is_mutable = true;
  }

  /* Tuple destructuring: let (x, y) = expr;
   * Consume the entire (a, b, ...) pattern and use a synthetic name. */
  if (check(parser, TOKEN_LPAREN)) {
    advance(parser); /* consume '(' */
    /* Collect binding names */
    while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
      if (check(parser, TOKEN_IDENT) || check(parser, TOKEN_KEYWORD_MUT)) {
        match(parser, TOKEN_KEYWORD_MUT);
        advance(parser); /* consume ident */
      } else {
        advance(parser);
      }
      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RPAREN))
        break;
    }
    match(parser, TOKEN_RPAREN);
    /* Use synthetic name for the declaration */
    nova_type_t *var_type2 = NULL;
    if (match(parser, TOKEN_COLON)) {
      var_type2 = parse_type(parser);
    }
    nova_expr_t *init2 = NULL;
    if (match(parser, TOKEN_EQ)) {
      init2 = parse_expression(parser);
    }
    match(parser, TOKEN_SEMICOLON);
    nova_stmt_t *s = nova_stmt_var("__tuple_destruct__", var_type2, init2,
                                   token_location(parser->previous));
    return s;
  }

  if (!match_ident_or_contextual_keyword(parser))
    return stmt_error(parser, "Expected variable name");

  char *name = strndup(parser->previous.start, parser->previous.length);
  if (!name)
    return stmt_error(parser, "Out of memory");

  nova_type_t *var_type = NULL;
  if (match(parser, TOKEN_COLON)) {
    var_type = parse_type(parser);
    if (!var_type) {
      free(name);
      return stmt_error(parser, "Expected type after ':'");
    }
  }

  nova_expr_t *initializer = NULL;
  if (match(parser, TOKEN_EQ)) {
    /* Special case: match expression as initializer — parse directly */
    if (match(parser, TOKEN_KEYWORD_MATCH)) {
      initializer = parse_match_expression(parser);
    } else {
      initializer = parse_expression(parser);
    }
    if (!initializer) {
      free(name);
      if (var_type)
        nova_type_free(var_type);
      return NULL;
    }
  }

  match(parser, TOKEN_SEMICOLON);

  nova_stmt_t *stmt = nova_stmt_var(name, var_type, initializer,
                                    token_location(parser->previous));
  free(name); /* nova_stmt_var kendi kopyasını aldı */
  return stmt;
}

/* for binding in start..end { body } */
static nova_stmt_t *parse_for_statement(nova_parser_t *parser) {
  if (!match(parser, TOKEN_IDENT))
    return stmt_error(parser, "Expected binding name after 'for'");

  char *binding = strndup(parser->previous.start, parser->previous.length);

  if (!match(parser, TOKEN_KEYWORD_IN)) {
    free(binding);
    return stmt_error(parser, "Expected 'in' after for binding");
  }

  nova_expr_t *start = parse_expression(parser);
  if (!start) {
    free(binding);
    return NULL;
  }

  if (!match(parser, TOKEN_DOT_DOT)) {
    free(binding);
    nova_expr_free(start);
    return stmt_error(parser, "Expected '..' after range start in for-loop");
  }

  nova_expr_t *end = parse_expression(parser);
  if (!end) {
    free(binding);
    nova_expr_free(start);
    return NULL;
  }

  if (!match(parser, TOKEN_LBRACE)) {
    free(binding);
    nova_expr_free(start);
    nova_expr_free(end);
    return stmt_error(parser, "Expected '{' before for body");
  }

  nova_stmt_t *body = parse_block(parser);
  if (!body) {
    free(binding);
    nova_expr_free(start);
    nova_expr_free(end);
    return NULL;
  }

  nova_stmt_t *res = nova_stmt_for(binding, start, end, body,
                                   token_location(parser->previous));
  free(binding);
  return res;
}

/* check/if condition { then } else { else } */
static nova_stmt_t *parse_check_statement(nova_parser_t *parser) {
  int condition_line = parser->previous.line;
  nova_expr_t *condition = NULL;
  nova_pattern_t *pattern = NULL;

  /* Support 'check let Pattern = Expr' */
  parser->inhibit_struct_init = true;
  if (match(parser, TOKEN_KEYWORD_LET)) {
    pattern = parse_pattern(parser);
    if (!match(parser, TOKEN_EQ)) {
      parser->inhibit_struct_init = false;
      if (pattern)
        nova_pattern_free(pattern);
      return stmt_error(parser, "Expected '=' after pattern in 'check let'");
    }
    condition = parse_expression(parser);
  } else {
    condition = parse_expression(parser);
  }
  parser->inhibit_struct_init = false;

  if (!condition) {
    if (pattern)
      nova_pattern_free(pattern);
    return NULL;
  }

  if (!match(parser, TOKEN_LBRACE)) {
    nova_expr_free(condition);
    if (pattern)
      nova_pattern_free(pattern);
    char err[128];
    snprintf(err, sizeof(err),
             "Expected '{' after condition for 'check' starting at line %d",
             condition_line);
    return stmt_error(parser, err);
  }
  nova_stmt_t *then_branch = parse_block(parser);
  if (!then_branch) {
    nova_expr_free(condition);
    if (pattern)
      nova_pattern_free(pattern);
    return NULL;
  }

  nova_stmt_t *else_branch = NULL;
  if (match(parser, TOKEN_KEYWORD_ELSE)) {
    if (check(parser, TOKEN_KEYWORD_IF) || check(parser, TOKEN_KEYWORD_CHECK)) {
      advance(parser);
      else_branch = parse_check_statement(parser);
    } else if (match(parser, TOKEN_LBRACE)) {
      else_branch = parse_block(parser);
    } else {
      nova_expr_free(condition);
      if (pattern)
        nova_pattern_free(pattern);
      nova_stmt_free(then_branch);
      return stmt_error(parser, "Expected '{', 'if' or 'check' after 'else'");
    }

    if (!else_branch) {
      nova_expr_free(condition);
      if (pattern)
        nova_pattern_free(pattern);
      nova_stmt_free(then_branch);
      return NULL;
    }
  }

  nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
  stmt->kind = STMT_CHECK;
  stmt->span = nova_span_from_location(token_location(parser->previous));
  stmt->data.check_stmt.condition = condition;
  stmt->data.check_stmt.pattern = pattern;
  stmt->data.check_stmt.then_branch = then_branch;
  stmt->data.check_stmt.else_branch = else_branch;
  return stmt;
}

/* return/yield value; */
static nova_stmt_t *parse_yield_statement(nova_parser_t *parser) {
  nova_expr_t *value = NULL;
  if (!check(parser, TOKEN_SEMICOLON)) {
    value = parse_expression(parser);
    if (!value)
      return NULL;
  }
  if (!match(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_RBRACE)) {
    if (value)
      nova_expr_free(value);
    return stmt_error(parser, "Expected ';' after yield/return");
  }
  nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
  stmt->kind = STMT_YIELD;
  stmt->span = nova_span_from_location(token_location(parser->previous));
  stmt->data.yield_stmt = value;
  return stmt;
}

/**
 * parse_return_statement: Parse a C-style `return expr;` statement.
 * Semantically identical to `yield` — produces STMT_RETURN which the
 * semantic and codegen layers handle identically to STMT_YIELD.
 */
static nova_stmt_t *parse_return_statement(nova_parser_t *parser) {
  nova_expr_t *value = NULL;
  if (!check(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_RBRACE)) {
    value = parse_expression(parser);
    if (!value)
      return NULL;
  }
  if (!match(parser, TOKEN_SEMICOLON)) {
    /* Allow `return` without `;` before `}` for concise syntax */
    if (!check(parser, TOKEN_RBRACE)) {
      if (value)
        nova_expr_free(value);
      return stmt_error(parser, "Expected ';' after return expression");
    }
  }
  nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
  if (!stmt) {
    if (value)
      nova_expr_free(value);
    return NULL;
  }
  stmt->kind = STMT_RETURN;
  stmt->span = nova_span_from_location(token_location(parser->previous));
  stmt->data.return_expr = value;
  return stmt;
}

/* while condition { body } */
static nova_stmt_t *parse_each_statement(nova_parser_t *parser) {
  nova_location_t loc = token_location(parser->previous);
  if (!match(parser, TOKEN_IDENT)) {
    return stmt_error(parser, "Expect binding name after 'each'");
  }
  char *binding = strndup(parser->previous.start, parser->previous.length);
  if (!match(parser, TOKEN_KEYWORD_IN)) {
    free(binding);
    return stmt_error(parser, "Expect 'in' after binding in 'each'");
  }
  nova_expr_t *iterator = parse_expression(parser);
  if (!iterator) {
    free(binding);
    return NULL;
  }
  nova_stmt_t *body = parse_statement(parser);
  if (!body) {
    free(binding);
    nova_expr_free(iterator);
    return NULL;
  }
  return nova_stmt_each(binding, iterator, body, loc);
}

static nova_stmt_t *parse_spawn_statement(nova_parser_t *parser) {
  nova_location_t loc = token_location(parser->previous);
  nova_expr_t *expr = parse_expression(parser);
  if (!expr)
    return NULL;
  match(parser, TOKEN_SEMICOLON);
  return nova_stmt_spawn(expr, loc);
}

static nova_stmt_t *parse_foreign_block(nova_parser_t *parser) {
  nova_location_t loc = token_location(parser->previous);
  char *abi = strdup("C");
  if (match(parser, TOKEN_LIT_STR)) {
    free(abi);
    abi = strndup(parser->previous.start + 1, parser->previous.length - 2);
  }
  nova_stmt_t *body = parse_statement(parser);
  if (!body) {
    free(abi);
    return NULL;
  }
  nova_stmt_t *stmt = nova_stmt_foreign(abi, body, loc);
  free(abi);
  return stmt;
}

static nova_stmt_t *parse_loop_statement(nova_parser_t *parser) {
  nova_location_t loc = token_location(parser->previous);
  if (!match(parser, TOKEN_LBRACE)) {
    return stmt_error(parser, "Expected '{' after 'loop'");
  }
  nova_stmt_t *body = parse_block(parser);
  if (!body)
    return NULL;

  /* loop { body } is equivalent to while true { body } */
  nova_expr_t *condition = nova_expr_lit_bool(true, loc);
  nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
  stmt->kind = STMT_WHILE;
  stmt->span = nova_span_from_location(loc);
  stmt->data.while_stmt.condition = condition;
  stmt->data.while_stmt.body = body;
  return stmt;
}

static nova_stmt_t *parse_while_statement(nova_parser_t *parser) {
  nova_expr_t *condition = parse_expression(parser);
  if (!condition)
    return NULL;

  if (!match(parser, TOKEN_LBRACE)) {
    nova_expr_free(condition);
    return stmt_error(parser, "Expected '{' after while condition");
  }
  nova_stmt_t *body = parse_block(parser);
  if (!body) {
    nova_expr_free(condition);
    return NULL;
  }

  nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
  stmt->kind = STMT_WHILE;
  stmt->span = nova_span_from_location(token_location(parser->previous));
  stmt->data.while_stmt.condition = condition;
  stmt->data.while_stmt.body = body;
  return stmt;
}

/* parse_block: '{' çağrıdan önce tüketilmiş olmalı */
static nova_stmt_t *parse_block(nova_parser_t *parser) {
  nova_stmt_t **statements = NULL;
  size_t count = 0;
  size_t capacity = 0;

  while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
    nova_stmt_t *stmt = parse_statement(parser);
    if (!stmt) {
      /* Recovery: advance and try next statement instead of aborting the whole
       * block */
      if (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        advance(parser);
        continue;
      }
      for (size_t i = 0; i < count; i++)
        nova_stmt_free(statements[i]);
      free(statements);
      return NULL;
    }
    if (count >= capacity) {
      capacity = capacity == 0 ? 4 : capacity * 2;
      statements = realloc(statements, sizeof(nova_stmt_t *) * capacity);
    }
    statements[count++] = stmt;
  }

  if (!match(parser, TOKEN_RBRACE)) {
    for (size_t i = 0; i < count; i++)
      nova_stmt_free(statements[i]);
    free(statements);
    return stmt_error(parser, "Expected '}' after block");
  }

  nova_stmt_t *block = calloc(1, sizeof(nova_stmt_t));
  block->kind = STMT_BLOCK;
  block->span = nova_span_from_location(token_location(parser->previous));
  block->data.block.statements = statements;
  block->data.block.count = count;
  return block;
}

/* shape StructName { field: Type, ... } */
static nova_stmt_t *parse_struct_declaration(nova_parser_t *parser) {
  if (!match(parser, TOKEN_IDENT))
    return stmt_error(parser, "Expected struct name after 'shape'");

  char *struct_name = strndup(parser->previous.start, parser->previous.length);
  nova_stmt_t *stmt =
      nova_stmt_struct_decl(struct_name, token_location(parser->previous));
  free(struct_name);

  /* Nova v10: Skip generic type parameters like <T> or <T, E> */
  if (match(parser, TOKEN_LT)) {
    int depth = 1;
    while (depth > 0 && !check(parser, TOKEN_EOF)) {
      if (match(parser, TOKEN_LT))
        depth++;
      else if (match(parser, TOKEN_GT))
        depth--;
      else
        advance(parser);
    }
  }

  /* Nova v10: parse optional 'derives [Trait1, Trait2]' */
  if (match(parser, TOKEN_KEYWORD_DERIVES)) {
    if (!match(parser, TOKEN_LBRACKET)) {
      nova_stmt_free(stmt);
      return stmt_error(parser, "Expected '[' after 'derives'");
    }
    while (!check(parser, TOKEN_RBRACKET) && !check(parser, TOKEN_EOF)) {
      if (!match(parser, TOKEN_IDENT)) {
        nova_stmt_free(stmt);
        return stmt_error(parser, "Expected trait name in derives list");
      }
      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACKET))
        break;
    }
    if (!match(parser, TOKEN_RBRACKET)) {
      nova_stmt_free(stmt);
      return stmt_error(parser, "Expected ']' after derives list");
    }
  }

  if (!match(parser, TOKEN_LBRACE)) {
    nova_stmt_free(stmt);
    return stmt_error(parser, "Expected '{' after struct name");
  }

  struct {
    char *name;
    nova_type_t *type;
  } *fields = NULL;
  size_t field_count = 0;

  while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
    // Nova v10: Skip visibility modifiers on fields and state keywords
    while ((check(parser, TOKEN_KEYWORD_PUB) &&
            peek_next(parser) != TOKEN_COLON) ||
           (check(parser, TOKEN_KEYWORD_EXPOSE) &&
            peek_next(parser) != TOKEN_COLON) ||
           (check(parser, TOKEN_KEYWORD_OPEN) &&
            peek_next(parser) != TOKEN_COLON) ||
           (check(parser, TOKEN_KEYWORD_VAR) &&
            peek_next(parser) != TOKEN_COLON) ||
           (check(parser, TOKEN_KEYWORD_LET) &&
            peek_next(parser) != TOKEN_COLON) ||
           (check(parser, TOKEN_KEYWORD_YIELD) &&
            peek_next(parser) != TOKEN_COLON)) {
      advance(parser);
    }

    // Nova v10: Skip view blocks, functions, and kernels inside
    // components/structs, BUT ONLY if they are not followed by a colon
    // (which would mean it's a field name).
    if ((check(parser, TOKEN_KEYWORD_VIEW) &&
         peek_next(parser) != TOKEN_COLON) ||
        (check(parser, TOKEN_KEYWORD_FN) && peek_next(parser) != TOKEN_COLON) ||
        (check(parser, TOKEN_KEYWORD_KERNEL) &&
         peek_next(parser) != TOKEN_COLON)) {
      advance(parser); // Consume the keyword
      if (match(parser, TOKEN_IDENT)) {
        /* skip name */
      }
      if (match(parser, TOKEN_LPAREN)) {
        int depth = 1;
        while (depth > 0 && !check(parser, TOKEN_EOF)) {
          if (match(parser, TOKEN_LPAREN))
            depth++;
          else if (match(parser, TOKEN_RPAREN))
            depth--;
          else
            advance(parser);
        }
      }
      if (match(parser, TOKEN_ARROW)) {
        /* skip type bits */
        while (!check(parser, TOKEN_LBRACE) &&
               !check(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_EOF))
          advance(parser);
      }
      if (match(parser, TOKEN_LBRACE)) {
        int depth = 1;
        while (depth > 0 && !check(parser, TOKEN_EOF)) {
          if (match(parser, TOKEN_LBRACE))
            depth++;
          else if (match(parser, TOKEN_RBRACE))
            depth--;
          else
            advance(parser);
        }
      } else {
        match(parser, TOKEN_SEMICOLON);
      }
      continue;
    }

    // Nova v10: Allow keywords to be used as field or variant names
    if (!match(parser, TOKEN_IDENT)) {
      if (parser->current.type >= TOKEN_KEYWORD_FN &&
          parser->current.type < TOKEN_PLUS) {
        advance(parser); // Accept keyword as identifier
      } else {
        for (size_t i = 0; i < field_count; i++) {
          free(fields[i].name);
          nova_type_free(fields[i].type);
        }
        free(fields);
        nova_stmt_free(stmt);
        return stmt_error(parser, "Expected field or variant name");
      }
    }
    char *field_name = strndup(parser->previous.start, parser->previous.length);

    // Nova v10: handle enum variants like Variant(Type) or just Variant
    if (match(parser, TOKEN_LPAREN)) {
      int depth = 1;
      while (depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LPAREN))
          depth++;
        else if (match(parser, TOKEN_RPAREN))
          depth--;
        else
          advance(parser);
      }
      free(field_name);
      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACE))
        break;
      continue;
    }

    // Nova v10: handle enum variants with named fields like Variant { field:
    // Type, ... }
    if (match(parser, TOKEN_LBRACE)) {
      int brace_depth = 1;
      while (brace_depth > 0 && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_LBRACE))
          brace_depth++;
        else if (match(parser, TOKEN_RBRACE))
          brace_depth--;
        else
          advance(parser);
      }
      free(field_name);
      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACE))
        break;
      continue;
    }

    if (match(parser, TOKEN_EQ)) {
      /* Nova v10: handle direct initialization: var x = ...; */
      int depth = 0;
      while (!check(parser, TOKEN_EOF)) {
        nova_token_type_t t = parser->current.type;
        if (t == TOKEN_LPAREN || t == TOKEN_LBRACE || t == TOKEN_LBRACKET ||
            t == TOKEN_LT) {
          depth++;
        } else if (t == TOKEN_RPAREN || t == TOKEN_RBRACE ||
                   t == TOKEN_RBRACKET || t == TOKEN_GT) {
          if (depth == 0) {
            if (t == TOKEN_RBRACE)
              break; // End of component
            break;
          }
          depth--;
        } else if (t == TOKEN_SEMICOLON || t == TOKEN_COMMA) {
          if (depth == 0)
            break;
        }
        advance(parser);
      }
      free(field_name);
      if (check(parser, TOKEN_SEMICOLON))
        advance(parser);
      if (check(parser, TOKEN_COMMA))
        advance(parser);
      continue;
    }

    if (!match(parser, TOKEN_COLON)) {
      /* Case of simple enum variant: Just the identifier */
      free(field_name);
      if (!match(parser, TOKEN_COMMA) && !check(parser, TOKEN_RBRACE))
        match(parser, TOKEN_SEMICOLON); // Nova: fields usually end with ;
      continue;
    }

    nova_type_t *field_type = parse_type(parser);
    if (!field_type) {
      free(field_name);
      for (size_t i = 0; i < field_count; i++) {
        free(fields[i].name);
        nova_type_free(fields[i].type);
      }
      free(fields);
      nova_stmt_free(stmt);
      return stmt_error(parser, "Expected field type");
    }

    fields = realloc(fields, sizeof(*fields) * (field_count + 1));
    fields[field_count].name = field_name;
    fields[field_count].type = field_type;
    field_count++;
    if (!match(parser, TOKEN_COMMA) && !match(parser, TOKEN_SEMICOLON) &&
        !check(parser, TOKEN_RBRACE))
      break;
  }

  if (!match(parser, TOKEN_RBRACE)) {
    for (size_t i = 0; i < field_count; i++) {
      free(fields[i].name);
      nova_type_free(fields[i].type);
    }
    free(fields);
    nova_stmt_free(stmt);
    return stmt_error(parser, "Expected '}' after struct fields");
  }

  stmt->data.struct_decl->fields = (void *)fields;
  stmt->data.struct_decl->field_count = (int)field_count;
  return stmt;
}

/* enum Result { Ok(i32), Err(String) } */
static nova_stmt_t *parse_enum_declaration(nova_parser_t *parser) {
  if (!match(parser, TOKEN_IDENT))
    return stmt_error(parser, "Expected enum name");

  char *enum_name = strndup(parser->previous.start, parser->previous.length);
  nova_stmt_t *stmt =
      nova_stmt_enum_decl(enum_name, token_location(parser->previous));
  free(enum_name);

  /* Optional generic type parameters: <T>, <T, E>, <T: Trait>, etc.
   * Consume and discard for now — full generics inference handled by semantic.
   */
  if (check(parser, TOKEN_LT)) {
    advance(parser); /* consume '<' */
    int depth = 1;
    while (!check(parser, TOKEN_EOF) && depth > 0) {
      if (check(parser, TOKEN_LT)) {
        depth++;
        advance(parser);
      } else if (check(parser, TOKEN_GT)) {
        depth--;
        advance(parser);
      } else {
        advance(parser);
      }
    }
  }

  /* Nova v10: parse optional 'derives [Trait1, Trait2]' */
  if (match(parser, TOKEN_KEYWORD_DERIVES)) {
    if (match(parser, TOKEN_LBRACKET)) {
      while (!check(parser, TOKEN_RBRACKET) && !check(parser, TOKEN_EOF)) {
        advance(parser); // Skip trait names
        match(parser, TOKEN_COMMA);
      }
      match(parser, TOKEN_RBRACKET);
    }
  }

  if (!match(parser, TOKEN_LBRACE))
    return stmt_error(parser, "Expected '{' after enum name");

  while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
    match(parser, TOKEN_KEYWORD_YIELD); // Optional 'yield' modifier

    if (!match(parser, TOKEN_IDENT)) {
      nova_stmt_free(stmt);
      return stmt_error(parser, "Expected variant name");
    }

    char *variant_name =
        strndup(parser->previous.start, parser->previous.length);
    nova_type_t **field_types = NULL;
    size_t field_count = 0;

    if (match(parser, TOKEN_LPAREN)) {
      /* Tuple-like variant: Some(T), RGB(i64, i64, i64) */
      if (!check(parser, TOKEN_RPAREN)) {
        do {
          nova_type_t *t = parse_type(parser);
          if (!t) {
            free(variant_name);
            nova_stmt_free(stmt);
            return NULL;
          }
          field_types =
              realloc(field_types, sizeof(nova_type_t *) * (field_count + 1));
          field_types[field_count++] = t;
        } while (match(parser, TOKEN_COMMA));
      }
      if (!match(parser, TOKEN_RPAREN)) {
        free(variant_name);
        nova_stmt_free(stmt);
        return stmt_error(parser, "Expected ')' after variant types");
      }
    } else if (check(parser, TOKEN_LBRACE)) {
      /* Struct-like variant: Circle { radius: f64 }, Rectangle { w: f64, h: f64
       * } Consume the entire { field: type, ... } block, collecting field
       * types. */
      advance(parser); /* consume '{' */
      while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        /* field name */
        if (!match(parser, TOKEN_IDENT)) {
          advance(parser);
          continue;
        }
        /* ':' */
        if (!match(parser, TOKEN_COLON)) {
          advance(parser);
          continue;
        }
        /* field type */
        nova_type_t *t = parse_type(parser);
        if (t) {
          field_types =
              realloc(field_types, sizeof(nova_type_t *) * (field_count + 1));
          field_types[field_count++] = t;
        }
        match(parser, TOKEN_COMMA);
      }
      if (!match(parser, TOKEN_RBRACE)) {
        free(variant_name);
        nova_stmt_free(stmt);
        return stmt_error(parser, "Expected '}' after struct variant fields");
      }
    }

    stmt->data.enum_decl.variants =
        realloc(stmt->data.enum_decl.variants,
                sizeof(*(stmt->data.enum_decl.variants)) *
                    (stmt->data.enum_decl.variant_count + 1));
    stmt->data.enum_decl.variants[stmt->data.enum_decl.variant_count].name =
        variant_name;
    stmt->data.enum_decl.variants[stmt->data.enum_decl.variant_count]
        .field_types = field_types;
    stmt->data.enum_decl.variants[stmt->data.enum_decl.variant_count]
        .field_count = field_count;
    stmt->data.enum_decl.variant_count++;

    match(parser, TOKEN_COMMA);
  }

  if (!match(parser, TOKEN_RBRACE)) {
    nova_stmt_free(stmt);
    return stmt_error(parser, "Expected '}' after enum variants");
  }

  return stmt;
}

static nova_pattern_t *parse_pattern(nova_parser_t *parser) {
  /* Nova syntax: match arm'larinda optional `yield` modifier desteklenir.
   * Ornek: `yield Some('=') => { ... }` */
  match(parser, TOKEN_KEYWORD_YIELD);

  if (match(parser, TOKEN_IDENT)) {
    char *name = strndup(parser->previous.start, parser->previous.length);
    nova_location_t loc = token_location(parser->previous);

    if (strcmp(name, "_") == 0) {
      free(name);
      return nova_pattern_any(loc);
    }

    // Handle Namespaced::Variant
    if (match(parser, TOKEN_COLON_COLON)) {
      if (!match_ident_or_contextual_keyword(parser)) {
        /* [Fix 12] Allow keywords as variants in patterns */
        if (parser->current.type >= TOKEN_KEYWORD_FN &&
            parser->current.type <= TOKEN_KEYWORD_ON) {
          advance(parser);
        } else {
          free(name);
          return (nova_pattern_t *)parse_error(
              parser, "Expected variant name after '::'");
        }
      }
      free(name);
      name = strndup(parser->previous.start, parser->previous.length);
    }

    if (match(parser, TOKEN_LPAREN)) {
      nova_pattern_t **params = NULL;
      size_t count = 0;
      if (!check(parser, TOKEN_RPAREN)) {
        while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
          nova_pattern_t *p = parse_pattern(parser);
          if (!p) {
            free(name);
            for (size_t i = 0; i < count; i++) {
              nova_pattern_free(params[i]);
            }
            if (params)
              free(params);
            return NULL;
          }
          params = realloc(params, sizeof(nova_pattern_t *) * (count + 1));
          params[count++] = p;
          if (!match(parser, TOKEN_COMMA))
            break;
        }
      }
      if (!match(parser, TOKEN_RPAREN)) {
        free(name);
        for (size_t i = 0; i < count; i++) {
          nova_pattern_free(params[i]);
        }
        free(params);
        return (nova_pattern_t *)parse_error(
            parser, "Expected ')' after variant patterns");
      }
      return nova_pattern_variant(name, params, count, loc);
    }

    return nova_pattern_ident(name, loc);
  }

  if (match(parser, TOKEN_LIT_INT) || match(parser, TOKEN_LIT_FLOAT) ||
      match(parser, TOKEN_LIT_STR) || match(parser, TOKEN_LIT_TRUE) ||
      match(parser, TOKEN_LIT_FALSE) || match(parser, TOKEN_LIT_CHAR)) {
    nova_location_t loc = token_location(parser->previous);
    nova_expr_t *lit;
    if (parser->previous.type == TOKEN_LIT_INT)
      lit = nova_expr_lit_int(atoll(parser->previous.start), loc);
    else if (parser->previous.type == TOKEN_LIT_FLOAT)
      lit = nova_expr_lit_float(atof(parser->previous.start), loc);
    else if (parser->previous.type == TOKEN_LIT_STR)
      lit = nova_expr_lit_str(
          strndup(parser->previous.start + 1, parser->previous.length - 2),
          loc);
    else if (parser->previous.type == TOKEN_LIT_CHAR) {
      /* Char literal: 'x' -> extract the character value */
      const char *s = parser->previous.start + 1;
      char ch = (s[0] == '\\') ? s[1] : s[0];
      lit = nova_expr_lit_int((long long)ch, loc);
    } else
      lit = nova_expr_lit_bool(parser->previous.type == TOKEN_LIT_TRUE, loc);

    return nova_pattern_literal(lit, loc);
  }

  return (nova_pattern_t *)parse_error(parser, "Expected pattern");
}

static nova_expr_t *parse_match_expression(nova_parser_t *parser) {
  nova_location_t loc = token_location(parser->previous);

  parser->inhibit_struct_init = true;
  nova_expr_t *target = parse_expression(parser);
  parser->inhibit_struct_init = false;

  if (!target) {
    return parse_error(parser, "Expected expression for match target");
  }

  if (!match(parser, TOKEN_LBRACE)) {
    nova_expr_free(target);
    return parse_error(parser, "Expected '{' after match target");
  }

  nova_match_arm_t **arms = NULL;
  size_t arm_count = 0;

  while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
    nova_pattern_t *pattern = parse_pattern(parser);
    if (!pattern) {
      nova_expr_free(target);
      for (size_t i = 0; i < arm_count; i++)
        nova_match_arm_free(arms[i]);
      if (arms)
        free(arms);
      return NULL;
    }

    if (!match(parser, TOKEN_FAT_ARROW)) {
      nova_pattern_free(pattern);
      nova_expr_free(target);
      for (size_t i = 0; i < arm_count; i++)
        nova_match_arm_free(arms[i]);
      if (arms)
        free(arms);
      return parse_error(parser, "Expected '=>' after pattern");
    }

    nova_expr_t *body = NULL;
    if (check(parser, TOKEN_LBRACE)) {
      nova_location_t b_loc = token_location(parser->current);
      nova_stmt_t *blk = parse_block(parser);
      if (!blk) {
        nova_pattern_free(pattern);
        nova_expr_free(target);
        for (size_t i = 0; i < arm_count; i++)
          nova_match_arm_free(arms[i]);
        if (arms)
          free(arms);
        return NULL;
      }
      body = nova_expr_block(blk, b_loc);
    } else {
      /* Nova v10: match arm bodies can have an optional 'yield' prefix */
      match(parser, TOKEN_KEYWORD_YIELD);
      body = parse_expression(parser);
      if (!body) {
        nova_pattern_free(pattern);
        nova_expr_free(target);
        for (size_t i = 0; i < arm_count; i++)
          nova_match_arm_free(arms[i]);
        if (arms)
          free(arms);
        return NULL;
      }
    }

    arms = realloc(arms, sizeof(nova_match_arm_t *) * (arm_count + 1));
    arms[arm_count++] = nova_match_arm(pattern, body);
    match(parser, TOKEN_COMMA);
  }

  if (!match(parser, TOKEN_RBRACE)) {
    nova_expr_free(target);
    for (size_t i = 0; i < arm_count; i++)
      nova_match_arm_free(arms[i]);
    if (arms)
      free(arms);
    return parse_error(parser, "Expected '}' after match arms");
  }

  return nova_expr_match(target, arms, arm_count, loc);
}

/* fn name(param: Type) -> RetType { body }
 * Düzeltme: parametre tipleri parse_type() ile; varsayılan dönüş void. */
static nova_stmt_t *parse_function_declaration(nova_parser_t *parser) {
  if (!match_ident_or_contextual_keyword(parser))
    return stmt_error(parser, "Expected function name");

  char *name = strndup(parser->previous.start, parser->previous.length);
  if (parser->current_type_prefix) {
    size_t len = strlen(parser->current_type_prefix) + strlen(name) + 2;
    char *mangled = malloc(len);
    snprintf(mangled, len, "%s_%s", parser->current_type_prefix, name);
    free(name);
    name = mangled;
  }

  /* Optional generic type parameters: fn foo<T>(...), fn bar<T: Trait, E>(...)
   */
  if (check(parser, TOKEN_LT)) {
    advance(parser); /* consume '<' */
    int depth = 1;
    while (!check(parser, TOKEN_EOF) && depth > 0) {
      if (check(parser, TOKEN_LT)) {
        depth++;
        advance(parser);
      } else if (check(parser, TOKEN_GT)) {
        depth--;
        advance(parser);
      } else {
        advance(parser);
      }
    }
  }

  if (!match(parser, TOKEN_LPAREN)) {
    free(name);
    return stmt_error(parser, "Expected '(' after function name");
  }

  nova_param_t **params = NULL;
  size_t param_count = 0;
  size_t param_capacity = 0;

  if (!check(parser, TOKEN_RPAREN)) {
    do {
      /* Handle Nova v10 special parameter forms: &self, &mut self, self, mut x
       */
      if (match(parser, TOKEN_AMPERSAND)) {
        /* &self or &mut self or &var self */
        while (match(parser, TOKEN_KEYWORD_MUT) ||
               match(parser, TOKEN_KEYWORD_VAR) ||
               match(parser, TOKEN_KEYWORD_LET)) {
        }
        if (!match(parser, TOKEN_KEYWORD_SELF)) {
          /* Not &self -- treat as reference param, try ident */
          if (!match(parser, TOKEN_IDENT)) {
            for (size_t i = 0; i < param_count; i++) {
              free(params[i]->name);
              if (params[i]->type)
                nova_type_free(params[i]->type);
              free(params[i]);
            }
            free(params);
            free(name);
            return stmt_error(parser, "Expected parameter name after '&'");
          }
        }
        nova_param_t *param = malloc(sizeof(nova_param_t));
        param->name = strndup(parser->previous.start, parser->previous.length);
        param->type = NULL;
        if (match(parser, TOKEN_COLON)) {
          param->type = parse_type(parser);
        }
        if (param_count >= param_capacity) {
          param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
          params = realloc(params, sizeof(nova_param_t *) * param_capacity);
        }
        params[param_count++] = param;
        continue;
      }
      if (match(parser, TOKEN_KEYWORD_SELF)) {
        /* bare self */
        nova_param_t *param = malloc(sizeof(nova_param_t));
        param->name = strdup("self");
        param->type = NULL;
        if (param_count >= param_capacity) {
          param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
          params = realloc(params, sizeof(nova_param_t *) * param_capacity);
        }
        params[param_count++] = param;
        continue;
      }
      /* Consume optional 'mut', 'var', or 'let' before parameter name */
      while (match(parser, TOKEN_KEYWORD_MUT) ||
             match(parser, TOKEN_KEYWORD_VAR) ||
             match(parser, TOKEN_KEYWORD_LET)) {
      }
      if (!match_ident_or_contextual_keyword(parser)) {
        for (size_t i = 0; i < param_count; i++) {
          free(params[i]->name);
          if (params[i]->type)
            nova_type_free(params[i]->type);
          free(params[i]);
        }
        free(params);
        free(name);
        return stmt_error(parser, "Expected parameter name");
      }

      nova_param_t *param = malloc(sizeof(nova_param_t));
      param->name = strndup(parser->previous.start, parser->previous.length);
      param->type = NULL;

      if (match(parser, TOKEN_COLON)) {
        param->type = parse_type(parser);
        if (!param->type) {
          free(param->name);
          free(param);
          for (size_t i = 0; i < param_count; i++) {
            free(params[i]->name);
            if (params[i]->type)
              nova_type_free(params[i]->type);
            free(params[i]);
          }
          free(params);
          free(name);
          return stmt_error(parser, "Expected type after ':' in parameter");
        }
      }

      if (param_count >= param_capacity) {
        param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
        params = realloc(params, sizeof(nova_param_t *) * param_capacity);
      }
      params[param_count++] = param;
    } while (match(parser, TOKEN_COMMA));
  }

  if (!match(parser, TOKEN_RPAREN)) {
    for (size_t i = 0; i < param_count; i++) {
      free(params[i]->name);
      if (params[i]->type)
        nova_type_free(params[i]->type);
      free(params[i]);
    }
    free(params);
    free(name);
    return stmt_error(parser, "Expected ')' after parameters");
  }

  nova_type_t *return_type = NULL;
  if (match(parser, TOKEN_ARROW)) {
    return_type = parse_type(parser);
    if (!return_type) {
      for (size_t i = 0; i < param_count; i++) {
        free(params[i]->name);
        if (params[i]->type)
          nova_type_free(params[i]->type);
        free(params[i]);
      }
      free(params);
      free(name);
      return stmt_error(parser, "Expected type after '->'");
    }
  }
  /* If no -> annotation, leave return_type = NULL for inference in semantic
   * analyzer */

  /* require/ensure contract clauses:
   * fn foo(x: i64) -> i64
   *     require x > 0, "msg"
   *     ensure result >= 0, "msg"
   * { body }
   */
  nova_contract_stmt_t *
    requires
  = NULL;
  size_t requires_count = 0;
  nova_contract_stmt_t *ensures = NULL;
  size_t ensures_count = 0;

  while (check(parser, TOKEN_KEYWORD_REQUIRE) ||
         check(parser, TOKEN_KEYWORD_ENSURE)) {
    bool is_require = match(parser, TOKEN_KEYWORD_REQUIRE);
    if (!is_require)
      match(parser, TOKEN_KEYWORD_ENSURE);

    nova_expr_t *cond = parse_comparison(parser);
    nova_expr_t *msg = NULL;
    if (match(parser, TOKEN_COMMA)) {
      msg = parse_expression(parser);
    }

    if (is_require) {
      requires = realloc(requires,
                         sizeof(nova_contract_stmt_t) * (requires_count + 1));
      requires[requires_count++] = (nova_contract_stmt_t){cond, msg};
    } else {
      ensures =
          realloc(ensures, sizeof(nova_contract_stmt_t) * (ensures_count + 1));
      ensures[ensures_count++] = (nova_contract_stmt_t){cond, msg};
    }
  }

  /* on silicon_vpu / kernel_vpu / web_vpu / mesh_vpu */
  int target_vpu = 0;
  if (match(parser, TOKEN_KEYWORD_ON)) {
    if (!match(parser, TOKEN_IDENT)) {
      for (size_t i = 0; i < param_count; i++) {
        free(params[i]->name);
        if (params[i]->type)
          nova_type_free(params[i]->type);
        free(params[i]);
      }
      free(params);
      free(name);
      nova_type_free(return_type);
      return stmt_error(parser, "Expected VPU model name after 'on'");
    }
    char *vpu_name = strndup(parser->previous.start, parser->previous.length);
    if (strcmp(vpu_name, "silicon_vpu") == 0)
      target_vpu = BACKEND_ARMY_SILICON;
    else if (strcmp(vpu_name, "kernel_vpu") == 0)
      target_vpu = BACKEND_ARMY_KERNEL;
    else if (strcmp(vpu_name, "web_vpu") == 0)
      target_vpu = BACKEND_ARMY_WEB;
    else if (strcmp(vpu_name, "mesh_vpu") == 0)
      target_vpu = BACKEND_ARMY_MESH;
    else {
      free(vpu_name);
      for (size_t i = 0; i < param_count; i++) {
        free(params[i]->name);
        if (params[i]->type)
          nova_type_free(params[i]->type);
        free(params[i]);
      }
      free(params);
      free(name);
      nova_type_free(return_type);
      return stmt_error(parser, "Unknown VPU model");
    }
    free(vpu_name);
  }

  nova_stmt_t *body = NULL;
  if (match(parser, TOKEN_LBRACE)) {
    body = parse_block(parser);
    if (!body) {
      free(name);
      nova_type_free(return_type);
      return NULL;
    }
  } else {
    /* Allow bodyless functions for externs - check if there's a semicolon */
    match(parser, TOKEN_SEMICOLON);
  }

  nova_stmt_t *fn_stmt = calloc(1, sizeof(nova_stmt_t));
  fn_stmt->kind = STMT_FN;
  fn_stmt->span = nova_span_from_location(token_location(parser->previous));
  fn_stmt->data.fn_stmt.name = name;
  fn_stmt->data.fn_stmt.params = params;
  fn_stmt->data.fn_stmt.param_count = param_count;
  fn_stmt->data.fn_stmt.return_type = return_type;
  fn_stmt->data.fn_stmt.body = body;
  fn_stmt->data.fn_stmt.target_vpu = target_vpu;
  fn_stmt->data.fn_stmt.
    requires
  =
    requires;
  fn_stmt->data.fn_stmt.requires_count = requires_count;
  fn_stmt->data.fn_stmt.ensures = ensures;
  fn_stmt->data.fn_stmt.ensures_count = ensures_count;
  return fn_stmt;
}

// ══════════════════════════════════════════════════════════════════════════════
// MODULE / IMPORT / PUB
// ══════════════════════════════════════════════════════════════════════════════

/* mod lexer; or mod lexer { ... } */
static nova_stmt_t *parse_module_declaration(nova_parser_t *parser) {
  if (!match(parser, TOKEN_IDENT))
    return stmt_error(parser, "Expected module name after 'mod'");

  char *module_name = strndup(parser->previous.start, parser->previous.length);

  /* Handle inline module body: mod foo { ... } */
  if (match(parser, TOKEN_LBRACE)) {
    /* Skip the module body for now */
    int depth = 1;
    while (depth > 0 && !check(parser, TOKEN_EOF)) {
      if (match(parser, TOKEN_LBRACE))
        depth++;
      else if (match(parser, TOKEN_RBRACE))
        depth--;
      else
        advance(parser);
    }
    /* No semicolon needed after inline module */
  } else {
    /* External module declaration: mod lexer; */
    if (!match(parser, TOKEN_SEMICOLON)) {
      /* Make semicolon optional if next token is a statement starter */
      if (!check(parser, TOKEN_KEYWORD_MOD) &&
          !check(parser, TOKEN_KEYWORD_IMPORT) &&
          !check(parser, TOKEN_KEYWORD_USE) &&
          !check(parser, TOKEN_KEYWORD_FN) &&
          !check(parser, TOKEN_KEYWORD_VAR) &&
          !check(parser, TOKEN_KEYWORD_LET) &&
          !check(parser, TOKEN_KEYWORD_SHAPE) && !check(parser, TOKEN_EOF) &&
          !check(parser, TOKEN_RBRACE)) {
        free(module_name);
        return stmt_error(parser, "Expected ';' after module declaration");
      }
    }
  }

  nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
  stmt->kind = STMT_MOD;
  stmt->span = nova_span_from_location(token_location(parser->previous));
  stmt->data.mod_stmt.name = module_name;
  return stmt;
}

/* import foo::{bar, baz};  /  import foo::*;  /  import foo as f; */
static nova_stmt_t *parse_import_declaration(nova_parser_t *parser) {
  if (!match(parser, TOKEN_IDENT) && !match(parser, TOKEN_KEYWORD_CRATE) &&
      !match(parser, TOKEN_KEYWORD_SELF) && !match(parser, TOKEN_KEYWORD_SUPER))
    return stmt_error(parser, "Expected module name after 'import' or 'use'");

  char *module_name = strndup(parser->previous.start, parser->previous.length);
  bool had_trailing_colon = false;

  while (match(parser, TOKEN_COLON_COLON)) {
    if (match(parser, TOKEN_IDENT) || match(parser, TOKEN_KEYWORD_CRATE) ||
        match(parser, TOKEN_KEYWORD_SELF) ||
        match(parser, TOKEN_KEYWORD_SUPER)) {
      size_t new_len = strlen(module_name) + 2 + parser->previous.length + 1;
      char *new_name = malloc(new_len);
      if (new_name) {
        snprintf(new_name, new_len, "%s::%.*s", module_name,
                 (int)parser->previous.length, parser->previous.start);
        free(module_name);
        module_name = new_name;
      }
    } else {
      had_trailing_colon = true;
      break;
    }
  }

  char **imports = NULL;
  size_t import_count = 0;
  char *alias = NULL;

  if (had_trailing_colon) {
    if (match(parser, TOKEN_LBRACE)) {
      while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        if (!match(parser, TOKEN_IDENT)) {
          free(module_name);
          if (alias)
            free(alias);
          for (size_t i = 0; i < import_count; i++)
            free(imports[i]);
          free(imports);
          return stmt_error(parser, "Expected import name");
        }
        imports = realloc(imports, sizeof(char *) * (import_count + 1));
        imports[import_count++] =
            strndup(parser->previous.start, parser->previous.length);
        match(parser, TOKEN_COMMA);
      }
      if (!match(parser, TOKEN_RBRACE)) {
        free(module_name);
        if (alias)
          free(alias);
        for (size_t i = 0; i < import_count; i++)
          free(imports[i]);
        free(imports);
        return stmt_error(parser, "Expected '}' after imports");
      }
    } else if (match(parser, TOKEN_STAR)) {
      imports = malloc(sizeof(char *));
      if (imports) {
        imports[0] = strdup("*");
        import_count = 1;
      }
    } else {
      free(module_name);
      return stmt_error(parser, "Expected '{' or '*' after '::'");
    }
  } else {
    if (match(parser, TOKEN_KEYWORD_AS)) {
      if (!match(parser, TOKEN_IDENT)) {
        free(module_name);
        return stmt_error(parser, "Expected alias name after 'as'");
      }
      alias = strndup(parser->previous.start, parser->previous.length);
    }
  }

  /* Make semicolon optional if next token is a statement starter */
  if (!match(parser, TOKEN_SEMICOLON)) {
    if (!check(parser, TOKEN_KEYWORD_MOD) &&
        !check(parser, TOKEN_KEYWORD_IMPORT) &&
        !check(parser, TOKEN_KEYWORD_USE) && !check(parser, TOKEN_KEYWORD_FN) &&
        !check(parser, TOKEN_KEYWORD_VAR) &&
        !check(parser, TOKEN_KEYWORD_LET) &&
        !check(parser, TOKEN_KEYWORD_SHAPE) && !check(parser, TOKEN_EOF) &&
        !check(parser, TOKEN_RBRACE)) {
      free(module_name);
      if (alias)
        free(alias);
      for (size_t i = 0; i < import_count; i++)
        free(imports[i]);
      free(imports);
      return stmt_error(parser, "Expected ';' after import statement");
    }
  }

  nova_stmt_t *stmt = calloc(1, sizeof(nova_stmt_t));
  if (!stmt) {
    free(module_name);
    if (alias)
      free(alias);
    return NULL;
  }
  stmt->kind = STMT_IMPORT;
  stmt->span = nova_span_from_location(token_location(parser->previous));
  stmt->data.import_stmt.module_name = module_name;
  stmt->data.import_stmt.imports = imports;
  stmt->data.import_stmt.import_count = import_count;
  stmt->data.import_stmt.alias = alias;
  return stmt;
}

/* pub fn / pub var / pub shape */
static nova_stmt_t *parse_public_declaration(nova_parser_t *parser) {
  nova_stmt_t *inner_stmt = NULL;

  if (match(parser, TOKEN_KEYWORD_FN))
    inner_stmt = parse_function_declaration(parser);
  else if (match(parser, TOKEN_KEYWORD_VAR))
    inner_stmt = parse_variable_declaration(parser);
  else if (match(parser, TOKEN_KEYWORD_SHAPE))
    inner_stmt = parse_struct_declaration(parser);
  else
    return stmt_error(parser, "Expected fn, var, or shape after 'pub'");

  if (!inner_stmt)
    return NULL;
  return inner_stmt;
}

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ══════════════════════════════════════════════════════════════════════════════

nova_parser_t *nova_parser_create(nova_lexer_t *lexer) {
  nova_parser_t *parser = malloc(sizeof(nova_parser_t));
  if (!parser)
    return NULL;
  parser->lexer = lexer;
  parser->error_message = NULL;
  parser->inhibit_struct_init = false;
  parser->current_type_prefix = NULL;
  advance(parser);
  return parser;
}

nova_expr_t *nova_parser_parse_expression(nova_parser_t *parser) {
  return parse_expression(parser);
}

nova_stmt_t *nova_parser_parse_statement(nova_parser_t *parser) {
  return parse_statement(parser);
}

nova_stmt_t **nova_parser_parse_statements(nova_parser_t *parser,
                                           size_t *count) {
  nova_stmt_t **statements = NULL;
  size_t capacity = 0;
  *count = 0;

  while (!check(parser, TOKEN_EOF)) {
    nova_stmt_t *stmt = parse_statement(parser);
    if (!stmt) {
      for (size_t i = 0; i < *count; i++)
        nova_stmt_free(statements[i]);
      free(statements);
      return NULL;
    }
    if (*count >= capacity) {
      capacity = capacity == 0 ? 8 : capacity * 2;
      statements = realloc(statements, sizeof(nova_stmt_t *) * capacity);
    }
    statements[(*count)++] = stmt;
  }
  return statements;
}

const char *nova_parser_get_error(nova_parser_t *parser) {
  return parser->error_message ? parser->error_message : "No error";
}

nova_token_t nova_parser_get_last_token(nova_parser_t *parser) {
  return parser->previous;
}

bool nova_parser_had_error(nova_parser_t *parser) {
  return parser->error_message != NULL;
}

void nova_parser_destroy(nova_parser_t *parser) {
  if (parser->error_message)
    free(parser->error_message);
  free(parser);
}