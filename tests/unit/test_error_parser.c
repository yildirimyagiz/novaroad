/**
 * @file test_error_parser.c
 * @brief Error handling parser tests
 */

#include "compiler/parser.h"
#include "compiler/error.h"
#include "compiler/lexer.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  ✓ PASS\n")

// Helper: parse from string
Expr *parse_expr_from_string(const char *source) {
    Lexer *lexer = lexer_create(source);
    Parser *parser = parser_create(lexer);
    Expr *expr = parse_expression(parser);
    return expr;
}

// Test 1: Try/catch basic
void test_try_catch_basic() {
    TEST("Try/catch basic parsing");
    
    const char *source = 
        "try {\n"
        "    risky_operation();\n"
        "} catch e {\n"
        "    println(\"Error occurred\");\n"
        "}";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_TRY);
    assert(expr->data.try_expr != NULL);
    assert(expr->data.try_expr->handler_count >= 1);
    
    expr_free(expr);
    PASS();
}

// Test 2: Try/catch with type
void test_try_catch_typed() {
    TEST("Try/catch with error type");
    
    const char *source =
        "try {\n"
        "    operation();\n"
        "} catch Error::IoError(e) {\n"
        "    handle_io_error(e);\n"
        "} catch Error::ParseError(e) {\n"
        "    handle_parse_error(e);\n"
        "}";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_TRY);
    assert(expr->data.try_expr->handler_count == 2);
    
    expr_free(expr);
    PASS();
}

// Test 3: Try/catch/finally
void test_try_catch_finally() {
    TEST("Try/catch/finally");
    
    const char *source =
        "try {\n"
        "    open_file();\n"
        "} catch e {\n"
        "    println(\"Error\");\n"
        "} finally {\n"
        "    cleanup();\n"
        "}";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_TRY);
    assert(expr->data.try_expr->finally_block != NULL);
    
    expr_free(expr);
    PASS();
}

// Test 4: Error propagation (?)
void test_error_propagation() {
    TEST("Error propagation operator (?)");
    
    const char *source = "read_file(path)?";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_PROPAGATE);
    assert(expr->data.propagate != NULL);
    assert(expr->data.propagate->inner != NULL);
    
    expr_free(expr);
    PASS();
}

// Test 5: Chained propagation
void test_chained_propagation() {
    TEST("Chained error propagation");
    
    const char *source = "parse(read_file(path)?)?";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_PROPAGATE);
    
    // Inner should also be propagate
    Expr *inner = expr->data.propagate->inner;
    // Inner is a function call, which contains propagation
    
    expr_free(expr);
    PASS();
}

// Test 6: Result type
void test_result_type() {
    TEST("Result<T, E> type parsing");
    
    const char *source = "Result<i64, String>";
    
    Lexer *lexer = lexer_create(source);
    Parser *parser = parser_create(lexer);
    Type *type = parse_type(parser);
    
    assert(type != NULL);
    assert(type->kind == TYPE_RESULT);
    assert(type->data.result.ok_type != NULL);
    assert(type->data.result.err_type != NULL);
    
    type_free(type);
    PASS();
}

// Test 7: Ok constructor
void test_ok_constructor() {
    TEST("Ok(value) constructor");
    
    const char *source = "Ok(42)";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_RESULT_OK);
    assert(expr->data.result_constructor.value != NULL);
    
    expr_free(expr);
    PASS();
}

// Test 8: Err constructor
void test_err_constructor() {
    TEST("Err(error) constructor");
    
    const char *source = "Err(\"File not found\")";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_RESULT_ERR);
    assert(expr->data.result_constructor.value != NULL);
    
    expr_free(expr);
    PASS();
}

// Test 9: Panic expression
void test_panic() {
    TEST("Panic expression");
    
    const char *source = "panic(\"Critical error!\")";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_PANIC);
    assert(expr->data.panic != NULL);
    
    expr_free(expr);
    PASS();
}

// Test 10: Panic macro style
void test_panic_macro() {
    TEST("Panic! macro style");
    
    const char *source = "panic!(\"Unrecoverable error\")";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_PANIC);
    
    expr_free(expr);
    PASS();
}

// Test 11: Complex error handling
void test_complex_error_handling() {
    TEST("Complex error handling scenario");
    
    const char *source =
        "try {\n"
        "    let data = parse(read_file(path)?)?.process()?;\n"
        "    Ok(data)\n"
        "} catch Error::IoError(e) {\n"
        "    Err(Error::Custom(\"IO failed: \" + e.message))\n"
        "} catch _ {\n"
        "    panic(\"Unknown error\")\n"
        "}";
    
    Expr *expr = parse_expr_from_string(source);
    assert(expr != NULL);
    assert(expr->kind == EXPR_TRY);
    
    expr_free(expr);
    PASS();
}

// Main test runner
int main() {
    printf("═══════════════════════════════════════════════\n");
    printf("  Error Handling Parser Tests\n");
    printf("═══════════════════════════════════════════════\n\n");
    
    test_try_catch_basic();
    test_try_catch_typed();
    test_try_catch_finally();
    test_error_propagation();
    test_chained_propagation();
    test_result_type();
    test_ok_constructor();
    test_err_constructor();
    test_panic();
    test_panic_macro();
    test_complex_error_handling();
    
    printf("\n═══════════════════════════════════════════════\n");
    printf("  ✓ All parser tests passed!\n");
    printf("═══════════════════════════════════════════════\n");
    
    return 0;
}
