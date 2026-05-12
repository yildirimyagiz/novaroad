/**
 * @file test_error_types.c
 * @brief Error handling type tests
 */

#include "compiler/error.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  ✓ PASS\n")

// Test 1: Result type creation
void test_result_creation() {
    TEST("Result type creation");
    
    // Create Result<i64, String>
    int ok_value = 42;
    ResultValue *result = result_ok(&ok_value, TYPE_I64, TYPE_STRING);
    
    assert(result != NULL);
    assert(result->variant == RESULT_OK);
    assert(*(int*)result->data.ok_value == 42);
    
    result_free(result);
    PASS();
}

// Test 2: Error creation
void test_error_creation() {
    TEST("Error creation");
    
    Error *err = error_new(ERROR_IO, "File not found");
    
    assert(err != NULL);
    assert(err->category == ERROR_IO);
    assert(strcmp(err->message, "File not found") == 0);
    
    error_free(err);
    PASS();
}

// Test 3: Error with location
void test_error_with_location() {
    TEST("Error with location");
    
    Error *err = error_with_location(ERROR_PARSE, 
                                     "Unexpected token",
                                     10, 5, "test.nova");
    
    assert(err->line == 10);
    assert(err->column == 5);
    assert(strcmp(err->file, "test.nova") == 0);
    
    error_free(err);
    PASS();
}

// Test 4: Error chaining
void test_error_chaining() {
    TEST("Error chaining");
    
    Error *cause = error_new(ERROR_IO, "Connection refused");
    Error *err = error_with_cause(ERROR_RUNTIME, 
                                  "Failed to load data",
                                  cause);
    
    assert(err->cause != NULL);
    assert(err->cause == cause);
    assert(strcmp(err->cause->message, "Connection refused") == 0);
    
    error_free(err);
    PASS();
}

// Test 5: Try expression
void test_try_expression() {
    TEST("Try expression");
    
    Stmt *try_block = /* create stmt */;
    TryExpr *try_expr = try_expr_new(try_block);
    
    assert(try_expr != NULL);
    assert(try_expr->try_block == try_block);
    assert(try_expr->handler_count == 0);
    
    try_expr_free(try_expr);
    PASS();
}

// Test 6: Catch handler
void test_catch_handler() {
    TEST("Catch handler");
    
    CatchHandler *handler = catch_handler_new("e", TYPE_ERROR, /* stmt */);
    
    assert(handler != NULL);
    assert(strcmp(handler->error_var, "e") == 0);
    assert(handler->error_type == TYPE_ERROR);
    
    catch_handler_free(handler);
    PASS();
}

// Test 7: Propagate expression
void test_propagate_expression() {
    TEST("Propagate expression");
    
    Expr *inner = /* create expr */;
    PropagateExpr *prop = propagate_expr_new(inner);
    
    assert(prop != NULL);
    assert(prop->inner == inner);
    
    propagate_expr_free(prop);
    PASS();
}

// Test 8: Panic expression
void test_panic_expression() {
    TEST("Panic expression");
    
    PanicExpr *panic = panic_expr_new("Critical error!");
    
    assert(panic != NULL);
    assert(strcmp(panic->message, "Critical error!") == 0);
    assert(panic->message_expr == NULL);
    
    panic_expr_free(panic);
    PASS();
}

// Test 9: Error formatting
void test_error_formatting() {
    TEST("Error formatting");
    
    Error *err = error_with_location(ERROR_PARSE,
                                     "Syntax error",
                                     5, 10, "main.nova");
    
    char *formatted = error_format(err);
    assert(formatted != NULL);
    assert(strstr(formatted, "Syntax error") != NULL);
    assert(strstr(formatted, "main.nova") != NULL);
    
    free(formatted);
    error_free(err);
    PASS();
}

// Test 10: Error printing
void test_error_printing() {
    TEST("Error printing");
    
    Error *cause = error_new(ERROR_IO, "Disk full");
    Error *err = error_with_cause(ERROR_RUNTIME, "Write failed", cause);
    
    printf("  Expected output:\n  ");
    error_print(err);
    
    error_free(err);
    PASS();
}

// Main test runner
int main() {
    printf("═══════════════════════════════════════════════\n");
    printf("  Error Handling Type Tests\n");
    printf("═══════════════════════════════════════════════\n\n");
    
    test_result_creation();
    test_error_creation();
    test_error_with_location();
    test_error_chaining();
    test_try_expression();
    test_catch_handler();
    test_propagate_expression();
    test_panic_expression();
    test_error_formatting();
    test_error_printing();
    
    printf("\n═══════════════════════════════════════════════\n");
    printf("  ✓ All tests passed!\n");
    printf("═══════════════════════════════════════════════\n");
    
    return 0;
}
