/**
 * Nova Diagnostics System
 * Beautiful error messages with source context
 */

#ifndef NOVA_DIAGNOSTICS_H
#define NOVA_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// DIAGNOSTIC LEVELS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  DIAG_ERROR,      // Fatal error - compilation fails
  DIAG_WARNING,    // Warning - compilation continues
  DIAG_NOTE,       // Informational note
  DIAG_HELP,       // Helpful suggestion
} DiagnosticLevel;

// ═══════════════════════════════════════════════════════════════════════════
// SOURCE SPAN (location in source code)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  const char *filename;
  size_t line;          // 1-based
  size_t column;        // 1-based
  size_t offset;        // Byte offset from start of file
  size_t length;        // Length of the span
} SourceSpan;

// ═══════════════════════════════════════════════════════════════════════════
// DIAGNOSTIC
// ═══════════════════════════════════════════════════════════════════════════

typedef struct Diagnostic {
  DiagnosticLevel level;
  char *message;
  SourceSpan span;
  
  // Related spans (for multi-location errors)
  SourceSpan *related_spans;
  char **related_messages;
  size_t related_count;
  
  // Suggestions
  char *suggestion;
  
  struct Diagnostic *next;  // For chaining
} Diagnostic;

// ═══════════════════════════════════════════════════════════════════════════
// DIAGNOSTIC CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  Diagnostic *diagnostics;
  size_t error_count;
  size_t warning_count;
  
  // Source file cache
  char **source_files;
  char **source_contents;
  size_t source_count;
  size_t source_capacity;
  
  // Options
  bool use_color;
  bool show_context;
  size_t context_lines;  // Lines of context before/after error
} DiagnosticContext;

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

// Context management
DiagnosticContext *diagnostic_context_create(void);
void diagnostic_context_destroy(DiagnosticContext *ctx);
void diagnostic_context_clear(DiagnosticContext *ctx);

// Add diagnostics
void diagnostic_error(DiagnosticContext *ctx, SourceSpan span, const char *message);
void diagnostic_warning(DiagnosticContext *ctx, SourceSpan span, const char *message);
void diagnostic_note(DiagnosticContext *ctx, SourceSpan span, const char *message);
void diagnostic_help(DiagnosticContext *ctx, SourceSpan span, const char *message);

// Add with formatted message
void diagnostic_errorf(DiagnosticContext *ctx, SourceSpan span, const char *fmt, ...);
void diagnostic_warningf(DiagnosticContext *ctx, SourceSpan span, const char *fmt, ...);

// Add related information
void diagnostic_add_related(Diagnostic *diag, SourceSpan span, const char *message);
void diagnostic_add_suggestion(Diagnostic *diag, const char *suggestion);

// Querying
bool diagnostic_has_errors(const DiagnosticContext *ctx);
size_t diagnostic_error_count(const DiagnosticContext *ctx);
size_t diagnostic_warning_count(const DiagnosticContext *ctx);

// Reporting
void diagnostic_emit(DiagnosticContext *ctx, Diagnostic *diag);
void diagnostic_emit_all(DiagnosticContext *ctx);
void diagnostic_print(const Diagnostic *diag, const DiagnosticContext *ctx);

// Source file management
void diagnostic_register_source(DiagnosticContext *ctx, const char *filename, const char *content);
const char *diagnostic_get_source_line(DiagnosticContext *ctx, const char *filename, size_t line);

// ═══════════════════════════════════════════════════════════════════════════
// SPAN UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

SourceSpan span_create(const char *filename, size_t line, size_t column, size_t length);
SourceSpan span_from_offset(const char *filename, size_t offset, size_t length);
SourceSpan span_merge(SourceSpan a, SourceSpan b);
bool span_contains(SourceSpan outer, SourceSpan inner);

#endif // NOVA_DIAGNOSTICS_H
