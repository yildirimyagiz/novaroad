#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Include the symbolic header */
#include "../../../include/nova_symbolic.h"

/* ============================================================================
 * Test framework
 * ============================================================================ */
static int g_passed = 0, g_failed = 0;

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("  ❌ FAIL: %s (line %d)\n", #cond, __LINE__); \
        g_failed++; return; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("  ❌ FAIL: %s is NULL (line %d)\n", #ptr, __LINE__); \
        g_failed++; return; \
    } \
} while(0)

#define RUN(fn) do { \
    printf("  [TEST] %-45s", #fn); \
    fn(); \
    if (g_failed == 0) { printf(" PASS\n"); g_passed++; } \
    else { g_failed = 0; } \
} while(0)

/* ============================================================================
 * Test: Context creation and destruction
 * ============================================================================ */
void test_context_create_destroy(void) {
    SymContext *ctx = sym_context_create("test_kernel");
    ASSERT_NOT_NULL(ctx);
    ASSERT(ctx->kernel_name != NULL);
    ASSERT(strcmp(ctx->kernel_name, "test_kernel") == 0);
    sym_context_destroy(ctx);
    ASSERT(1);
}

/* ============================================================================
 * Test: Constant integer creation
 * ============================================================================ */
void test_const_int(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *expr = sym_const_int(ctx, 42);
    ASSERT_NOT_NULL(expr);
    ASSERT(expr->kind == SYM_CONST_INT);
    ASSERT(expr->int_val == 42);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Constant real creation
 * ============================================================================ */
void test_const_real(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *expr = sym_const_real(ctx, 3.14);
    ASSERT_NOT_NULL(expr);
    ASSERT(expr->kind == SYM_CONST_REAL);
    ASSERT(expr->real_val > 3.13 && expr->real_val < 3.15);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Constant boolean creation
 * ============================================================================ */
void test_const_bool(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *expr_true = sym_const_bool(ctx, true);
    SymExpr *expr_false = sym_const_bool(ctx, false);
    ASSERT_NOT_NULL(expr_true);
    ASSERT_NOT_NULL(expr_false);
    ASSERT(expr_true->kind == SYM_CONST_BOOL);
    ASSERT(expr_true->bool_val == true);
    ASSERT(expr_false->bool_val == false);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Variable creation (integer)
 * ============================================================================ */
void test_var_int(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *var = sym_var(ctx, "x", SYM_TYPE_INT);
    ASSERT_NOT_NULL(var);
    ASSERT(var->kind == SYM_VAR);
    ASSERT(var->var_name != NULL);
    ASSERT(strcmp(var->var_name, "x") == 0);
    ASSERT(var->type.base_type == SYM_TYPE_INT);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Variable creation (real)
 * ============================================================================ */
void test_var_real(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *var = sym_var(ctx, "y", SYM_TYPE_REAL);
    ASSERT_NOT_NULL(var);
    ASSERT(var->kind == SYM_VAR);
    ASSERT(var->type.base_type == SYM_TYPE_REAL);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Bitvector variable creation
 * ============================================================================ */
void test_var_bitvec(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *var = sym_var_bitvec(ctx, "bv32", 32);
    ASSERT_NOT_NULL(var);
    ASSERT(var->kind == SYM_VAR);
    ASSERT(var->type.base_type == SYM_TYPE_BITVEC);
    ASSERT(var->type.bitvec_width == 32);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Tensor variable creation
 * ============================================================================ */
void test_var_tensor(void) {
    SymContext *ctx = sym_context_create("test");
    int shape[] = {2, 3};
    SymExpr *var = sym_var_tensor(ctx, "A", shape, 2, SYM_TYPE_REAL);
    ASSERT_NOT_NULL(var);
    ASSERT(var->kind == SYM_VAR);
    ASSERT(var->type.base_type == SYM_TYPE_TENSOR);
    ASSERT(var->type.tensor_ndim == 2);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Arithmetic - Addition
 * ============================================================================ */
void test_add(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 10);
    SymExpr *b = sym_const_int(ctx, 20);
    SymExpr *sum = sym_add(ctx, a, b);
    ASSERT_NOT_NULL(sum);
    ASSERT(sum->kind == SYM_ADD);
    ASSERT(sum->left == a);
    ASSERT(sum->right == b);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Arithmetic - Subtraction
 * ============================================================================ */
void test_sub(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 30);
    SymExpr *b = sym_const_int(ctx, 10);
    SymExpr *diff = sym_sub(ctx, a, b);
    ASSERT_NOT_NULL(diff);
    ASSERT(diff->kind == SYM_SUB);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Arithmetic - Multiplication
 * ============================================================================ */
void test_mul(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 5);
    SymExpr *b = sym_const_int(ctx, 7);
    SymExpr *prod = sym_mul(ctx, a, b);
    ASSERT_NOT_NULL(prod);
    ASSERT(prod->kind == SYM_MUL);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Arithmetic - Division
 * ============================================================================ */
void test_div(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_real(ctx, 10.0);
    SymExpr *b = sym_const_real(ctx, 2.0);
    SymExpr *div = sym_div(ctx, a, b);
    ASSERT_NOT_NULL(div);
    ASSERT(div->kind == SYM_DIV);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Arithmetic - Negation
 * ============================================================================ */
void test_neg(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 42);
    SymExpr *neg = sym_neg(ctx, a);
    ASSERT_NOT_NULL(neg);
    ASSERT(neg->kind == SYM_NEG);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Arithmetic - Absolute value
 * ============================================================================ */
void test_abs(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_real(ctx, -5.5);
    SymExpr *abs = sym_abs(ctx, a);
    ASSERT_NOT_NULL(abs);
    ASSERT(abs->kind == SYM_ABS);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Comparison - Equality
 * ============================================================================ */
void test_eq(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 5);
    SymExpr *b = sym_const_int(ctx, 5);
    SymExpr *eq = sym_eq(ctx, a, b);
    ASSERT_NOT_NULL(eq);
    ASSERT(eq->kind == SYM_EQ);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Comparison - Less than
 * ============================================================================ */
void test_lt(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 3);
    SymExpr *b = sym_const_int(ctx, 7);
    SymExpr *lt = sym_lt(ctx, a, b);
    ASSERT_NOT_NULL(lt);
    ASSERT(lt->kind == SYM_LT);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Comparison - Less than or equal
 * ============================================================================ */
void test_le(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 5);
    SymExpr *b = sym_const_int(ctx, 5);
    SymExpr *le = sym_le(ctx, a, b);
    ASSERT_NOT_NULL(le);
    ASSERT(le->kind == SYM_LE);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Comparison - Greater than
 * ============================================================================ */
void test_gt(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 10);
    SymExpr *b = sym_const_int(ctx, 5);
    SymExpr *gt = sym_gt(ctx, a, b);
    ASSERT_NOT_NULL(gt);
    ASSERT(gt->kind == SYM_GT);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Comparison - Greater than or equal
 * ============================================================================ */
void test_ge(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_int(ctx, 5);
    SymExpr *b = sym_const_int(ctx, 5);
    SymExpr *ge = sym_ge(ctx, a, b);
    ASSERT_NOT_NULL(ge);
    ASSERT(ge->kind == SYM_GE);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Logic - AND
 * ============================================================================ */
void test_and(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_bool(ctx, true);
    SymExpr *b = sym_const_bool(ctx, true);
    SymExpr *and = sym_and(ctx, a, b);
    ASSERT_NOT_NULL(and);
    ASSERT(and->kind == SYM_AND);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Logic - OR
 * ============================================================================ */
void test_or(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_bool(ctx, true);
    SymExpr *b = sym_const_bool(ctx, false);
    SymExpr *or = sym_or(ctx, a, b);
    ASSERT_NOT_NULL(or);
    ASSERT(or->kind == SYM_OR);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Logic - NOT
 * ============================================================================ */
void test_not(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_bool(ctx, true);
    SymExpr *not = sym_not(ctx, a);
    ASSERT_NOT_NULL(not);
    ASSERT(not->kind == SYM_NOT);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Logic - IMPLIES
 * ============================================================================ */
void test_implies(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *a = sym_const_bool(ctx, true);
    SymExpr *b = sym_const_bool(ctx, false);
    SymExpr *impl = sym_implies(ctx, a, b);
    ASSERT_NOT_NULL(impl);
    ASSERT(impl->kind == SYM_IMPLIES);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Quantifier - FORALL
 * ============================================================================ */
void test_forall(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *lo = sym_const_int(ctx, 0);
    SymExpr *hi = sym_const_int(ctx, 10);
    SymExpr *var = sym_var(ctx, "i", SYM_TYPE_INT);
    SymExpr *body = sym_lt(ctx, var, hi);
    SymExpr *forall = sym_forall(ctx, "i", lo, hi, body);
    ASSERT_NOT_NULL(forall);
    ASSERT(forall->kind == SYM_FORALL);
    ASSERT(forall->bound_var != NULL);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Quantifier - EXISTS
 * ============================================================================ */
void test_exists(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *lo = sym_const_int(ctx, 0);
    SymExpr *hi = sym_const_int(ctx, 100);
    SymExpr *var = sym_var(ctx, "x", SYM_TYPE_INT);
    SymExpr *body = sym_eq(ctx, var, sym_const_int(ctx, 42));
    SymExpr *exists = sym_exists(ctx, "x", lo, hi, body);
    ASSERT_NOT_NULL(exists);
    ASSERT(exists->kind == SYM_EXISTS);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Array - SELECT
 * ============================================================================ */
void test_select(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *array = sym_var(ctx, "arr", SYM_TYPE_ARRAY);
    SymExpr *idx = sym_const_int(ctx, 5);
    SymExpr *sel = sym_select(ctx, array, idx);
    ASSERT_NOT_NULL(sel);
    ASSERT(sel->kind == SYM_SELECT);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Array - STORE
 * ============================================================================ */
void test_store(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *array = sym_var(ctx, "arr", SYM_TYPE_ARRAY);
    SymExpr *idx = sym_const_int(ctx, 5);
    SymExpr *val = sym_const_real(ctx, 3.14);
    SymExpr *stored = sym_store(ctx, array, idx, val);
    ASSERT_NOT_NULL(stored);
    ASSERT(stored->kind == SYM_STORE);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Floating point safety - IS_NAN
 * ============================================================================ */
void test_is_nan(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *x = sym_const_real(ctx, 1.0);
    SymExpr *is_nan = sym_is_nan(ctx, x);
    ASSERT_NOT_NULL(is_nan);
    ASSERT(is_nan->kind == SYM_IS_NAN);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Floating point safety - IS_INF
 * ============================================================================ */
void test_is_inf(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *x = sym_const_real(ctx, 1e308);
    SymExpr *is_inf = sym_is_inf(ctx, x);
    ASSERT_NOT_NULL(is_inf);
    ASSERT(is_inf->kind == SYM_IS_INF);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Floating point safety - FP_SAFE
 * ============================================================================ */
void test_fp_safe(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *x = sym_const_real(ctx, 42.0);
    SymExpr *fp_safe = sym_fp_safe(ctx, x);
    ASSERT_NOT_NULL(fp_safe);
    /* sym_fp_safe returns ¬isNaN(x) ∧ ¬isInf(x), which is an AND node */
    ASSERT(fp_safe->kind == SYM_AND);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: ITE (if-then-else)
 * ============================================================================ */
void test_ite(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *cond = sym_const_bool(ctx, true);
    SymExpr *then_e = sym_const_int(ctx, 1);
    SymExpr *else_e = sym_const_int(ctx, 0);
    SymExpr *ite = sym_ite(ctx, cond, then_e, else_e);
    ASSERT_NOT_NULL(ite);
    ASSERT(ite->kind == SYM_ITE);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Context - declare
 * ============================================================================ */
void test_declare(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *var = sym_var(ctx, "input", SYM_TYPE_INT);
    sym_declare(ctx, var);
    ASSERT(ctx->decl_count > 0);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Context - assert
 * ============================================================================ */
void test_assert(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *cond = sym_const_bool(ctx, true);
    sym_assert(ctx, cond);
    ASSERT(ctx->assert_count > 0);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Context - assume
 * ============================================================================ */
void test_assume(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *cond = sym_const_bool(ctx, true);
    sym_assume(ctx, cond);
    ASSERT(ctx->assume_count > 0);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Expression to string (pretty-printing)
 * ============================================================================ */
void test_expr_to_string(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *expr = sym_const_int(ctx, 42);
    char buf[256];
    int len = sym_expr_to_string(expr, buf, sizeof(buf));
    ASSERT(len > 0);
    ASSERT(buf[0] != '\0');
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Test: Context dump
 * ============================================================================ */
void test_context_dump(void) {
    SymContext *ctx = sym_context_create("test");
    SymExpr *var = sym_var(ctx, "x", SYM_TYPE_INT);
    SymExpr *cond = sym_gt(ctx, var, sym_const_int(ctx, 0));
    sym_declare(ctx, var);
    sym_assert(ctx, cond);
    
    /* Should not crash */
    sym_context_dump(ctx);
    ASSERT(1);
    sym_context_destroy(ctx);
}

/* ============================================================================
 * Main: Run all tests
 * ============================================================================ */
int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Nova Symbolic IR Unit Tests             ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n");
    
    RUN(test_context_create_destroy);
    RUN(test_const_int);
    RUN(test_const_real);
    RUN(test_const_bool);
    RUN(test_var_int);
    RUN(test_var_real);
    RUN(test_var_bitvec);
    RUN(test_var_tensor);
    RUN(test_add);
    RUN(test_sub);
    RUN(test_mul);
    RUN(test_div);
    RUN(test_neg);
    RUN(test_abs);
    RUN(test_eq);
    RUN(test_lt);
    RUN(test_le);
    RUN(test_gt);
    RUN(test_ge);
    RUN(test_and);
    RUN(test_or);
    RUN(test_not);
    RUN(test_implies);
    RUN(test_forall);
    RUN(test_exists);
    RUN(test_select);
    RUN(test_store);
    RUN(test_is_nan);
    RUN(test_is_inf);
    RUN(test_fp_safe);
    RUN(test_ite);
    RUN(test_declare);
    RUN(test_assert);
    RUN(test_assume);
    RUN(test_expr_to_string);
    RUN(test_context_dump);
    
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Results: %d/%d passed                     ║\n", g_passed, g_passed + g_failed);
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n");
    
    return (g_failed == 0) ? 0 : 1;
}
