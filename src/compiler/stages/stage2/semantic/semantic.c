/**
 * @file semantic.c
 * @brief Semantic analysis for Nova compiler
 */

#include <compiler/ast.h>
#include <compiler/dimensions.h>
#include <compiler/module_registry.h>
#include <compiler/physics_constants.h>
#include <compiler/semantic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global module registry — initialized once per compiler session */
static nova_module_registry_t *g_module_registry = NULL;

/**
 * nova_semantic_set_module_registry: Attach an external module registry
 * to the semantic analyzer. Called by the compiler before analysis.
 */
void nova_semantic_set_module_registry(nova_module_registry_t *reg) {
  g_module_registry = reg;
}

/**
 * nova_semantic_get_module_registry: Get the current global registry.
 */
nova_module_registry_t *nova_semantic_get_module_registry(void) {
  if (!g_module_registry) {
    g_module_registry = nova_module_registry_create();
  }
  return g_module_registry;
}

// Forward declarations
typedef struct nova_symbol_table nova_symbol_table_t;
typedef struct function_context function_context_t;
static nova_type_t *analyze_expression(nova_semantic_t *semantic,
                                       nova_expr_t *expr,
                                       nova_symbol_table_t *table);
static bool analyze_statement_with_context(nova_semantic_t *semantic,
                                           nova_stmt_t *stmt,
                                           nova_symbol_table_t *table,
                                           function_context_t *fn_ctx);
static bool analyze_statement(nova_semantic_t *semantic, nova_stmt_t *stmt,
                              nova_symbol_table_t *table);
#include <stdarg.h>

// ══════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

// Function context for tracking return types
typedef struct function_context {
  const nova_type_t
      *declared_return; // stmt->data.fn_stmt.return_type (NULL if not declared)
  nova_type_t *inferred_return; // NULL initially, set on first yield expr
  bool has_return;
} function_context_t;

/* nova_type_clone is public API declared in ast.h, implemented in ast.c */
static bool analyze_statement(nova_semantic_t *semantic, nova_stmt_t *stmt,
                              struct nova_symbol_table *table);
static bool analyze_statement_with_context(nova_semantic_t *semantic,
                                           nova_stmt_t *stmt,
                                           struct nova_symbol_table *table,
                                           function_context_t *fn_ctx);

// ══════════════════════════════════════════════════════════════════════════════
// TYPE UTILITIES
// ══════════════════════════════════════════════════════════════════════════════

/**
 * nova_type_name: Get human-readable type name
 */
static const char *nova_type_name(const nova_type_t *type) {
  if (!type)
    return "unknown";

  switch (type->kind) {
  case TYPE_VOID:
    return "void";
  case TYPE_BOOL:
    return "bool";
  case TYPE_I32:
    return "i32";
  case TYPE_I64:
    return "i64";
  case TYPE_F32:
    return "f32";
  case TYPE_F64:
    return "f64";
  case TYPE_USIZE:
    return "usize";
  case TYPE_U8:
    return "u8";
  case TYPE_STR:
    return "str";
  case TYPE_PTR:
    return "pointer";
  case TYPE_PTR_MUT:
    return "mut pointer";
  case TYPE_DATA:
    return type->data.name ? type->data.name : "data";
  case TYPE_ARRAY:
    return "array";
  case TYPE_FN:
    return "function";
  case TYPE_POINTER:
    return "pointer";
  case TYPE_QTY:
    return type->data.qty.unit_expr ? type->data.qty.unit_expr : "qty";
  default:
    return "unknown";
  }
}

/** * nova_type_clone: Recursive deep copy.
 * The returned pointer must be freed with nova_type_free().
 */
/* nova_type_clone is now defined in ast.c and declared in ast.h (public API).
 * We keep a local alias here for readability — no-op. */

// ══════════════════════════════════════════════════════════════════════════════
// SCOPE STACK
// ══════════════════════════════════════════════════════════════════════════════

#define SCOPE_STACK_MAX 64

typedef struct nova_symbol_table {
  struct nova_symbol *symbols;
  int next_global_index;
  int next_local_index;
  int scope_depth;
  /* Snapshot of local index saved on each scope entry */
  int local_index_stack[SCOPE_STACK_MAX];
} nova_symbol_table_t;

// ══════════════════════════════════════════════════════════════════════════════
// SYMBOL TABLE
// ══════════════════════════════════════════════════════════════════════════════

// Symbol type defined in semantic.h

static nova_symbol_table_t *nova_symbol_table_create(void) {
  nova_symbol_table_t *table = malloc(sizeof(nova_symbol_table_t));
  if (!table)
    return NULL;
  table->symbols = NULL;
  table->next_global_index = 0;
  table->next_local_index = 0;
  table->scope_depth = 0;
  return table;
}

static void nova_symbol_table_destroy(nova_symbol_table_t *table) {
  nova_symbol_t *cur = table->symbols;
  while (cur) {
    nova_symbol_t *next = cur->next;
    free(cur->name);
    nova_type_free(cur->type);
    free(cur);
    cur = next;
  }
  free(table);
}

/** * nova_symbol_table_add: Adds a symbol.
 * [Fix 1] Type is always cloned — ownership of the pointer at call site
 * unchanged, no double-free risk.
 */
static void nova_symbol_table_add(nova_symbol_table_t *table, const char *name,
                                  const nova_type_t *type, bool is_mutable) {
  nova_symbol_t *sym = malloc(sizeof(nova_symbol_t));
  if (!sym)
    return;

  sym->name = strdup(name);
  sym->type = nova_type_clone(type); /* [Fix 1] */
  sym->is_mutable = is_mutable;
  sym->is_global = (table->scope_depth == 0);
  sym->depth = table->scope_depth;

  sym->index =
      sym->is_global ? table->next_global_index++ : table->next_local_index++;

  sym->next = table->symbols;
  table->symbols = sym;
}

static nova_symbol_t *nova_symbol_table_lookup(nova_symbol_table_t *table,
                                               const char *name) {
  nova_symbol_t *cur = table->symbols;
  while (cur) {
    if (strcmp(cur->name, name) == 0)
      return cur;
    cur = cur->next;
  }
  return NULL;
}

static void nova_symbol_table_update_type(nova_symbol_table_t *table,
                                          const char *name,
                                          const nova_type_t *new_type) {
  nova_symbol_t *sym = nova_symbol_table_lookup(table, name);
  if (sym) {
    nova_type_free(sym->type);
    sym->type = nova_type_clone(new_type);
  }
}

/** * begin_scope: [Fix 6] Current next_local_index saved to stack.
 */
static void begin_scope(nova_symbol_table_t *table) {
  if (table->scope_depth < SCOPE_STACK_MAX)
    table->local_index_stack[table->scope_depth] = table->next_local_index;
  table->scope_depth++;
}

/** * end_scope: [Fix 5] Removes all symbols at current depth.
 *            [Fix 6] Restores local index to previous value.
 */
static void end_scope(nova_symbol_table_t *table) {
  if (table->scope_depth == 0)
    return;

  /* Remove and free symbols at current depth */
  nova_symbol_t **pp = &table->symbols;
  while (*pp) {
    nova_symbol_t *cur = *pp;
    if (cur->depth == table->scope_depth) {
      *pp = cur->next;
      free(cur->name);
      nova_type_free(cur->type);
      free(cur);
    } else {
      pp = &cur->next;
    }
  }

  table->scope_depth--;

  /* Restore local index to value before entering this scope */
  if (table->scope_depth < SCOPE_STACK_MAX)
    table->next_local_index = table->local_index_stack[table->scope_depth];
  else if (table->scope_depth == 0)
    table->next_local_index = 0;
}

