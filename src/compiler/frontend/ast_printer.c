/**
 * @file ast_printer.c
 * @brief AST debug printer
 *
 * Fixes vs original:
 *  - AST_PROGRAM was referenced but never defined in nova_ast_node_type_t
 *  - AST_FOR, AST_BLOCK, AST_LET, AST_ASSIGN, AST_MATCH, AST_STRUCT,
 *    AST_ENUM, AST_TRAIT, AST_IMPL were all in the enum but not handled
 *  - nova_program_print() added for the typed AST layer
 *  - ast_printer.h was empty (no declaration of nova_ast_print)
 */

#include "ast_printer.h"
#include "compiler/ast.h"
#include <stdio.h>

// ─── Legacy nova_ast_node_t printer ────────────────────────────────────────

static void print_indent(int depth) {
  for (int i = 0; i < depth; i++)
    printf("  ");
}

static const char *ast_node_type_name(nova_ast_node_type_t t) {
  switch (t) {
  case AST_LITERAL:
    return "LITERAL";
  case AST_VARIABLE:
    return "VARIABLE";
  case AST_BINARY_OP:
    return "BINARY_OP";
  case AST_UNARY_OP:
    return "UNARY_OP";
  case AST_CALL:
    return "CALL";
  case AST_IF:
    return "IF";
  case AST_WHILE:
    return "WHILE";
  case AST_FOR:
    return "FOR";
  case AST_FUNCTION:
    return "FUNCTION";
  case AST_RETURN:
    return "RETURN";
  case AST_BLOCK:
    return "BLOCK";
  case AST_LET:
    return "LET";
  case AST_ASSIGN:
    return "ASSIGN";
  case AST_MATCH:
    return "MATCH";
  case AST_STRUCT:
    return "STRUCT";
  case AST_ENUM:
    return "ENUM";
  case AST_TRAIT:
    return "TRAIT";
  case AST_IMPL:
    return "IMPL";
  /* FIX: AST_PROGRAM does not exist in the enum — removed */
  default:
    return "UNKNOWN";
  }
}

static void print_ast_node(nova_ast_node_t *node, int depth) {
  if (!node)
    return;

  print_indent(depth);
  printf("%s", ast_node_type_name(node->type));

  /* Print variable name inline if available */
  if (node->type == AST_VARIABLE && node->data.variable.name)
    printf("(%s)", node->data.variable.name);

  printf("\n");

  nova_ast_node_t *child = node->children;
  while (child) {
    print_ast_node(child, depth + 1);
    child = child->next;
  }
}

void nova_ast_print(nova_ast_node_t *node) {
  printf("=== AST TREE ===\n");
  print_ast_node(node, 0);
  printf("=== END AST ===\n");
}

// ─── Typed nova_program_t printer ──────────────────────────────────────────

static void print_type(nova_type_t *type) {
  if (!type) {
    printf("<null>");
    return;
  }
  switch (type->kind) {
  case TYPE_VOID:
    printf("void");
    break;
  case TYPE_BOOL:
    printf("bool");
    break;
  case TYPE_I32:
    printf("i32");
    break;
  case TYPE_I64:
    printf("i64");
    break;
  case TYPE_F32:
    printf("f32");
    break;
  case TYPE_F64:
    printf("f64");
    break;
  case TYPE_U8:
    printf("u8");
    break;
  case TYPE_USIZE:
    printf("usize");
    break;
  case TYPE_STR:
    printf("str");
    break;
  case TYPE_PTR:
    printf("*");
    print_type(type->data.ptr.pointee);
    break;
  case TYPE_PTR_MUT:
    printf("*mut ");
    print_type(type->data.ptr.pointee);
    break;
  case TYPE_POINTER:
    printf("ptr<");
    print_type(type->data.ptr.pointee);
    printf(">");
    break;
  case TYPE_ARRAY:
    printf("[");
    print_type(type->data.ptr.pointee);
    if (type->data.ptr.size)
      printf("; %zu", type->data.ptr.size);
    printf("]");
    break;
  case TYPE_DATA:
    printf("%s", type->data.name ? type->data.name : "?");
    break;
  case TYPE_VAR:
    printf("%s", type->data.var_name ? type->data.var_name : "?");
    break;
  case TYPE_FLOW:
    switch (type->data.flow.flow_kind) {
    case FLOW_SIGNAL:
      printf("Signal<");
      break;
    case FLOW_STREAM:
      printf("Stream<");
      break;
    case FLOW_TASK:
      printf("Task<");
      break;
    case FLOW_CHAN:
      printf("Chan<");
      break;
    }
    print_type(type->data.flow.inner);
    printf(">");
    break;
  default:
    printf("<type:%d>", type->kind);
    break;
  }
}

