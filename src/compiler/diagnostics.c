#include <stdbool.h>

// Diagnostics Implementation
// Error reporting with source locations and spans

#include "compiler/ast.h"
#include "compiler/sourcemgr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// Diagnostic Collector
// ═══════════════════════════════════════════════════════════════════════════

nova_diag_collector_t* nova_diag_collector_create(void) {
    nova_diag_collector_t* dc = (nova_diag_collector_t*)malloc(sizeof(nova_diag_collector_t));
    if (!dc) return NULL;

    dc->capacity = 16;
    dc->count = 0;
    dc->diags = (nova_diag_t**)malloc(sizeof(nova_diag_t*) * dc->capacity);

    if (!dc->diags) {
        free(dc);
        return NULL;
    }

    return dc;
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

// Add diagnostic
static void nova_diag_add(nova_diag_collector_t* dc, nova_diag_level_t level,
                         nova_span_t span, const char* message) {
    if (!dc) return;

    // Expand capacity if needed
    if (dc->count >= dc->capacity) {
        dc->capacity *= 2;
        nova_diag_t** new_diags = (nova_diag_t**)realloc(dc->diags,
                                                         sizeof(nova_diag_t*) * dc->capacity);
        if (!new_diags) return;
        dc->diags = new_diags;
    }

    // Create diagnostic
    nova_diag_t* diag = (nova_diag_t*)malloc(sizeof(nova_diag_t));
    if (!diag) return;

    diag->level = level;
    diag->span = span;
    diag->message = strdup(message);

    dc->diags[dc->count++] = diag;
}

// Public diagnostic functions
void nova_diag_error(nova_diag_collector_t* dc, nova_span_t span, const char* message) {
    nova_diag_add(dc, DIAG_ERROR, span, message);
}

void nova_diag_warning(nova_diag_collector_t* dc, nova_span_t span, const char* message) {
    nova_diag_add(dc, DIAG_WARNING, span, message);
}

// Print diagnostics with source context
void nova_diag_print(nova_diag_collector_t* dc, nova_sourcemgr_t* sm) {
    if (!dc) return;

    for (size_t i = 0; i < dc->count; i++) {
        nova_diag_t* diag = dc->diags[i];

        // Get filename
        const char* filename = sm ? nova_sourcemgr_get_filename(sm, diag->span.file_id) : "<unknown>";
        if (!filename) filename = "<unknown>";

        // Print diagnostic header
        const char* level_str;
        switch (diag->level) {
            case DIAG_ERROR: level_str = "error"; break;
            case DIAG_WARNING: level_str = "warning"; break;
            case DIAG_INFO: level_str = "info"; break;
            default: level_str = "unknown"; break;
        }

        fprintf(stderr, "%s: %s\n", level_str, diag->message);
        fprintf(stderr, "  --> %s:%zu:%zu\n", filename, diag->span.line, diag->span.col);

        // Try to print source line if sourcemgr available
        if (sm) {
            size_t line_length;
            const char* line_content = nova_sourcemgr_get_line(sm, diag->span.file_id,
                                                              (int)diag->span.line, &line_length);

            if (line_content && line_length > 0) {
                fprintf(stderr, "   |\n");
                fprintf(stderr, "%3zu| %.*s\n", diag->span.line, (int)line_length, line_content);
                fprintf(stderr, "   | ");

                // Calculate underline position
                size_t start_col = diag->span.col - 1;
                size_t end_col = start_col + (diag->span.end - diag->span.start);

                for (size_t i = 0; i < line_length; i++) {
                    if (i >= start_col && i < end_col) {
                        fprintf(stderr, "^");
                    } else {
                        fprintf(stderr, " ");
                    }
                }
                fprintf(stderr, "\n");
            }
        }
    }
}
