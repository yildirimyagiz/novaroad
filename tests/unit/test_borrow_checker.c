#include "compiler/borrow_checker.h"
#include "compiler/ast.h"
#include <stdio.h>
#include <stdlib.h>

// Test borrow checker D.1
int main() {
    printf("🧪 Testing Nova Borrow Checker (D.1)\n");
    printf("=====================================\n\n");

    // Create diagnostics
    nova_diag_collector_t* diag = nova_diag_collector_create();

    // Create borrow checker
    nova_borrow_checker_t* bc = nova_borrow_checker_create(diag);

    // Test basic borrow registration
    nova_span_t span = {0, 0, 10, 1, 5};

    // Register a shared borrow
    printf("1. Testing shared borrow registration...\n");
    bool ok = nova_borrow_register(bc, BORROW_SHARED, "x", span);
    printf("   Shared borrow 'x': %s\n", ok ? "✓" : "✗");

    // Try to register mutable borrow on same variable (should fail)
    printf("2. Testing mutable borrow conflict...\n");
    ok = nova_borrow_register(bc, BORROW_MUT, "x", span);
    printf("   Mutable borrow 'x' (should fail): %s\n", ok ? "✗ UNEXPECTED" : "✓");

    // Register mutable borrow on different variable
    printf("3. Testing mutable borrow on different var...\n");
    ok = nova_borrow_register(bc, BORROW_MUT, "y", span);
    printf("   Mutable borrow 'y': %s\n", ok ? "✓" : "✗");

    // Try to register any borrow on mutable variable (should fail)
    printf("4. Testing shared borrow on mutable var...\n");
    ok = nova_borrow_register(bc, BORROW_SHARED, "y", span);
    printf("   Shared borrow 'y' (should fail): %s\n", ok ? "✗ UNEXPECTED" : "✓");

    // Test scope management
    printf("5. Testing scope management...\n");
    nova_borrow_enter_scope(bc);
    ok = nova_borrow_register(bc, BORROW_SHARED, "z", span);
    printf("   Register 'z' in scope 1: %s\n", ok ? "✓" : "✗");

    nova_borrow_exit_scope(bc);
    nova_borrow_debug_print(bc);

    // Test type analysis
    printf("6. Testing type analysis...\n");
    nova_type_t* t1 = nova_type_ptr(nova_type_i32());  // *i32
    nova_type_t* t2 = nova_type_ptr_mut(nova_type_i32());  // *mut i32
    nova_type_t* t3 = nova_type_i32();  // i32

    nova_borrow_kind_t k1 = nova_type_to_borrow_kind(t1);
    nova_borrow_kind_t k2 = nova_type_to_borrow_kind(t2);
    nova_borrow_kind_t k3 = nova_type_to_borrow_kind(t3);

    printf("   *i32 -> %s\n", k1 == BORROW_SHARED ? "SHARED" : "OTHER");
    printf("   *mut i32 -> %s\n", k2 == BORROW_MUT ? "MUTABLE" : "OTHER");
    printf("   i32 -> %s\n", k3 == BORROW_OWNED ? "OWNED" : "OTHER");

    // Check diagnostics
    printf("7. Diagnostics summary:\n");
    printf("   Total diagnostics: %zu\n", diag->count);
    nova_diag_print(diag, NULL);

    // Cleanup
    nova_borrow_checker_destroy(bc);
    nova_diag_collector_destroy(diag);

    printf("\n✅ Borrow checker test completed!\n");

    return 0;
}
