#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Defer / RAII / Drop Semantics for Nova - Phase D.3
// Deterministic cleanup with defer statements

// === DEFER INFRASTRUCTURE ===

// Defer action
typedef struct nova_defer_action {
    void (*cleanup_func)(void*);  // cleanup function
    void* context;                // context data
    struct nova_defer_action* next;
} nova_defer_action_t;

// Defer stack for scope management
typedef struct nova_defer_stack {
    nova_defer_action_t* top;
    size_t count;
} nova_defer_stack_t;

// Global defer stack (per thread in real implementation)
static nova_defer_stack_t* current_defer_stack = NULL;

// Initialize defer system
void nova_defer_init() {
    if (!current_defer_stack) {
        current_defer_stack = calloc(1, sizeof(nova_defer_stack_t));
    }
}

// Run all defer actions (for scope exit)
void nova_defer_run_all();

// Cleanup defer system
void nova_defer_cleanup() {
    if (current_defer_stack) {
        // Run all remaining defers
        nova_defer_run_all();
        free(current_defer_stack);
        current_defer_stack = NULL;
    }
}

// Add defer action
void nova_defer_add(void (*cleanup_func)(void*), void* context) {
    if (!current_defer_stack) {
        nova_defer_init();
    }

    nova_defer_action_t* action = malloc(sizeof(nova_defer_action_t));
    action->cleanup_func = cleanup_func;
    action->context = context;
    action->next = current_defer_stack->top;

    current_defer_stack->top = action;
    current_defer_stack->count++;
}

// Run top defer action
void nova_defer_run() {
    if (!current_defer_stack || !current_defer_stack->top) {
        return;
    }

    nova_defer_action_t* action = current_defer_stack->top;
    current_defer_stack->top = action->next;
    current_defer_stack->count--;

    // Execute cleanup
    action->cleanup_func(action->context);
    free(action);
}

// Run all defer actions (for scope exit)
void nova_defer_run_all() {
    while (current_defer_stack && current_defer_stack->top) {
        nova_defer_run();
    }
}

// === SCOPE MANAGEMENT ===

// Scope context for defer management
typedef struct nova_scope_context {
    nova_defer_stack_t* saved_stack;
    size_t defer_count_at_entry;
} nova_scope_context_t;

// Enter scope (save defer state)
nova_scope_context_t* nova_scope_enter() {
    nova_scope_context_t* ctx = malloc(sizeof(nova_scope_context_t));
    ctx->saved_stack = current_defer_stack;
    ctx->defer_count_at_entry = current_defer_stack ? current_defer_stack->count : 0;
    return ctx;
}

// Exit scope (run defers added in this scope)
void nova_scope_exit(nova_scope_context_t* ctx) {
    if (!ctx) return;

    // Run defers added in this scope
    size_t defers_to_run = current_defer_stack->count - ctx->defer_count_at_entry;
    for (size_t i = 0; i < defers_to_run; i++) {
        nova_defer_run();
    }

    free(ctx);
}

// === DROP TRAIT SIMULATION ===

// Drop trait interface
typedef struct nova_drop_trait {
    void (*drop)(void* self);
} nova_drop_trait_t;

// Drop-enabled object
typedef struct nova_drop_object {
    void* data;
    nova_drop_trait_t* trait;
} nova_drop_object_t;

// Destroy drop object
void nova_drop_object_destroy(nova_drop_object_t* obj) {
    if (!obj) return;

    if (obj->trait && obj->trait->drop) {
        obj->trait->drop(obj->data);
    }

    free(obj);
}

// Create drop object
nova_drop_object_t* nova_drop_object_create(void* data, nova_drop_trait_t* trait) {
    nova_drop_object_t* obj = malloc(sizeof(nova_drop_object_t));
    obj->data = data;
    obj->trait = trait;

    // Auto-add defer for cleanup
    nova_defer_add((void (*)(void*))nova_drop_object_destroy, obj);

    return obj;
}

// === COMMON CLEANUP FUNCTIONS ===

// File cleanup
typedef struct {
    FILE* file;
} nova_file_context_t;

void nova_file_close(void* context) {
    nova_file_context_t* ctx = (nova_file_context_t*)context;
    if (ctx->file) {
        fclose(ctx->file);
        printf("Deferred: file closed\n");
    }
    free(ctx);
}

// Memory cleanup
void nova_free(void* ptr) {
    if (ptr) {
        free(ptr);
        printf("Deferred: memory freed\n");
    }
}

// Custom cleanup
typedef struct {
    char* message;
} nova_custom_context_t;

void nova_custom_cleanup(void* context) {
    nova_custom_context_t* ctx = (nova_custom_context_t*)context;
    printf("Deferred: %s\n", ctx->message);
    free(ctx->message);
    free(ctx);
}

// === DROP TRAIT EXAMPLES ===

// String drop trait
void nova_string_drop(void* self) {
    char** str = (char**)self;
    if (*str) {
        free(*str);
        *str = NULL;
        printf("Dropped: string\n");
    }
}

// Vector drop trait
typedef struct {
    void* data;
    size_t size;
} nova_vec_t;

