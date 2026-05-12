/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA BORROW CHECKER (Header)                             ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_BORROW_CHECKER_H
#define NOVA_BORROW_CHECKER_H

#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════════
// TYPES
// ═══════════════════════════════════════════════════════════════════════════════

typedef enum {
    BORROW_IMMUTABLE,  // &T
    BORROW_MUTABLE     // &mut T
} BorrowKind;

typedef struct NovaBorrowChecker NovaBorrowChecker;

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Create a new borrow checker
 */
NovaBorrowChecker *nova_borrow_checker_new(void);

/**
 * Free the borrow checker
 */
void nova_borrow_checker_free(NovaBorrowChecker *checker);

/**
 * Declare a new variable
 */
void nova_borrow_checker_declare_variable(
    NovaBorrowChecker *checker,
    const char *name
);

/**
 * Check if immutable borrow is allowed
 */
bool nova_borrow_checker_can_borrow_immutable(
    NovaBorrowChecker *checker,
    const char *var_name
);

/**
 * Try to create an immutable borrow (&T)
 */
bool nova_borrow_checker_borrow_immutable(
    NovaBorrowChecker *checker,
    const char *var_name,
    int location
);

/**
 * Check if mutable borrow is allowed
 */
bool nova_borrow_checker_can_borrow_mutable(
    NovaBorrowChecker *checker,
    const char *var_name
);

/**
 * Try to create a mutable borrow (&mut T)
 */
bool nova_borrow_checker_borrow_mutable(
    NovaBorrowChecker *checker,
    const char *var_name,
    int location
);

/**
 * Move a value (transfer ownership)
 */
bool nova_borrow_checker_move_value(
    NovaBorrowChecker *checker,
    const char *var_name,
    int location
);

/**
 * Check if a variable can be used (not moved)
 */
bool nova_borrow_checker_check_use(
    NovaBorrowChecker *checker,
    const char *var_name,
    int location
);

/**
 * Enter a new scope
 */
void nova_borrow_checker_enter_scope(NovaBorrowChecker *checker);

/**
 * Exit current scope (cleans up borrows)
 */
void nova_borrow_checker_exit_scope(NovaBorrowChecker *checker);

/**
 * Check if there are any errors
 */
bool nova_borrow_checker_has_errors(NovaBorrowChecker *checker);

/**
 * Print all errors
 */
void nova_borrow_checker_print_errors(NovaBorrowChecker *checker);

/**
 * Enable/disable borrow checking
 */
void nova_borrow_checker_set_enabled(NovaBorrowChecker *checker, bool enabled);

#endif // NOVA_BORROW_CHECKER_H