// ══════════════════════════════════════════════════════════════════════════════
// SEMANTIC ANALYZER STRUCTURE
// ══════════════════════════════════════════════════════════════════════════════

struct nova_semantic {
  nova_program_t *ast;
  nova_symbol_table_t *nova_symbol_table;
  char *error_message;
};

/** * semantic_set_error: [Fix 10] Frees previous message, then writes new one.
 */
static void semantic_set_error(nova_semantic_t *semantic, const char *fmt,
                               ...) {
  free(semantic->error_message); /* [Fix 10] */
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  semantic->error_message = strdup(buf);
}

// ══════════════════════════════════════════════════════════════════════════════
// TYPE HELPERS
// ══════════════════════════════════════════════════════════════════════════════

static bool type_is_numeric(const nova_type_t *t) {
  return t &&
         (t->kind == TYPE_I32 || t->kind == TYPE_I64 || t->kind == TYPE_F32 ||
          t->kind == TYPE_F64 || t->kind == TYPE_U8 || t->kind == TYPE_USIZE ||
          t->kind == TYPE_BOOL);
}

static bool type_is_float(const nova_type_t *t) {
  return t && (t->kind == TYPE_F32 || t->kind == TYPE_F64);
}

static bool type_is_pointer(const nova_type_t *t) {
  return t && (t->kind == TYPE_PTR || t->kind == TYPE_PTR_MUT ||
               t->kind == TYPE_POINTER);
}

static bool types_compatible(const nova_type_t *a, const nova_type_t *b) {
  if (!a || !b)
    return false;

  /* Sovereign v10: 'any' is compatible with everything in Stage 0 */
  if (a->kind == TYPE_DATA && a->data.name && strcmp(a->data.name, "any") == 0)
    return true;
  if (b->kind == TYPE_DATA && b->data.name && strcmp(b->data.name, "any") == 0)
    return true;

  if (a->kind == b->kind) {
    /* For DATA types, check names match (or accept if either name is NULL) */
    if (a->kind == TYPE_DATA) {
      if (!a->data.name || !b->data.name)
        return true;
      return strcmp(a->data.name, b->data.name) == 0;
    }
    /* For QTY types: inner types must be compatible.
     * Dimensional checking is done separately in analyze_expression. */
    if (a->kind == TYPE_QTY) {
      return types_compatible(a->data.qty.inner_type, b->data.qty.inner_type);
    }
    return true;
  }
  /* qty<f64, kg> is compatible with plain f64 (scalar ↔ qty coercion for init)
   */
  if (a->kind == TYPE_QTY && type_is_numeric(b))
    return types_compatible(a->data.qty.inner_type, b);
  if (b->kind == TYPE_QTY && type_is_numeric(a))
    return types_compatible(a, b->data.qty.inner_type);
  /* Implicit widening between numeric types */
  if (type_is_numeric(a) && type_is_numeric(b))
    return true;
  /* All pointer variants are interchangeable */
  if (type_is_pointer(a) && type_is_pointer(b))
    return true;
  /* DATA type is compatible with pointers (struct init assigned to pointer) */
  if ((a->kind == TYPE_DATA && type_is_pointer(b)) ||
      (b->kind == TYPE_DATA && type_is_pointer(a)))
    return true;
  /* STR is compatible with pointer types (string literals to *String) */
  if ((a->kind == TYPE_STR && type_is_pointer(b)) ||
      (b->kind == TYPE_STR && type_is_pointer(a)))
    return true;
  /* STR is compatible with DATA (user-defined 'shape String' vs builtin str) */
  if ((a->kind == TYPE_STR && b->kind == TYPE_DATA) ||
      (a->kind == TYPE_DATA && b->kind == TYPE_STR))
    return true;
  return false;
}

/* ── Unit Algebra helpers ────────────────────────────────────────────────── */

/**
 * qty_get_or_parse_dim: Returns the dimension of a qty type.
 * Lazily parses unit_expr into dimension if dimension pointer is NULL.
 * The returned pointer is OWNED by the type — do NOT free it.
 */
static nova_dimension_t *qty_get_or_parse_dim(nova_type_t *qty_type) {
  if (!qty_type || qty_type->kind != TYPE_QTY)
    return NULL;
  if (!qty_type->data.qty.dimension && qty_type->data.qty.unit_expr) {
    qty_type->data.qty.dimension = nova_dim_parse(qty_type->data.qty.unit_expr);
  }
  return (nova_dimension_t *)qty_type->data.qty.dimension;
}

/**
 * make_qty_type: Create a new qty type with the given inner type and dimension.
 * unit_expr is built from nova_dim_to_string so the clone path always works.
 */
static nova_type_t *make_qty_type(nova_type_t *inner, nova_dimension_t *dim) {
  const char *unit_str = dim ? nova_dim_to_string(dim) : "dimensionless";
  nova_type_t *t = nova_type_qty(nova_type_clone(inner), unit_str);
  /* Attach dimension (owned by this type) */
  t->data.qty.dimension = dim;
  return t;
}

// ══════════════════════════════════════════════════════════════════════════════
// EXPRESSION ANALYSIS
// ══════════════════════════════════════════════════════════════════════════════

/** * analyze_expression: Returns the type of the expression.
 * The returned nova_type_t* belongs to the call site; must be freed with
 * nova_type_free(). On error, returns NULL and sets error_message.
 */
