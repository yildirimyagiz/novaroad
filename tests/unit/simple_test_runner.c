#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

// Simple Nova Test Framework - Phase D.5
typedef enum {
    TEST_PASS,
    TEST_FAIL
} test_result_t;

typedef test_result_t (*test_func_t)(void);

// Test execution with panic recovery
static jmp_buf test_jmp_buf;
static char* current_failure = NULL;

#define assert(expr) do { \
    if (!(expr)) { \
        current_failure = "Assertion failed: " #expr; \
        longjmp(test_jmp_buf, 1); \
    } \
} while(0)

#define assert_eq(a, b) do { \
    if ((a) != (b)) { \
        char msg[256]; \
        sprintf(msg, "assert_eq failed: %d != %d", (int)(a), (int)(b)); \
        current_failure = msg; \
        longjmp(test_jmp_buf, 1); \
    } \
} while(0)

test_result_t run_test(const char* name, test_func_t func) {
    printf("  Running %s... ", name);
    fflush(stdout);

    current_failure = NULL;

    if (setjmp(test_jmp_buf) == 0) {
        test_result_t result = func();
        if (result == TEST_PASS) {
            printf("✓ PASS\n");
            return TEST_PASS;
        } else {
            printf("✗ FAIL\n");
            return TEST_FAIL;
        }
    } else {
        printf("✗ FAIL\n");
        if (current_failure) {
            printf("    %s\n", current_failure);
        }
        return TEST_FAIL;
    }
}

// Sample tests
test_result_t test_hashmap_basic() {
    int value = 42;
    assert_eq(value, 42);
    assert(value > 0);
    return TEST_PASS;
}

test_result_t test_borrow_checker() {
    int x = 5;
    int* y = &x;
    assert_eq(*y, 5);
    return TEST_PASS;
}

test_result_t test_error_propagation() {
    assert(1 + 1 == 2);
    return TEST_PASS;
}

test_result_t test_deliberate_failure() {
    assert_eq(1, 2); // This will fail
    return TEST_PASS;
}

int main() {
    printf("🚀 Nova Test Runner (Phase D.5)\n");
    printf("================================\n\n");

    int total = 0;
    int passed = 0;
    int failed = 0;

    // Run tests
    total++;
    if (run_test("hashmap_basic", test_hashmap_basic) == TEST_PASS) passed++;
    else failed++;

    total++;
    if (run_test("borrow_checker", test_borrow_checker) == TEST_PASS) passed++;
    else failed++;

    total++;
    if (run_test("error_propagation", test_error_propagation) == TEST_PASS) passed++;
    else failed++;

    total++;
    if (run_test("deliberate_failure", test_deliberate_failure) == TEST_PASS) passed++;
    else failed++;

    // Summary
    printf("\n📊 Test Results:\n");
    printf("  Total: %d\n", total);
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);

    if (failed > 0) {
        printf("\n❌ %d test(s) failed\n", failed);
        return 1; // Non-zero exit for CI/CD
    } else {
        printf("\n✅ All tests passed!\n");
        return 0;
    }
}
