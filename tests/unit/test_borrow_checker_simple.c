#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Minimal test for borrow checker core logic
typedef struct {
    int file_id;
    size_t start, end, line, col;
} nova_span_t;

typedef struct {
    int level;
    nova_span_t span;
    char* message;
} nova_diag_t;

typedef struct {
    nova_diag_t** diags;
    size_t count, capacity;
} nova_diag_collector_t;

nova_diag_collector_t* nova_diag_collector_create(void) {
    nova_diag_collector_t* dc = malloc(sizeof(nova_diag_collector_t));
    dc->capacity = 16;
    dc->count = 0;
    dc->diags = malloc(sizeof(nova_diag_t*) * dc->capacity);
    return dc;
}

void nova_diag_error(nova_diag_collector_t* dc, nova_span_t span, const char* message, ...) {
    // Simplified - just use the message
    if (dc->count >= dc->capacity) {
        dc->capacity *= 2;
        dc->diags = realloc(dc->diags, sizeof(nova_diag_t*) * dc->capacity);
    }
    nova_diag_t* diag = malloc(sizeof(nova_diag_t));
    diag->level = 1; // error
    diag->span = span;
    diag->message = strdup(message);
    dc->diags[dc->count++] = diag;
}

void nova_diag_print(nova_diag_collector_t* dc, void* sm) {
    for (size_t i = 0; i < dc->count; i++) {
        nova_diag_t* diag = dc->diags[i];
        printf("error: %s\n", diag->message);
    }
}

void nova_diag_collector_destroy(nova_diag_collector_t* dc) {
    if (!dc) return;
    for (size_t i = 0; i < dc->count; i++) {
        free(dc->diags[i]->message);
        free(dc->diags[i]);
    }
    free(dc->diags);
    free(dc);
}

typedef enum {
    BORROW_SHARED,
    BORROW_MUT,
    BORROW_OWNED
} nova_borrow_kind_t;

typedef struct {
    nova_borrow_kind_t kind;
    char* var_name;
    int scope_level;
    nova_span_t span;
    bool is_active;
} nova_borrow_info_t;

typedef struct {
    nova_borrow_info_t** borrows;
    int borrow_count;
    int borrow_capacity;
    int current_scope;
} nova_borrow_context_t;

typedef struct {
    nova_diag_collector_t* diag;
    nova_borrow_context_t* ctx;
} nova_borrow_checker_t;

nova_borrow_checker_t* nova_borrow_checker_create(nova_diag_collector_t* diag) {
    nova_borrow_checker_t* bc = calloc(1, sizeof(nova_borrow_checker_t));
    bc->diag = diag;
    bc->ctx = calloc(1, sizeof(nova_borrow_context_t));
    bc->ctx->borrows = NULL;
    bc->ctx->borrow_count = 0;
    bc->ctx->borrow_capacity = 0;
    bc->ctx->current_scope = 0;
    return bc;
}

void nova_borrow_checker_destroy(nova_borrow_checker_t* bc) {
    if (!bc) return;
    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        free(bc->ctx->borrows[i]->var_name);
        free(bc->ctx->borrows[i]);
    }
    free(bc->ctx->borrows);
    free(bc->ctx);
    free(bc);
}

void nova_borrow_enter_scope(nova_borrow_checker_t* bc) {
    bc->ctx->current_scope++;
}

void nova_borrow_exit_scope(nova_borrow_checker_t* bc) {
    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        if (bc->ctx->borrows[i]->scope_level == bc->ctx->current_scope) {
            bc->ctx->borrows[i]->is_active = false;
        }
    }
    bc->ctx->current_scope--;
}

static void add_borrow(nova_borrow_context_t* ctx, nova_borrow_info_t* borrow) {
    if (ctx->borrow_count >= ctx->borrow_capacity) {
        ctx->borrow_capacity = ctx->borrow_capacity == 0 ? 16 : ctx->borrow_capacity * 2;
        ctx->borrows = realloc(ctx->borrows, ctx->borrow_capacity * sizeof(nova_borrow_info_t*));
    }
    ctx->borrows[ctx->borrow_count++] = borrow;
}

bool nova_borrow_register(nova_borrow_checker_t* bc,
                         nova_borrow_kind_t kind,
                         const char* var_name,
                         nova_span_t span) {
    // Check for conflicts
    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        nova_borrow_info_t* existing = bc->ctx->borrows[i];
        if (!existing->is_active) continue;

        if (strcmp(existing->var_name, var_name) == 0) {
            if (existing->kind == BORROW_MUT || kind == BORROW_MUT) {
                nova_diag_error(bc->diag, span,
                    "borrow conflict: cannot borrow `%s` as `%s` because it is already borrowed as `%s`",
                    var_name,
                    kind == BORROW_MUT ? "mutable" : "immutable",
                    existing->kind == BORROW_MUT ? "mutable" : "immutable");
                return false;
            }
        }
    }

    // Register new borrow
    nova_borrow_info_t* borrow = calloc(1, sizeof(nova_borrow_info_t));
    borrow->kind = kind;
    borrow->var_name = strdup(var_name);
    borrow->scope_level = bc->ctx->current_scope;
    borrow->span = span;
    borrow->is_active = true;

    add_borrow(bc->ctx, borrow);
    return true;
}

void nova_borrow_debug_print(nova_borrow_checker_t* bc) {
    printf("=== BORROW STATE ===\n");
    printf("Scope: %d\n", bc->ctx->current_scope);
    printf("Active borrows: %d\n", bc->ctx->borrow_count);

    for (int i = 0; i < bc->ctx->borrow_count; i++) {
        nova_borrow_info_t* borrow = bc->ctx->borrows[i];
        if (!borrow->is_active) continue;

        printf("  %s: %s (scope %d)\n",
               borrow->var_name,
               borrow->kind == BORROW_SHARED ? "shared" :
               borrow->kind == BORROW_MUT ? "mutable" : "owned",
               borrow->scope_level);
    }
    printf("===================\n");
}

int main() {
    printf("🧪 Testing Nova Borrow Checker Core Logic\n");
    printf("=========================================\n\n");

    // Create diagnostics
    nova_diag_collector_t* diag = nova_diag_collector_create();

    // Create borrow checker
    nova_borrow_checker_t* bc = nova_borrow_checker_create(diag);

    nova_span_t span = {0, 0, 10, 1, 5};

    // Test basic borrow registration
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

    // Try to register shared borrow on mutable variable (should fail)
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

    // Check diagnostics
    printf("6. Diagnostics summary:\n");
    printf("   Total diagnostics: %zu\n", diag->count);
    nova_diag_print(diag, NULL);

    // Cleanup
    nova_borrow_checker_destroy(bc);
    nova_diag_collector_destroy(diag);

    printf("\n✅ Borrow checker core logic test completed!\n");

    return 0;
}