static nova_type_t *analyze_expression(nova_semantic_t *semantic,
                                       nova_expr_t *expr,
                                       nova_symbol_table_t *table) {
  if (!expr)
    return NULL;

  /* Check for poisoned/invalid pointer (AST parsing error) */
  if ((uintptr_t)expr == 0xbebebebebebebebe) {
    semantic_set_error(semantic, "Invalid expression (parser error)");
    return NULL;
  }

  switch (expr->kind) {
  case EXPR_INT:
    expr->type = nova_type_i64();
    return nova_type_clone(expr->type);
  case EXPR_FLOAT:
    expr->type = nova_type_f64();
    return nova_type_clone(expr->type);
  case EXPR_STR:
    expr->type = nova_type_str();
    return nova_type_clone(expr->type);
  case EXPR_BOOL:
    expr->type = nova_type_bool();
    return nova_type_clone(expr->type);

  case EXPR_IDENT: {
    nova_symbol_t *sym = nova_symbol_table_lookup(table, expr->data.ident);
    if (!sym) {
      semantic_set_error(semantic, "Undefined variable '%s'", expr->data.ident);
      return NULL;
    }
    if (sym->type) {
      expr->type = nova_type_clone(sym->type);
      return nova_type_clone(expr->type);
    }
    return NULL;
  }

  case EXPR_BINARY: {
    const char *op = expr->data.binary.op;

    /* For the 'in' unit conversion operator, the right side is a unit name
     * (an EXPR_IDENT like "g", "m", "km") — it's NOT a variable reference.
     * Skip semantic analysis of the RHS and handle it directly. */
    if (strcmp(op, "in") == 0) {
      nova_type_t *lt =
          analyze_expression(semantic, expr->data.binary.left, table);
      if (!lt)
        return NULL;
      bool lt_is_qty = (lt->kind == TYPE_QTY);

      /* Dummy rt for the unit algebra 'in' branch below */
      nova_type_t *rt = nova_type_f64(); /* placeholder — won't be used */
      bool rt_is_qty = false;
      nova_type_t *inner = lt_is_qty ? lt->data.qty.inner_type : lt;
      nova_dimension_t *ld = lt_is_qty ? qty_get_or_parse_dim(lt) : NULL;
      (void)rt_is_qty;

      /* RHS must be EXPR_IDENT with unit name */
      const char *to_unit = NULL;
      if (expr->data.binary.right->kind == EXPR_IDENT) {
        to_unit = expr->data.binary.right->data.ident;
      }
      if (!to_unit) {
        semantic_set_error(
            semantic,
            "'in' conversion: right side must be a unit name identifier");
        nova_type_free(lt);
        nova_type_free(rt);
        return NULL;
      }
      nova_dimension_t *to_dim = nova_dim_parse(to_unit);
      if (ld && to_dim && !nova_dim_compatible(ld, to_dim)) {
        semantic_set_error(
            semantic,
            "Unit conversion error: '%s' and '%s' are incompatible dimensions",
            lt->data.qty.unit_expr ? lt->data.qty.unit_expr : "?", to_unit);
        nova_dim_destroy(to_dim);
        nova_type_free(lt);
        nova_type_free(rt);
        return NULL;
      }
      nova_type_t *result = make_qty_type(inner, to_dim);
      nova_type_free(lt);
      nova_type_free(rt);
      return result;
    }

    nova_type_t *lt =
        analyze_expression(semantic, expr->data.binary.left, table);
    if (!lt)
      return NULL;
    nova_type_t *rt =
        analyze_expression(semantic, expr->data.binary.right, table);
    if (!rt) {
      nova_type_free(lt);
      return NULL;
    }

    /* ── Unit Algebra: dimensional analysis for qty types ─────────────── */
    bool lt_is_qty = (lt->kind == TYPE_QTY);
    bool rt_is_qty = (rt->kind == TYPE_QTY);

    if (lt_is_qty || rt_is_qty) {
      /* Determine inner scalar type for result.
       * Clone immediately so it survives nova_type_free(lt/rt). */
      nova_type_t *inner_raw =
          lt_is_qty ? lt->data.qty.inner_type : rt->data.qty.inner_type;
      nova_type_t *inner = nova_type_clone(inner_raw);

      /* Get dimensions (lazily parsed) */
      nova_dimension_t *ld = lt_is_qty ? qty_get_or_parse_dim(lt) : NULL;
      nova_dimension_t *rd = rt_is_qty ? qty_get_or_parse_dim(rt) : NULL;

      nova_type_t *result = NULL;
      bool is_cmp = (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                     strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
                     strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0);

      if (is_cmp) {
        /* Comparisons: both sides must have same dimension */
        if (lt_is_qty && rt_is_qty && ld && rd &&
            !nova_dim_compatible(ld, rd)) {
          semantic_set_error(
              semantic,
              "Dimension mismatch in comparison: cannot compare '%s' and '%s'",
              nova_type_name(lt), nova_type_name(rt));
          nova_type_free(lt);
          nova_type_free(rt);
          return NULL;
        }
        result = nova_type_bool();

      } else if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
        /* Addition/subtraction: dimensions must be compatible */
        if (lt_is_qty && rt_is_qty) {
          if (ld && rd && !nova_dim_compatible(ld, rd)) {
            semantic_set_error(semantic,
                               "Dimension mismatch: cannot %s '%s' and '%s' — "
                               "incompatible units",
                               (strcmp(op, "+") == 0 ? "add" : "subtract"),
                               nova_type_name(lt), nova_type_name(rt));
            nova_type_free(lt);
            nova_type_free(rt);
            return NULL;
          }
          /* Result has same dimension as left operand */
          nova_dimension_t *res_dim =
              ld ? nova_dim_parse(lt->data.qty.unit_expr) : NULL;
          result = make_qty_type(inner, res_dim);
        } else {
          /* qty ± scalar: keep qty type */
          nova_dimension_t *res_dim =
              lt_is_qty ? (ld ? nova_dim_parse(lt->data.qty.unit_expr) : NULL)
                        : (rd ? nova_dim_parse(rt->data.qty.unit_expr) : NULL);
          result = make_qty_type(inner, res_dim);
        }

      } else if (strcmp(op, "*") == 0) {
        /* Multiplication: dimensions multiply */
        nova_dimension_t *res_dim = NULL;
        if (ld && rd) {
          res_dim = nova_dim_multiply(ld, rd);
        } else if (ld) {
          res_dim = nova_dim_parse(lt->data.qty.unit_expr);
        } else if (rd) {
          res_dim = nova_dim_parse(rt->data.qty.unit_expr);
        }
        result = make_qty_type(inner, res_dim);

      } else if (strcmp(op, "**") == 0) {
        /* Power: qty<T, unit> ** n → qty<T, unit^n>
         * RHS must be a plain integer literal exponent. */
        nova_dimension_t *res_dim = NULL;
        int exp = 2; /* default exponent */
        if (expr->data.binary.right->kind == EXPR_INT) {
          exp = (int)expr->data.binary.right->data.lit_int;
        } else if (expr->data.binary.right->kind == EXPR_FLOAT) {
          exp = (int)expr->data.binary.right->data.lit_float;
        }
        if (ld) {
          res_dim = nova_dim_power(ld, exp);
        } else if (rd) {
          res_dim = nova_dim_power(rd, exp);
        }
        result = make_qty_type(inner, res_dim);

      } else if (strcmp(op, "/") == 0) {
        /* Division: dimensions divide */
        nova_dimension_t *res_dim = NULL;
        if (ld && rd) {
          res_dim = nova_dim_divide(ld, rd);
          /* If result is dimensionless, return plain scalar */
          if (nova_dim_is_dimensionless(res_dim)) {
            nova_dim_destroy(res_dim);
            nova_type_free(lt);
            nova_type_free(rt);
            nova_type_t *inner_scalar = nova_type_clone(inner);
            nova_type_free(inner);
            return inner_scalar;
          }
        } else if (ld) {
          res_dim = nova_dim_parse(lt->data.qty.unit_expr);
        }
        result = make_qty_type(inner, res_dim);

      } else if (strcmp(op, "in") == 0) {
        /* Unit conversion: `expr in target_unit`
         * e.g. `10.0.km in m`, `mass in g`
         * Semantics:
         *   - Left side must be qty<T, dim_from>
         *   - Right side is a unit identifier (parsed as EXPR_IDENT)
         *   - Dimensions must be compatible (same physical quantity)
         *   - Result type is qty<T, dim_to> (same dimension, different scale)
         */
        if (!lt_is_qty) {
          semantic_set_error(
              semantic,
              "'in' conversion: left side must be a qty type, got '%s'",
              nova_type_name(lt));
          nova_type_free(lt);
          nova_type_free(rt);
          return NULL;
        }
        /* RHS is the target unit — parse its name from the EXPR_IDENT node */
        const char *to_unit = NULL;
        if (expr->data.binary.right->kind == EXPR_IDENT) {
          to_unit = expr->data.binary.right->data.ident;
        }
        if (!to_unit) {
          semantic_set_error(semantic,
                             "'in' conversion: right side must be a unit name");
          nova_type_free(lt);
          nova_type_free(rt);
          return NULL;
        }
        nova_dimension_t *to_dim = nova_dim_parse(to_unit);
        if (ld && to_dim && !nova_dim_compatible(ld, to_dim)) {
          semantic_set_error(
              semantic,
              "Unit conversion error: '%s' and '%s' are incompatible "
              "dimensions",
              lt->data.qty.unit_expr ? lt->data.qty.unit_expr : "?", to_unit);
          nova_dim_destroy(to_dim);
          nova_type_free(lt);
          nova_type_free(rt);
          return NULL;
        }
        /* Result: qty<inner, to_unit> */
        result = make_qty_type(inner, to_dim);

      } else {
        /* Unknown op with qty: fall through to plain result */
        result = nova_type_clone(inner);
      }

      nova_type_free(inner); /* cloned copy — free after use */
      nova_type_free(lt);
      nova_type_free(rt);
      return result;
    }
    /* ── End unit algebra ─────────────────────────────────────────────── */

    if (!types_compatible(lt, rt)) {
      semantic_set_error(semantic,
                         "Type mismatch in binary expression '%s': cannot "
                         "operate on '%s' and '%s'",
                         op, nova_type_name(lt), nova_type_name(rt));
      nova_type_free(lt);
      nova_type_free(rt);
      return NULL;
    }

    /* Comparators return bool; floats widen to f64; else left type */
    bool is_cmp = (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                   strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
                   strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0);
    nova_type_t *result;
    if (is_cmp) {
      result = nova_type_bool();
    } else if (type_is_float(lt) || type_is_float(rt)) {
      result = nova_type_f64();
    } else {
      result = nova_type_clone(lt);
    }
    nova_type_free(lt);
    nova_type_free(rt);
    return result;
  }

  case EXPR_ASSIGN: {
    nova_symbol_t *sym =
        nova_symbol_table_lookup(table, expr->data.assign.name);
    if (!sym) {
      semantic_set_error(semantic, "Undefined variable '%s' in assignment",
                         expr->data.assign.name);
      return NULL;
    }
    if (!sym->is_mutable) {
      semantic_set_error(semantic, "Cannot assign to immutable variable '%s'",
                         expr->data.assign.name);
      return NULL;
    }
    nova_type_t *vt =
        analyze_expression(semantic, expr->data.assign.value, table);
    if (!vt)
      return NULL;
    if (!types_compatible(sym->type, vt)) {
      semantic_set_error(semantic, "Type mismatch in assignment to '%s'",
                         expr->data.assign.name);
      nova_type_free(vt);
      return NULL;
    }
    nova_type_free(
        vt); // Free the type of the value, as we'll use the symbol's type
    if (sym && sym->type) {
      expr->type = nova_type_clone(sym->type);
      return nova_type_clone(expr->type);
    }
    // Fallback if sym->type is somehow null (shouldn't happen if sym exists)
    expr->type = nova_type_void(); // Or some other default error type
    return nova_type_clone(expr->type);
  }

  case EXPR_ADDR_OF: {
    nova_type_t *ot = analyze_expression(semantic, expr->data.addr_of, table);
    if (!ot)
      return NULL;
    /* nova_type_pointer takes ownership of ot — don't free it */
    return nova_type_pointer(ot);
  }

  case EXPR_DEREF: {
    nova_type_t *pt = analyze_expression(semantic, expr->data.deref, table);
    if (!pt)
      return NULL;

    /* Accept TYPE_POINTER, TYPE_PTR (shared borrow) and TYPE_PTR_MUT (mutable
     * borrow) */
    if (pt->kind != TYPE_POINTER && pt->kind != TYPE_PTR &&
        pt->kind != TYPE_PTR_MUT) {
      semantic_set_error(semantic, "Cannot dereference non-pointer type");
      nova_type_free(pt);
      return NULL;
    }

    /* Take deep copy of pointee, then free ptr */
    nova_type_t *result = nova_type_clone(pt->data.ptr.pointee);
    nova_type_free(pt);
    return result;
  }

  case EXPR_CALL: {
    /* DEBUG */
    if (expr->data.call.func && expr->data.call.func->kind == EXPR_IDENT) {
      /* fprintf(stderr, "DEBUG: Analyzing call to '%s' with %zu args\n",
              expr->data.call.func->data.ident, expr->data.call.arg_count); */
    }

    /* Analyze callee - check if it's a function */
    nova_type_t *ct = analyze_expression(semantic, expr->data.call.func, table);
    if (!ct)
      return NULL;

    /* For function call, callee must be an identifier */
    nova_symbol_t *func_sym = NULL;
    if (expr->data.call.func->kind == EXPR_IDENT) {
      const char *func_name = expr->data.call.func->data.ident;
      func_sym = nova_symbol_table_lookup(table, func_name);
      if (!func_sym) {
        semantic_set_error(semantic, "Undefined function '%s'", func_name);
        nova_type_free(ct);
        return NULL;
      }
    }

    /* Check if function type is TYPE_FN and validate arguments */
    if (func_sym && func_sym->type && func_sym->type->kind == TYPE_FN) {
      size_t expected_count = func_sym->type->data.fn.param_count;
      size_t actual_count = expr->data.call.arg_count;

      /* Check argument count */
      if (actual_count != expected_count) {
        semantic_set_error(semantic,
                           "Function '%s' expects %zu argument%s, got %zu",
                           expr->data.call.func->data.ident, expected_count,
                           expected_count == 1 ? "" : "s", actual_count);
        nova_type_free(ct);
        return NULL;
      }

      /* Analyze and check argument types */
      for (size_t i = 0; i < expr->data.call.arg_count; i++) {
        nova_type_t *at =
            analyze_expression(semantic, expr->data.call.args[i], table);
        if (!at) {
          nova_type_free(ct);
          return NULL;
        }

        /* Type inference: if parameter type is NULL, infer from argument */
        if (i < expected_count && func_sym->type->data.fn.params[i] == NULL) {
          // Infer parameter type from first call
          func_sym->type->data.fn.params[i] = nova_type_clone(at);
        }

        /* Check type compatibility */
        if (i < expected_count && func_sym->type->data.fn.params[i]) {
          nova_type_t *expected_type = func_sym->type->data.fn.params[i];

          /* Simple type equality check (can be extended for coercion) */
          if (!types_compatible(expected_type, at)) {
            semantic_set_error(
                semantic,
                "Type mismatch in argument %zu of function '%s': "
                "expected '%s', got '%s'",
                i + 1, expr->data.call.func->data.ident,
                nova_type_name(expected_type), nova_type_name(at));
            nova_type_free(at);
            nova_type_free(ct);
            return NULL;
          }
        }

        nova_type_free(at);
      }

      nova_type_free(ct);

      /* Return the function's return type */
      if (func_sym->type->data.fn.return_type) {
        expr->type = nova_type_clone(func_sym->type->data.fn.return_type);
        return nova_type_clone(expr->type);
      }
      expr->type = nova_type_i64();
      return nova_type_clone(expr->type);
    }

    /* Old path for non-TYPE_FN functions (builtins, etc.) */
    nova_type_free(ct);

    for (size_t i = 0; i < expr->data.call.arg_count; i++) {
      nova_type_t *at =
          analyze_expression(semantic, expr->data.call.args[i], table);
      if (!at)
        return NULL;
      nova_type_free(at);
    }

    /* Return the function's return type if available */
    if (func_sym && func_sym->type) {
      expr->type = nova_type_clone(func_sym->type);
      return nova_type_clone(expr->type);
    }

    /* Fallback for unknown functions */
    expr->type = nova_type_i64();
    return nova_type_clone(expr->type);
  }

  case EXPR_FIELD_ACCESS: {
    nova_type_t *base =
        analyze_expression(semantic, expr->data.field_access.object, table);
    nova_type_free(base);

    expr->type = nova_type_i64();
    return nova_type_clone(expr->type);
  }

  case EXPR_HEAP_NEW: {
    /* heap new T(...) returns *T (pointer to T) */
    if (!expr->data.heap_new.type) {
      semantic_set_error(semantic, "heap new requires a type");
      return NULL;
    }

    /* Analyze constructor arguments */
    for (size_t i = 0; i < expr->data.heap_new.arg_count; i++) {
      nova_type_t *at =
          analyze_expression(semantic, expr->data.heap_new.args[i], table);
      if (!at)
        return NULL;
      nova_type_free(at);
    }

    /* Return pointer to the allocated type */
    expr->type = nova_type_ptr(nova_type_clone(expr->data.heap_new.type));
    return nova_type_clone(expr->type);
  }

  case EXPR_STRUCT_INIT: {
    /* Struct initializer: StructName { ... } returns TYPE_DATA */
    if (expr->data.struct_init.struct_name) {
      /* Analyze field values */
      for (size_t i = 0; i < expr->data.struct_init.field_count; i++) {
        if (expr->data.struct_init.fields &&
            expr->data.struct_init.fields[i].value) {
          nova_type_t *ft = analyze_expression(
              semantic, expr->data.struct_init.fields[i].value, table);
          nova_type_free(ft);
        }
      }
      expr->type = nova_type_data(expr->data.struct_init.struct_name);
      return nova_type_clone(expr->type);
    }
    expr->type = nova_type_i64();
    return nova_type_clone(expr->type);
  }

  case EXPR_STRING_LEN:
    if (expr->data.string_len) {
      nova_type_t *st =
          analyze_expression(semantic, expr->data.string_len, table);
      nova_type_free(st);
    }
    expr->type = nova_type_usize();
    return nova_type_clone(expr->type);

  case EXPR_STRING_SLICE:
    if (expr->data.string_slice.string) {
      nova_type_t *st =
          analyze_expression(semantic, expr->data.string_slice.string, table);
      nova_type_free(st);
    }
    if (expr->data.string_slice.start) {
      nova_type_t *st =
          analyze_expression(semantic, expr->data.string_slice.start, table);
      nova_type_free(st);
    }
    if (expr->data.string_slice.end) {
      nova_type_t *et =
          analyze_expression(semantic, expr->data.string_slice.end, table);
      nova_type_free(et);
    }
    return nova_type_str();

  case EXPR_STRING_CONCAT: {
    if (expr->data.string_concat.left) {
      nova_type_t *lt =
          analyze_expression(semantic, expr->data.string_concat.left, table);
      nova_type_free(lt);
    }
    if (expr->data.string_concat.right) {
      nova_type_t *rt =
          analyze_expression(semantic, expr->data.string_concat.right, table);
      nova_type_free(rt);
    }
    expr->type = nova_type_str();
    return nova_type_clone(expr->type);
  }

  case EXPR_ARRAY_LIT: {
    nova_type_t *elem_type = NULL;
    for (size_t i = 0; i < expr->data.array_lit.count; i++) {
      nova_type_t *at =
          analyze_expression(semantic, expr->data.array_lit.elements[i], table);
      if (!at)
        return NULL;
      if (!elem_type) {
        elem_type = at;
      } else {
        if (!types_compatible(elem_type, at)) {
          semantic_set_error(semantic, "Inconsistent types in array literal");
          nova_type_free(at);
          nova_type_free(elem_type);
          return NULL;
        }
        nova_type_free(at);
      }
    }
    if (!elem_type)
      elem_type = nova_type_i64(); // Default for empty array
    nova_type_t *res = nova_type_array(elem_type, expr->data.array_lit.count);
    expr->type = nova_type_clone(res);
    return res;
  }

  case EXPR_INDEX: {
    nova_type_t *ot =
        analyze_expression(semantic, expr->data.index.object, table);
    if (!ot)
      return NULL;
    nova_type_t *it =
        analyze_expression(semantic, expr->data.index.index, table);
    if (!it) {
      nova_type_free(ot);
      return NULL;
    }

    if (!type_is_numeric(it)) {
      semantic_set_error(semantic, "Array index must be numeric");
      nova_type_free(ot);
      nova_type_free(it);
      return NULL;
    }
    nova_type_free(it);

    if (ot->kind != TYPE_ARRAY && ot->kind != TYPE_POINTER &&
        ot->kind != TYPE_STR) {
      semantic_set_error(semantic, "Cannot index into non-indexable type");
      nova_type_free(ot);
      return NULL;
    }

    nova_type_t *res;
    if (ot->kind == TYPE_ARRAY) {
      res = nova_type_clone(ot->data.ptr.pointee);
    } else if (ot->kind == TYPE_POINTER) {
      res = nova_type_clone(ot->data.ptr.pointee);
    } else {
      /* String indexing returns u8 (char) */
      res = nova_type_u8();
    }
    nova_type_free(ot);
    expr->type = nova_type_clone(res);
    return res;
  }

  case EXPR_ENUM_VARIANT: {
    /* Look up the enum type */
    nova_symbol_t *enum_sym =
        nova_symbol_table_lookup(table, expr->data.enum_variant.enum_name);
    if (!enum_sym) {
      semantic_set_error(semantic, "Undefined enum '%s'",
                         expr->data.enum_variant.enum_name);
      return NULL;
    }

    /* Verify it's a TYPE_DATA (enum/struct) */
    if (enum_sym->type->kind != TYPE_DATA) {
      semantic_set_error(semantic, "'%s' is not an enum type",
                         expr->data.enum_variant.enum_name);
      return NULL;
    }

    /* For now, return the enum type itself */
    /* TODO: Validate variant exists and check argument types */
    nova_type_t *enum_type = nova_type_clone(enum_sym->type);
    expr->type = nova_type_clone(enum_type);
    return enum_type;
  }

  case EXPR_MATCH: {
    /* Analyze the target expression */
    nova_type_t *target_type =
        analyze_expression(semantic, expr->data.match.target, table);
    if (!target_type)
      return NULL;
    nova_type_free(target_type);

    /* Analyze each arm body and infer the result type from the first arm */
    nova_type_t *result_type = NULL;
    for (size_t i = 0; i < expr->data.match.arm_count; i++) {
      nova_match_arm_t *arm = expr->data.match.arms[i];
      if (!arm || !arm->body)
        continue;
      nova_type_t *arm_type = analyze_expression(semantic, arm->body, table);
      if (!arm_type) {
        nova_type_free(result_type);
        return NULL;
      }
      if (!result_type) {
        result_type = arm_type; /* first arm sets the type */
      } else {
        nova_type_free(arm_type); /* subsequent arms — ignore for now */
      }
    }
    if (!result_type)
      result_type = nova_type_i64(); /* fallback */
    expr->type = nova_type_clone(result_type);
    return result_type;
  }

  default:
    return nova_type_i64();
  }
}

