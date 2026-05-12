/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA OWNERSHIP SYSTEM ║ ║ ║ ║  Compile-time ownership
 * tracking for zero-cost abstractions                  ║ ║  Inspired by Rust's
 * ownership model, adapted for Nova                      ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_OWNERSHIP_H
#define NOVA_OWNERSHIP_H

#include "nova_ast.h"
#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════════
// OWNERSHIP KINDS
// ═══════════════════════════════════════════════════════════════════════════════

typedef enum {
  OWNERSHIP_OWNED,        // Variable owns the value (can move)
  OWNERSHIP_BORROWED,     // Immutable borrow (&T)
  OWNERSHIP_BORROWED_MUT, // Mutable borrow (&mut T)
  OWNERSHIP_MOVED,        // Value has been moved (use-after-move error)
  OWNERSHIP_COPIED,       // Value was copied (Copy types)
  OWNERSHIP_UNKNOWN       // Not yet analyzed
} OwnershipKind;

// ═══════════════════════════════════════════════════════════════════════════════
// OWNERSHIP METADATA (attached to AST nodes)
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct OwnershipMetadata {
  OwnershipKind kind;

  // Move tracking
  bool is_moved;     // Has this value been moved?
  int move_location; // Line number where it was moved

  // Borrow tracking
  int borrow_count;         // Number of active borrows
  int mutable_borrow_count; // Number of active mutable borrows

  // Drop tracking
  bool needs_drop;   // Does this value need a destructor?
  int drop_location; // Where should drop be called?

  // Lifetime (simple region-based)
  int lifetime_id;           // Lifetime region ID
  const char *lifetime_name; // Optional lifetime annotation

} OwnershipMetadata;

// ═══════════════════════════════════════════════════════════════════════════════
// OWNERSHIP CHECKER
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct OwnershipChecker {
  void *current_scope; // Points to opaque Scope struct
  int error_count;
  char **error_messages;
  int error_capacity;
  bool enabled;
} OwnershipChecker;

// ═══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Create ownership checker
 */
OwnershipChecker *nova_ownership_checker_new(void);

/**
 * Free ownership checker
 */
void nova_ownership_checker_free(OwnershipChecker *checker);

/**
 * Analyze AST for ownership violations
 */
bool nova_ownership_check(OwnershipChecker *checker, ASTNode *ast);

/**
 * Mark variable as moved
 */
void nova_ownership_mark_moved(OwnershipChecker *checker,
                                 const char *var_name, int location);

/**
 * Add borrow (immutable)
 */
bool nova_ownership_add_borrow(OwnershipChecker *checker,
                                 const char *var_name, int location);

/**
 * Add mutable borrow
 */
bool nova_ownership_add_borrow_mut(OwnershipChecker *checker,
                                     const char *var_name, int location);

/**
 * Remove borrow (when reference goes out of scope)
 */
void nova_ownership_remove_borrow(OwnershipChecker *checker,
                                    const char *var_name);

/**
 * Check if value can be used
 */
bool nova_ownership_can_use(OwnershipChecker *checker, const char *var_name,
                              int location);

/**
 * Insert drop point
 */
void nova_ownership_insert_drop(OwnershipChecker *checker,
                                  const char *var_name, int location);

/**
 * Get ownership metadata for variable
 */
OwnershipMetadata *nova_ownership_get_metadata(OwnershipChecker *checker,
                                                 const char *var_name);

// ═══════════════════════════════════════════════════════════════════════════════
// ERROR REPORTING
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * Report use-after-move error
 */
void nova_ownership_error_use_after_move(OwnershipChecker *checker,
                                           const char *var_name,
                                           int use_location, int move_location);

/**
 * Report borrow conflict error
 */
void nova_ownership_error_borrow_conflict(OwnershipChecker *checker,
                                            const char *var_name, int location);

/**
 * Print all errors
 */
void nova_ownership_print_errors(OwnershipChecker *checker);

#endif // NOVA_OWNERSHIP_H
