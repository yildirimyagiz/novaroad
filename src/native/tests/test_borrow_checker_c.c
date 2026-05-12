/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    BORROW CHECKER C IMPLEMENTATION - TESTS ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#include "../src/nova_borrow_checker.h"
#include <assert.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════════
// TEST HELPERS
// ═══════════════════════════════════════════════════════════════════════════════

#define TEST_START(name)                                                       \
  printf("\n🧪 Test: %s\n", name);                                             \
  printf("─────────────────────────────────────────────────────────\n");

#define TEST_END() printf("✅ PASS\n");

// ═══════════════════════════════════════════════════════════════════════════════
// TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_basic_borrow() {
  TEST_START("Basic Immutable Borrow");

  NovaBorrowChecker *checker = nova_borrow_checker_new();

  nova_borrow_checker_declare_variable(checker, "x");

  // Multiple immutable borrows - OK
  assert(nova_borrow_checker_borrow_immutable(checker, "x", 10));
  assert(nova_borrow_checker_borrow_immutable(checker, "x", 11));

  nova_borrow_checker_print_errors(checker);
  assert(!nova_borrow_checker_has_errors(checker));

  nova_borrow_checker_free(checker);
  TEST_END();
}

void test_mutable_borrow_conflict() {
  TEST_START("Mutable Borrow Conflicts");

  NovaBorrowChecker *checker = nova_borrow_checker_new();

  nova_borrow_checker_declare_variable(checker, "x");

  // Mutable borrow - OK
  assert(nova_borrow_checker_borrow_mutable(checker, "x", 20));

  // Try another immutable while mutable exists - ERROR
  assert(!nova_borrow_checker_borrow_immutable(checker, "x", 21));

  // Try another mutable - ERROR
  assert(!nova_borrow_checker_borrow_mutable(checker, "x", 22));

  nova_borrow_checker_print_errors(checker);
  assert(nova_borrow_checker_has_errors(checker));

  nova_borrow_checker_free(checker);
  TEST_END();
}

void test_use_after_move() {
  TEST_START("Use After Move");

  NovaBorrowChecker *checker = nova_borrow_checker_new();

  nova_borrow_checker_declare_variable(checker, "s");

  // Move value
  assert(nova_borrow_checker_move_value(checker, "s", 30));

  // Try to use after move - ERROR
  assert(!nova_borrow_checker_check_use(checker, "s", 31));

  // Try to borrow after move - ERROR
  assert(!nova_borrow_checker_borrow_immutable(checker, "s", 32));

  // Try to move again - ERROR
  assert(!nova_borrow_checker_move_value(checker, "s", 33));

  nova_borrow_checker_print_errors(checker);
  assert(nova_borrow_checker_has_errors(checker));

  nova_borrow_checker_free(checker);
  TEST_END();
}

void test_scope_cleanup() {
  TEST_START("Scope-Based Borrow Cleanup");

  NovaBorrowChecker *checker = nova_borrow_checker_new();

  nova_borrow_checker_declare_variable(checker, "x");

  // Outer scope immutable borrow
  assert(nova_borrow_checker_borrow_immutable(checker, "x", 40));

  // Enter inner scope
  nova_borrow_checker_enter_scope(checker);
  assert(nova_borrow_checker_borrow_immutable(checker, "x", 41));

  // Exit inner scope (inner borrow cleaned up)
  nova_borrow_checker_exit_scope(checker);

  // Outer borrow still active, so can't borrow mutably
  assert(!nova_borrow_checker_borrow_mutable(checker, "x", 42));

  nova_borrow_checker_print_errors(checker);

  nova_borrow_checker_free(checker);
  TEST_END();
}

void test_move_while_borrowed() {
  TEST_START("Cannot Move While Borrowed");

  NovaBorrowChecker *checker = nova_borrow_checker_new();

  nova_borrow_checker_declare_variable(checker, "v");

  // Borrow
  assert(nova_borrow_checker_borrow_immutable(checker, "v", 50));

  // Try to move while borrowed - ERROR
  assert(!nova_borrow_checker_move_value(checker, "v", 51));

  nova_borrow_checker_print_errors(checker);
  assert(nova_borrow_checker_has_errors(checker));

  nova_borrow_checker_free(checker);
  TEST_END();
}

void test_enhanced_error_messages() {
  TEST_START("Enhanced Error Messages");

  NovaBorrowChecker *checker = nova_borrow_checker_new();

  nova_borrow_checker_declare_variable(checker, "data");

  // Create immutable borrow
  nova_borrow_checker_borrow_immutable(checker, "data", 60);

  // Try mutable borrow - should get enhanced error
  nova_borrow_checker_borrow_mutable(checker, "data", 65);

  printf("\nError messages (should include line numbers, notes, and help):\n");
  nova_borrow_checker_print_errors(checker);

  nova_borrow_checker_free(checker);
  TEST_END();
}

void test_disabled_checker() {
  TEST_START("Disabled Checker (unsafe mode)");

  NovaBorrowChecker *checker = nova_borrow_checker_new();
  nova_borrow_checker_set_enabled(checker, false);

  nova_borrow_checker_declare_variable(checker, "x");

  // Everything should succeed when disabled
  assert(nova_borrow_checker_borrow_mutable(checker, "x", 70));
  assert(nova_borrow_checker_borrow_immutable(checker, "x",
                                              71)); // Would normally fail
  assert(nova_borrow_checker_move_value(checker, "x", 72));
  assert(
      nova_borrow_checker_check_use(checker, "x", 73)); // Would normally fail

  assert(!nova_borrow_checker_has_errors(checker));

  nova_borrow_checker_free(checker);
  TEST_END();
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════════

int main() {
  printf("═══════════════════════════════════════════════════════════\n");
  printf("   NOVA BORROW CHECKER (C) - TEST SUITE\n");
  printf("═══════════════════════════════════════════════════════════\n");

  test_basic_borrow();
  test_mutable_borrow_conflict();
  test_use_after_move();
  test_scope_cleanup();
  test_move_while_borrowed();
  test_enhanced_error_messages();
  test_disabled_checker();

  printf("\n═══════════════════════════════════════════════════════════\n");
  printf("✅ All tests completed!\n");
  printf("═══════════════════════════════════════════════════════════\n");

  yield 0;
}