static void register_pattern_bindings(nova_semantic_t *semantic,
                                      nova_pattern_t *pat,
                                      nova_symbol_table_t *table) {
  if (!pat)
    return;
  switch (pat->kind) {
  case PATTERN_IDENT:
    if (pat->data.ident) {
      nova_type_t *t = nova_type_i64(); // Simplified for Stage 0
      nova_symbol_table_add(table, pat->data.ident, t, true);
      nova_type_free(t);
    }
    break;
  case PATTERN_ENUM:
    for (size_t i = 0; i < pat->data.variant.param_count; i++) {
      register_pattern_bindings(semantic, pat->data.variant.params[i], table);
    }
    break;
  default:
    break;
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// STATEMENT ANALYSIS
// ══════════════════════════════════════════════════════════════════════════════

static bool analyze_statement_with_context(nova_semantic_t *semantic,
                                           nova_stmt_t *stmt,
                                           nova_symbol_table_t *table,
                                           function_context_t *fn_ctx) {
  if (!stmt)
    return true;

  switch (stmt->kind) {
  case STMT_VAR_DECL: {
    /* [Fix 4] Ownership flow:
     *   - init_type: from analyze_expression → owned by us → must free
     *   - var_decl.type: owned by AST → don't touch
     *   - nova_symbol_table_add always clones → we can always free
     */
    nova_type_t *init_type = NULL;
    if (stmt->data.var_decl.init) {
      init_type = analyze_expression(semantic, stmt->data.var_decl.init, table);
      /* If analyze_expression returned NULL AND set an error, propagate it */
      if (!init_type && semantic->error_message) {
        return false;
      }
    }

    /* Type to use: explicit annotation > init_type > default i64 */
    const nova_type_t *var_type =
        stmt->data.var_decl.type ? stmt->data.var_decl.type : init_type;

    if (init_type && stmt->data.var_decl.type) {
      if (!types_compatible(stmt->data.var_decl.type, init_type)) {
        semantic_set_error(semantic, "Type mismatch in declaration of '%s'",
                           stmt->data.var_decl.name);
        nova_type_free(init_type);
        return false;
      }
    }

    if (var_type) {
      nova_symbol_table_add(table, stmt->data.var_decl.name, var_type, true);
    } else {
      nova_type_t *def = nova_type_i64();
      nova_symbol_table_add(table, stmt->data.var_decl.name, def, true);
      nova_type_free(def);
    }

    nova_type_free(init_type); /* safe after clone */
    return true;
  }

  case STMT_EXPR: {
    nova_type_t *t = analyze_expression(semantic, stmt->data.expr, table);
    if (!t)
      return false;
    nova_type_free(t);
    return true;
  }

  case STMT_CHECK: {
    nova_type_t *ct =
        analyze_expression(semantic, stmt->data.check_stmt.condition, table);
    if (!ct)
      return false;
    if (!stmt->data.check_stmt.pattern && ct->kind != TYPE_BOOL) {
      semantic_set_error(semantic, "Condition expression must be bool");
      nova_type_free(ct);
      return false;
    }
    nova_type_free(ct);

    begin_scope(table);
    if (stmt->data.check_stmt.pattern) {
      register_pattern_bindings(semantic, stmt->data.check_stmt.pattern, table);
    }

    bool res = analyze_statement_with_context(
        semantic, stmt->data.check_stmt.then_branch, table, fn_ctx);
    end_scope(table);

    if (!res)
      return false;

    if (stmt->data.check_stmt.else_branch) {
      if (!analyze_statement_with_context(
              semantic, stmt->data.check_stmt.else_branch, table, fn_ctx))
        return false;
    }
    return true;
  }

  case STMT_WHILE: {
    nova_type_t *ct =
        analyze_expression(semantic, stmt->data.while_stmt.condition, table);
    if (!ct)
      return false;
    if (ct->kind != TYPE_BOOL) {
      semantic_set_error(semantic, "While condition must be bool");
      nova_type_free(ct);
      return false;
    }
    nova_type_free(ct);

    if (stmt->data.while_stmt.body)
      if (!analyze_statement_with_context(semantic, stmt->data.while_stmt.body,
                                          table, fn_ctx))
        return false;
    return true;
  }

  case STMT_FOR: {
    nova_type_t *st =
        analyze_expression(semantic, stmt->data.for_stmt.start, table);
    if (!st)
      return false;
    nova_type_t *et =
        analyze_expression(semantic, stmt->data.for_stmt.end, table);
    if (!et) {
      nova_type_free(st);
      return false;
    }

    if (!type_is_numeric(st) || !type_is_numeric(et)) {
      semantic_set_error(semantic,
                         "For loop range start and end must be numeric");
      nova_type_free(st);
      nova_type_free(et);
      return false;
    }
    nova_type_free(st);
    nova_type_free(et);

    begin_scope(table);
    nova_type_t *it = nova_type_i64();
    nova_symbol_table_add(table, stmt->data.for_stmt.binding, it, false);
    nova_type_free(it);

    bool res = analyze_statement_with_context(
        semantic, stmt->data.for_stmt.body, table, fn_ctx);
    end_scope(table);
    return res;
  }

  case STMT_RETURN:
  case STMT_YIELD: {
    if (fn_ctx)
      fn_ctx->has_return = true;

    // Handle both return and yield expressions
    nova_expr_t *return_val = (stmt->kind == STMT_RETURN)
                                  ? stmt->data.return_expr
                                  : stmt->data.yield_stmt;

    // return; or yield; (no expr - void return)
    if (!return_val) {
      if (fn_ctx) {
        // declared return exists: must be void
        if (fn_ctx->declared_return &&
            fn_ctx->declared_return->kind != TYPE_VOID) {
          semantic_set_error(semantic,
                             "Return type mismatch: expected non-void");
          return false;
        }
        // inferred exists and non-void: conflict
        if (!fn_ctx->declared_return && fn_ctx->inferred_return &&
            fn_ctx->inferred_return->kind != TYPE_VOID) {
          semantic_set_error(
              semantic, "Return type mismatch: mixed void and value returns");
          return false;
        }
      }
      return true;
    }

    // return <expr> or yield <expr>;
    nova_type_t *t = analyze_expression(semantic, return_val, table);
    if (!t)
      return false;

    if (fn_ctx && fn_ctx->declared_return) {
      // declared return exists: enforce it
      if (!types_compatible(fn_ctx->declared_return, t)) {
        semantic_set_error(semantic,
                           "Return type mismatch: expected different type");
        nova_type_free(t);
        return false;
      }
      nova_type_free(t);
      return true;
    }

    if (fn_ctx && !fn_ctx->declared_return) {
      // no declared return => infer / consistency check
      if (!fn_ctx->inferred_return) {
        fn_ctx->inferred_return = nova_type_clone(t);
        nova_type_free(t);
        return true;
      }

      if (!types_compatible(fn_ctx->inferred_return, t)) {
        semantic_set_error(semantic, "Inconsistent inferred return types");
        nova_type_free(t);
        return false;
      }
    }

    nova_type_free(t);
    return true;
  }

  case STMT_FN: {
    /* For nested functions (scope_depth > 0), register in current scope.
     * Top-level functions are registered in PASS 1 during analyze_program. */
    if (stmt->data.fn_stmt.name && table->scope_depth > 0) {
      /* Build TYPE_FN with parameter types */
      nova_type_t **param_types = NULL;
      if (stmt->data.fn_stmt.param_count > 0) {
        param_types =
            malloc(sizeof(nova_type_t *) * stmt->data.fn_stmt.param_count);
        for (size_t j = 0; j < stmt->data.fn_stmt.param_count; j++) {
          if (stmt->data.fn_stmt.params[j]->type) {
            param_types[j] =
                nova_type_clone(stmt->data.fn_stmt.params[j]->type);
          } else {
            // Leave NULL for type inference from call site
            param_types[j] = NULL;
          }
        }
      }
      nova_type_t *return_type =
          stmt->data.fn_stmt.return_type
              ? nova_type_clone(stmt->data.fn_stmt.return_type)
              : nova_type_i64();
      nova_type_t *fn_type = malloc(sizeof(nova_type_t));
      fn_type->kind = TYPE_FN;
      fn_type->data.fn.params = param_types;
      fn_type->data.fn.param_count = stmt->data.fn_stmt.param_count;
      fn_type->data.fn.return_type = return_type;
      nova_symbol_table_add(table, stmt->data.fn_stmt.name, fn_type, false);
      nova_type_free(fn_type);
    }
    begin_scope(table);

    /* [Fix 7] Use param->type; fallback to i32 if NULL */
    /* Check for duplicate parameters */
    for (size_t i = 0; i < stmt->data.fn_stmt.param_count; i++) {
      nova_param_t *p = stmt->data.fn_stmt.params[i];

      /* Check if parameter name already exists in current scope */
      for (size_t j = 0; j < i; j++) {
        if (strcmp(p->name, stmt->data.fn_stmt.params[j]->name) == 0) {
          semantic_set_error(semantic, "duplicate parameter name '%s'",
                             p->name);
          end_scope(table);
          return false;
        }
      }

      if (p->type) {
        nova_symbol_table_add(table, p->name, p->type, true);
      } else {
        nova_type_t *def = nova_type_i32();
        nova_symbol_table_add(table, p->name, def, true);
        nova_type_free(def);
      }
    }

    /* Create function context for return type checking */
    function_context_t fn_context = {.declared_return =
                                         stmt->data.fn_stmt.return_type,
                                     .inferred_return = NULL,
                                     .has_return = false};

    bool ok = !stmt->data.fn_stmt.body ||
              analyze_statement_with_context(semantic, stmt->data.fn_stmt.body,
                                             table, &fn_context);

    /* Finalize return type checking */
    if (ok) {
      const nova_type_t *effective_return = fn_context.declared_return
                                                ? fn_context.declared_return
                                                : fn_context.inferred_return;

      /* Check if non-void function has return statement */
      if (effective_return && effective_return->kind != TYPE_VOID) {
        if (!fn_context.has_return) {
          /* Check for implicit tail expression (Rust/Nova style):
           * If the body's last statement is STMT_EXPR, treat it as
           * an implicit return. This supports: fn new() -> T { ... result } */
          bool has_tail_expr = false;
          nova_stmt_t *body = stmt->data.fn_stmt.body;
          if (body && body->kind == STMT_BLOCK && body->data.block.count > 0) {
            nova_stmt_t *last =
                body->data.block.statements[body->data.block.count - 1];
            if (last && last->kind == STMT_EXPR) {
              has_tail_expr = true;
            }
          }
          if (!has_tail_expr) {
            semantic_set_error(semantic,
                               "missing return statement in function '%s'",
                               stmt->data.fn_stmt.name ? stmt->data.fn_stmt.name
                                                       : "<anonymous>");
            ok = false;
          }
        }
      }

      /* Update function type in symbol table with finalized type */
      if (stmt->data.fn_stmt.name && table->scope_depth == 1) {
        /* scope_depth == 1 because we're inside the function's scope */
        /* Update in parent (global) scope */
        nova_type_t *fn_type =
            fn_context.declared_return
                ? nova_type_clone(fn_context.declared_return)
                : (fn_context.inferred_return
                       ? nova_type_clone(fn_context.inferred_return)
                       : nova_type_void());

        /* Temporarily exit scope to update in global */
        table->scope_depth--;

        /* Get original function type to preserve parameters */
        nova_symbol_t *orig_sym =
            nova_symbol_table_lookup(table, stmt->data.fn_stmt.name);
        if (orig_sym && orig_sym->type && orig_sym->type->kind == TYPE_FN) {
          nova_type_t *new_fn_type = malloc(sizeof(nova_type_t));
          new_fn_type->kind = TYPE_FN;
          new_fn_type->data.fn.param_count =
              orig_sym->type->data.fn.param_count;
          if (new_fn_type->data.fn.param_count > 0) {
            new_fn_type->data.fn.params = malloc(
                sizeof(nova_type_t *) * new_fn_type->data.fn.param_count);
            for (size_t k = 0; k < new_fn_type->data.fn.param_count; k++) {
              new_fn_type->data.fn.params[k] =
                  nova_type_clone(orig_sym->type->data.fn.params[k]);
            }
          } else {
            new_fn_type->data.fn.params = NULL;
          }
          new_fn_type->data.fn.return_type = nova_type_clone(fn_type);

          nova_symbol_table_update_type(table, stmt->data.fn_stmt.name,
                                        new_fn_type);
          nova_type_free(new_fn_type);
        } else {
          nova_symbol_table_update_type(table, stmt->data.fn_stmt.name,
                                        fn_type);
        }

        table->scope_depth++;

        nova_type_free(fn_type);
      }
    }

    /* Cleanup inferred type */
    nova_type_free(fn_context.inferred_return);

    end_scope(table);
    return ok;
  }

  case STMT_BLOCK: {
    begin_scope(table);
    bool ok = true;
    for (size_t i = 0; i < stmt->data.block.count && ok; i++)
      ok = analyze_statement_with_context(
          semantic, stmt->data.block.statements[i], table, fn_ctx);
    end_scope(table);
    return ok;
  }

  case STMT_BREAK:
  case STMT_CONTINUE:

    return true;

  case STMT_IMPORT: {
    /*
     * import physics;          — register all physics constants
     * import physics::{c, h};  — register only selected constants
     *
     * Each constant is added to the symbol table as:
     *   qty<f64, unit>  for dimensional constants (c, h, k_B, G, e, ...)
     *   f64             for dimensionless constants (pi, alpha, euler_e)
     */
    const char *module_name = stmt->data.import_stmt.module_name;
    if (!module_name)
      return true;

    char **selected = stmt->data.import_stmt.imports;
    size_t sel_count = stmt->data.import_stmt.import_count;

    /* ── User-defined module import ────────────────────────────────────
     * `import mymodule;` or `import mymodule::{foo, bar};`
     * Resolves via the global module registry (file-based discovery).
     */
    /* std:: prefix → Nova builtin standard library, treat as resolved.
     * e.g. use std::io::println;  use std::math::sqrt;
     * The functions are already available as builtins. */
    if (strcmp(module_name, "physics") != 0) {
      nova_module_registry_t *reg = nova_semantic_get_module_registry();

      /* nova_symbol_table_add wrapper for the registry callback */
      typedef void (*add_fn_t)(void *, const char *, const nova_type_t *, bool);
      add_fn_t add_cb = (add_fn_t)(void *)nova_symbol_table_add;

      const char **sym_names = NULL;
      size_t sym_count = 0;
      if (selected && sel_count > 0) {
        sym_names = (const char **)selected;
        sym_count = sel_count;
      }

      bool ok = nova_module_registry_resolve_import(
          reg, table, (void *)add_cb, module_name, sym_names, sym_count);

      if (!ok) {
        /* std:: prefix → fallback to builtins if not found on disk */
        if (strncmp(module_name, "std::", 5) == 0 ||
            strcmp(module_name, "std") == 0) {
          return true;
        }
        semantic_set_error(semantic, "Failed to import module '%s'",
                           module_name);
        return false;
      }
      return true;
    }

    if (strcmp(module_name, "physics") == 0) {
      /* selected and sel_count already defined above */

      for (const nova_physics_constant_t *pc = nova_physics_constants; pc->name;
           pc++) {
        /* If a selective import list exists, only register named constants */
        if (selected && sel_count > 0) {
          int found = 0;
          for (size_t i = 0; i < sel_count; i++) {
            if (selected[i] && strcmp(selected[i], pc->name) == 0) {
              found = 1;
              break;
            }
          }
          if (!found)
            continue;
        }

        /* Build type: qty<f64, unit> or plain f64 for dimensionless */
        nova_type_t *const_type;
        if (pc->unit && pc->unit[0] != '\0') {
          const_type = nova_type_qty(nova_type_f64(), pc->unit);
          const_type->data.qty.dimension = nova_dim_parse(pc->unit);
        } else {
          const_type = nova_type_f64();
        }

        /* Register into current symbol table scope — physics constants are
         * immutable */
        nova_symbol_table_add(table, pc->name, const_type, false);
      }
      return true;
    }

    /* Unknown module — warn but continue (forward compat) */
    fprintf(stderr, "warning: unknown module '%s' — import ignored\n",
            module_name);
    return true;
  }

  case STMT_STRUCT_DECL:
    /* Struct declarations are valid - just skip analysis for now */
    /* TODO: Add struct fields to type system */
    return true;

  default:
    return true;
  }
}

/* Wrapper function for backward compatibility */
static bool analyze_statement(nova_semantic_t *semantic, nova_stmt_t *stmt,
                              nova_symbol_table_t *table) {
  return analyze_statement_with_context(semantic, stmt, table, NULL);
}

// ══════════════════════════════════════════════════════════════════════════════
// PROGRAM ANALYSIS
// ══════════════════════════════════════════════════════════════════════════════

static bool analyze_program(nova_semantic_t *semantic,
                            nova_program_t *program) {
  nova_symbol_table_t *global_table = nova_symbol_table_create();
  if (!global_table)
    return false;

  /* Built-in symbols */
  {
    nova_type_t *vt = nova_type_void();
    nova_symbol_table_add(global_table, "print", vt, false);
    nova_symbol_table_add(global_table, "println", vt, false);
    nova_symbol_table_add(global_table, "assert", vt, false);
    nova_symbol_table_add(global_table, "assert_eq", vt, false);
    nova_symbol_table_add(global_table, "assert_ne", vt, false);
    nova_symbol_table_add(global_table, "assert_approx_eq", vt, false);
    nova_symbol_table_add(global_table, "panic", vt, false);
    nova_symbol_table_add(global_table, "eprintln", vt, false);
    nova_symbol_table_add(global_table, "eprint", vt, false);
    nova_type_free(vt); /* nova_symbol_table_add cloned; free original */
  }

  /* String builtin functions */
  {
    nova_type_t *i64_type = nova_type_i64();
    nova_symbol_table_add(global_table, "String_len", i64_type, false);
    nova_symbol_table_add(global_table, "String_from", i64_type, false);
    nova_symbol_table_add(global_table, "String_from_literal", i64_type, false);
    nova_symbol_table_add(global_table, "String_concat", i64_type, false);
    nova_type_free(i64_type);
  }

  /* Built-in Enum Variants (Result, Option) for bootstrap */
  {
    nova_type_t *rt = nova_type_data("Result");
    nova_symbol_table_add(global_table, "Ok", rt, false);
    nova_symbol_table_add(global_table, "Err", rt, false);
    nova_type_free(rt);

    nova_type_t *ot = nova_type_data("Option");
    nova_symbol_table_add(global_table, "Some", ot, false);
    nova_symbol_table_add(global_table, "None", ot, false);
    nova_type_free(ot);
  }

  /* PASS 1: Register all function and enum names (for forward references and
   * type checking) */
  for (size_t i = 0; i < program->declaration_count; i++) {
    nova_top_level_t *decl = program->declarations[i];
    if (!decl || !((nova_stmt_t *)decl->data))
      continue;

    nova_stmt_t *stmt = (nova_stmt_t *)decl->data;

    /* Register enum types and their variants */
    if (stmt->kind == STMT_ENUM_DECL && stmt->data.enum_decl.name) {
      /* Register the enum type itself */
      nova_type_t *enum_type = malloc(sizeof(nova_type_t));
      enum_type->kind = TYPE_DATA;
      enum_type->data.name = strdup(stmt->data.enum_decl.name);
      nova_symbol_table_add(global_table, stmt->data.enum_decl.name, enum_type,
                            false);
      nova_type_free(enum_type);

      /* Register each variant as a constructor function or value */
      for (size_t j = 0; j < stmt->data.enum_decl.variant_count; j++) {
        char *variant_name = stmt->data.enum_decl.variants[j].name;

        /* For now, treat all variants as having the enum type */
        nova_type_t *variant_type = malloc(sizeof(nova_type_t));
        variant_type->kind = TYPE_DATA;
        variant_type->data.name = strdup(stmt->data.enum_decl.name);
        nova_symbol_table_add(global_table, variant_name, variant_type, false);
        nova_type_free(variant_type);
      }
    } else if (stmt->kind == STMT_FN && stmt->data.fn_stmt.name) {
      /* Build TYPE_FN with parameter types */
      nova_type_t **param_types = NULL;
      if (stmt->data.fn_stmt.param_count > 0) {
        param_types =
            malloc(sizeof(nova_type_t *) * stmt->data.fn_stmt.param_count);
        for (size_t j = 0; j < stmt->data.fn_stmt.param_count; j++) {
          if (stmt->data.fn_stmt.params[j]->type) {
            param_types[j] =
                nova_type_clone(stmt->data.fn_stmt.params[j]->type);
          } else {
            // Leave NULL for type inference from call site
            param_types[j] = NULL;
          }
        }
      }

      nova_type_t *return_type =
          stmt->data.fn_stmt.return_type
              ? nova_type_clone(stmt->data.fn_stmt.return_type)
              : nova_type_i64(); // Default return type

      nova_type_t *fn_type = malloc(sizeof(nova_type_t));
      fn_type->kind = TYPE_FN;
      fn_type->data.fn.params = param_types;
      fn_type->data.fn.param_count = stmt->data.fn_stmt.param_count;
      fn_type->data.fn.return_type = return_type;

      nova_symbol_table_add(global_table, stmt->data.fn_stmt.name, fn_type,
                            false);
      nova_type_free(fn_type);
    }
  }

  bool success = true;

  /* PASS 2: Analyze all statements (will update function types with inferred
   * values) */
  for (size_t i = 0; i < program->declaration_count && success; i++) {
    nova_top_level_t *decl = program->declarations[i];
    if (decl && ((nova_stmt_t *)decl->data)) {
      success = analyze_statement(semantic, ((nova_stmt_t *)decl->data),
                                  global_table);
    }
  }

  if (success) {
    semantic->symbol_table = global_table;
  } else {
    nova_symbol_table_destroy(global_table);
    semantic->symbol_table = NULL;
  }

  return success;
}

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ══════════════════════════════════════════════════════════════════════════════

nova_semantic_t *nova_semantic_create(nova_program_t *ast) {
  nova_semantic_t *semantic = malloc(sizeof(nova_semantic_t));
  if (!semantic)
    return NULL;
  semantic->ast = ast;
  semantic->symbol_table = NULL;
  semantic->error_message = NULL;
  return semantic;
}

bool nova_semantic_analyze(nova_semantic_t *semantic) {
  return analyze_program(semantic, semantic->ast);
}

const char *nova_semantic_get_error(nova_semantic_t *semantic) {
  return semantic->error_message ? semantic->error_message : "No error";
}

void nova_semantic_destroy(nova_semantic_t *semantic) {
  if (!semantic)
    return;
  free(semantic->error_message);
  if (semantic->symbol_table)
    nova_symbol_table_destroy(semantic->symbol_table);
  free(semantic);
}

nova_symbol_t *nova_semantic_lookup_variable(nova_semantic_t *semantic,
                                             const char *name) {
  if (!semantic || !semantic->symbol_table)
    return NULL;
  return nova_symbol_table_lookup(semantic->symbol_table, name);
}