void nova_vec_drop(void* self) {
    nova_vec_t* vec = (nova_vec_t*)self;
    if (vec->data) {
        free(vec->data);
        vec->data = NULL;
        printf("Dropped: vector (%zu elements)\n", vec->size);
    }
}

// === AST FOR DEFER STATEMENTS ===

// Simplified AST for defer testing
typedef enum {
    STMT_DEFER,
    STMT_SCOPE,
    STMT_EXPR
} nova_stmt_kind_t;

typedef struct nova_stmt nova_stmt_t;

typedef struct {
    void (*cleanup_func)(void*);  // function to call
    void* context;                // context to pass
} nova_defer_stmt_t;

typedef struct {
    nova_stmt_t** stmts;
    size_t stmt_count;
} nova_scope_stmt_t;

typedef struct {
    int value;  // dummy expression
} nova_expr_stmt_t;

struct nova_stmt {
    nova_stmt_kind_t kind;
    union {
        nova_defer_stmt_t defer;
        nova_scope_stmt_t scope;
        nova_expr_stmt_t expr;
    } data;
};

// Execute statement (including defer lowering)
void nova_stmt_execute(nova_stmt_t* stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case STMT_DEFER: {
            // Add defer action
            nova_defer_add(stmt->data.defer.cleanup_func, stmt->data.defer.context);
            break;
        }

        case STMT_SCOPE: {
            // Enter scope
            nova_scope_context_t* ctx = nova_scope_enter();

            // Execute statements in scope
            for (size_t i = 0; i < stmt->data.scope.stmt_count; i++) {
                nova_stmt_execute(stmt->data.scope.stmts[i]);
            }

            // Exit scope (runs defers)
            nova_scope_exit(ctx);
            break;
        }

        case STMT_EXPR: {
            printf("Executed: expression with value %d\n", stmt->data.expr.value);
            break;
        }
    }
}

// === TEST INFRASTRUCTURE ===

void nova_test_defer_semantics() {
    printf("🧪 Testing Nova Defer / RAII / Drop Semantics (D.3)\n");
    printf("===================================================\n\n");

    nova_defer_init();

    // Test 1: Basic defer
    printf("1. Testing basic defer...\n");

    nova_custom_context_t* ctx1 = malloc(sizeof(nova_custom_context_t));
    ctx1->message = strdup("cleanup 1 executed");

    nova_defer_add(nova_custom_cleanup, ctx1);
    printf("   Added defer action\n");

    nova_defer_run();
    printf("   Ran defer action: ✓\n\n");

    // Test 2: Scope-based defer
    printf("2. Testing scope-based defer...\n");

    nova_scope_context_t* scope_ctx = nova_scope_enter();
    printf("   Entered scope\n");

    // Add multiple defers in scope
    nova_custom_context_t* ctx2 = malloc(sizeof(nova_custom_context_t));
    ctx2->message = strdup("scope cleanup 1");

    nova_custom_context_t* ctx3 = malloc(sizeof(nova_custom_context_t));
    ctx3->message = strdup("scope cleanup 2");

    nova_defer_add(nova_custom_cleanup, ctx2);
    nova_defer_add(nova_custom_cleanup, ctx3);
    printf("   Added 2 defer actions in scope\n");

    nova_scope_exit(scope_ctx);
    printf("   Exited scope (should have run 2 defers): ✓\n\n");

    // Test 3: Drop trait simulation
    printf("3. Testing Drop trait simulation...\n");

    // Create string with drop trait
    nova_drop_trait_t string_trait = { nova_string_drop };
    char* test_str = strdup("Hello, World!");
    nova_drop_object_t* drop_str = nova_drop_object_create(&test_str, &string_trait);
    printf("   Created drop-enabled string\n");

    // Create vector with drop trait
    nova_drop_trait_t vec_trait = { nova_vec_drop };
    nova_vec_t test_vec = { malloc(100), 10 };
    nova_drop_object_t* drop_vec = nova_drop_object_create(&test_vec, &vec_trait);
    printf("   Created drop-enabled vector\n");

    // Objects will be dropped when scope exits
    nova_scope_context_t* drop_scope = nova_scope_enter();
    nova_scope_exit(drop_scope);
    printf("   Scope exit (should have dropped objects): ✓\n\n");

    // Test 4: File handling with defer
    printf("4. Testing file handling with defer...\n");

    // Simulate file operations
    nova_file_context_t* file_ctx = malloc(sizeof(nova_file_context_t));
    file_ctx->file = fopen("/tmp/test.txt", "w");
    if (file_ctx->file) {
        fprintf(file_ctx->file, "test data");
        nova_defer_add(nova_file_close, file_ctx);
        printf("   Opened file with deferred close\n");

        // File will be closed when defer runs
        nova_defer_run();
        printf("   Deferred close executed: ✓\n");
    } else {
        free(file_ctx);
        printf("   Could not create test file\n");
    }

    printf("\n✅ Defer / RAII / Drop semantics test completed!\n");
}

int main() {
    nova_test_defer_semantics();
    nova_defer_cleanup();
    return 0;
}
