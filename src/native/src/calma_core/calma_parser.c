#include "calma_parser.h"

/**
 * CALMA NATIVE PARSER
 * Recursive descent parser implementation for Nova
 */

static Token current_tok;
static Lexer *global_lexer;

void advance(Lexer *l) { current_tok = next_token(l); }

Node *parse_expression(Lexer *l) {
  if (current_tok.type == TOKEN_INTEGER) {
    Node *n = malloc(sizeof(Node));
    n->type = NODE_LITERAL_EXPR;
    n->as.literal_val = strdup(current_tok.value);
    n->next = NULL;
    advance(l);
    return n;
  }
  if (current_tok.type == TOKEN_IDENTIFIER) {
    Node *n = malloc(sizeof(Node));
    n->type = NODE_IDENTIFIER_EXPR;
    n->as.id_name = strdup(current_tok.value);
    n->next = NULL;
    advance(l);
    return n;
  }
  return NULL;
}

Node *parse_statement(Lexer *l) {
  if (current_tok.type == TOKEN_LET) {
    advance(l); // skip 'let'
    char *name = strdup(current_tok.value);
    advance(l); // skip identifier

    Node *val = NULL;
    if (current_tok.type == TOKEN_ASSIGN) {
      advance(l);
      val = parse_expression(l);
    }

    Node *n = malloc(sizeof(Node));
    n->type = NODE_VAR_DECL;
    n->as.var_decl.name = name;
    n->as.var_decl.value = val;
    n->next = NULL;
    return n;
  }

  // Fallback: assume expression statement
  Node *expr = parse_expression(l);
  if (expr) {
    Node *n = malloc(sizeof(Node));
    n->type = NODE_EXPR_STMT;
    n->as.var_decl.value = expr; // Reusing union slot for simplicity
    n->next = NULL;
    return n;
  }

  return NULL;
}

Node *parse_program(Lexer *l) {
  advance(l); // Init first token
  Node *head = NULL;
  Node *tail = NULL;

  while (current_tok.type != TOKEN_EOF) {
    Node *stmt = parse_statement(l);
    if (stmt) {
      if (!head)
        head = stmt;
      if (tail)
        tail->next = stmt;
      tail = stmt;
    } else {
      advance(l); // Error recovery: skip unknown
    }
  }
  return head;
}

void print_ast(Node *n, int indent) {
  while (n) {
    for (int i = 0; i < indent; i++)
      printf("  ");
    switch (n->type) {
    case NODE_VAR_DECL:
      printf("VAR_DECL: %s\n", n->as.var_decl.name);
      if (n->as.var_decl.value)
        print_ast(n->as.var_decl.value, indent + 1);
      break;
    case NODE_LITERAL_EXPR:
      printf("LITERAL: %s\n", n->as.literal_val);
      break;
    case NODE_IDENTIFIER_EXPR:
      printf("IDENTIFIER: %s\n", n->as.id_name);
      break;
    case NODE_EXPR_STMT:
      printf("EXPR_STMT\n");
      break;
    default:
      printf("Node Type: %d\n", n->type);
    }
    n = n->next;
  }
}

// Renamed main to avoid redefinition errors when linking with other components.
// This function can be used for standalone parser testing.
int calma_parser_test_main(int argc, char **argv) {
  if (argc < 2)
    return 1;
  FILE *f = fopen(argv[1], "rb");
  if (!f)
    return 1;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *src = malloc(size + 1);
  fread(src, 1, size, f);
  src[size] = '\0';
  fclose(f);

  Lexer l = {src, 0, 1, 1};
  Node *program = parse_program(&l);

  printf("\n--- CALMA NATIVE AST ---\n");
  print_ast(program, 0);

  return 0;
}