static void print_expr(nova_expr_t *expr, int depth) {
  if (!expr) {
    print_indent(depth);
    printf("<null_expr>\n");
    return;
  }
  print_indent(depth);
  switch (expr->kind) {
  case EXPR_INT:
    printf("int(%lld)\n", (long long)expr->data.lit_int);
    break;
  case EXPR_FLOAT:
    printf("float(%g)\n", expr->data.lit_float);
    break;
  case EXPR_BOOL:
    printf("bool(%s)\n", expr->data.lit_bool ? "true" : "false");
    break;
  case EXPR_STR:
    printf("str(\"%s\")\n", expr->data.lit_str ? expr->data.lit_str : "");
    break;
  case EXPR_IDENT:
    printf("ident(%s)\n", expr->data.ident ? expr->data.ident : "?");
    break;
  case EXPR_BINARY:
    printf("binary(%s)\n", expr->data.binary.op ? expr->data.binary.op : "?");
    print_expr(expr->data.binary.left, depth + 1);
    print_expr(expr->data.binary.right, depth + 1);
    break;
  case EXPR_ASSIGN:
    printf("assign(%s)\n",
           expr->data.assign.name ? expr->data.assign.name : "?");
    print_expr(expr->data.assign.value, depth + 1);
    break;
  case EXPR_CALL:
    printf("call\n");
    print_expr(expr->data.call.func, depth + 1);
    for (size_t i = 0; i < expr->data.call.arg_count; i++)
      print_expr(expr->data.call.args[i], depth + 1);
    break;
  default:
    printf("expr(%d)\n", expr->kind);
    break;
  }
}

static void print_stmt(nova_stmt_t *stmt, int depth);

static void print_fn_stmt(nova_stmt_t *s, int depth) {
  print_indent(depth);
  printf("fn %s(", s->data.fn_stmt.name ? s->data.fn_stmt.name : "?");
  for (size_t i = 0; i < s->data.fn_stmt.param_count; i++) {
    if (i)
      printf(", ");
    printf("%s: ", s->data.fn_stmt.params[i]->name);
    print_type(s->data.fn_stmt.params[i]->type);
  }
  printf(") -> ");
  print_type(s->data.fn_stmt.return_type);
  printf("\n");
  if (s->data.fn_stmt.body)
    print_stmt(s->data.fn_stmt.body, depth + 1);
}

static void print_stmt(nova_stmt_t *stmt, int depth) {
  if (!stmt)
    return;
  switch (stmt->kind) {
  case STMT_FN:
    print_fn_stmt(stmt, depth);
    break;
  case STMT_RETURN:
    print_indent(depth);
    printf("return\n");
    print_expr(stmt->data.return_expr, depth + 1);
    break;
  case STMT_YIELD:
    print_indent(depth);
    printf("yield\n");
    print_expr(stmt->data.yield_stmt, depth + 1);
    break;
  case STMT_VAR_DECL:
    print_indent(depth);
    printf("let %s: ",
           stmt->data.var_decl.name ? stmt->data.var_decl.name : "?");
    print_type(stmt->data.var_decl.type);
    printf("\n");
    if (stmt->data.var_decl.init)
      print_expr(stmt->data.var_decl.init, depth + 1);
    break;
  case STMT_EXPR:
    print_expr(stmt->data.expr, depth);
    break;
  case STMT_BLOCK:
    print_indent(depth);
    printf("block\n");
    for (size_t i = 0; i < stmt->data.block.count; i++)
      print_stmt(stmt->data.block.statements[i], depth + 1);
    break;
  case STMT_CHECK:
    print_indent(depth);
    printf("if\n");
    print_expr(stmt->data.check_stmt.condition, depth + 1);
    print_stmt(stmt->data.check_stmt.then_branch, depth + 1);
    if (stmt->data.check_stmt.else_branch)
      print_stmt(stmt->data.check_stmt.else_branch, depth + 1);
    break;
  case STMT_WHILE:
    print_indent(depth);
    printf("while\n");
    print_expr(stmt->data.while_stmt.condition, depth + 1);
    print_stmt(stmt->data.while_stmt.body, depth + 1);
    break;
  case STMT_FOR:
    print_indent(depth);
    printf("for %s in\n",
           stmt->data.for_stmt.binding ? stmt->data.for_stmt.binding : "?");
    print_expr(stmt->data.for_stmt.start, depth + 1);
    print_expr(stmt->data.for_stmt.end, depth + 1);
    print_stmt(stmt->data.for_stmt.body, depth + 1);
    break;
  case STMT_EACH:
    print_indent(depth);
    printf("each %s in\n",
           stmt->data.each_stmt.binding ? stmt->data.each_stmt.binding : "?");
    print_expr(stmt->data.each_stmt.iterator, depth + 1);
    print_stmt(stmt->data.each_stmt.body, depth + 1);
    break;
  case STMT_STRUCT_DECL:
    print_indent(depth);
    printf("data %s\n", stmt->data.struct_decl->name);
    break;
  case STMT_ENUM_DECL:
    print_indent(depth);
    printf("cases %s\n", stmt->data.enum_decl.name);
    break;
  default:
    print_indent(depth);
    printf("stmt(%d)\n", stmt->kind);
    break;
  }
}

void nova_program_print(nova_program_t *program) {
  if (!program) {
    printf("<null program>\n");
    return;
  }
  printf("=== PROGRAM: %s ===\n", program->filename ? program->filename : "?");
  for (size_t i = 0; i < program->declaration_count; i++) {
    nova_top_level_t *d = program->declarations[i];
    if (!d)
      continue;
    if (d->kind == DECL_FUNCTION)
      print_stmt((nova_stmt_t *)d->data, 0);
    else
      printf("decl(%d)\n", d->kind);
  }
  printf("=== END PROGRAM ===\n");
}
