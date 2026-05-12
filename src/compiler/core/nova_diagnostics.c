/**
 * Nova Diagnostics Implementation
 */

#include "compiler/nova_diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ═══════════════════════════════════════════════════════════════════════════
// ANSI COLOR CODES
// ═══════════════════════════════════════════════════════════════════════════

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// ═══════════════════════════════════════════════════════════════════════════
// CONTEXT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

DiagnosticContext *diagnostic_context_create(void) {
  DiagnosticContext *ctx = (DiagnosticContext *)calloc(1, sizeof(DiagnosticContext));
  if (!ctx) return NULL;
  
  ctx->use_color = true;  // Auto-detect terminal in production
  ctx->show_context = true;
  ctx->context_lines = 2;
  
  ctx->source_capacity = 16;
  ctx->source_files = (char **)calloc(ctx->source_capacity, sizeof(char*));
  ctx->source_contents = (char **)calloc(ctx->source_capacity, sizeof(char*));
  
  return ctx;
}

void diagnostic_context_destroy(DiagnosticContext *ctx) {
  if (!ctx) return;
  
  diagnostic_context_clear(ctx);
  
  for (size_t i = 0; i < ctx->source_count; i++) {
    free(ctx->source_files[i]);
    free(ctx->source_contents[i]);
  }
  free(ctx->source_files);
  free(ctx->source_contents);
  
  free(ctx);
}

void diagnostic_context_clear(DiagnosticContext *ctx) {
  if (!ctx) return;
  
  Diagnostic *diag = ctx->diagnostics;
  while (diag) {
    Diagnostic *next = diag->next;
    
    free(diag->message);
    free(diag->suggestion);
    
    for (size_t i = 0; i < diag->related_count; i++) {
      free(diag->related_messages[i]);
    }
    free(diag->related_spans);
    free(diag->related_messages);
    
    free(diag);
    diag = next;
  }
  
  ctx->diagnostics = NULL;
  ctx->error_count = 0;
  ctx->warning_count = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// ADD DIAGNOSTICS
// ═══════════════════════════════════════════════════════════════════════════

static Diagnostic *diagnostic_create(DiagnosticLevel level, SourceSpan span, const char *message) {
  Diagnostic *diag = (Diagnostic *)calloc(1, sizeof(Diagnostic));
  if (!diag) return NULL;
  
  diag->level = level;
  diag->message = message ? strdup(message) : NULL;
  diag->span = span;
  diag->next = NULL;
  
  return diag;
}

static void diagnostic_add(DiagnosticContext *ctx, Diagnostic *diag) {
  if (!ctx || !diag) return;
  
  // Add to end of list
  if (!ctx->diagnostics) {
    ctx->diagnostics = diag;
  } else {
    Diagnostic *last = ctx->diagnostics;
    while (last->next) {
      last = last->next;
    }
    last->next = diag;
  }
  
  // Update counts
  if (diag->level == DIAG_ERROR) {
    ctx->error_count++;
  } else if (diag->level == DIAG_WARNING) {
    ctx->warning_count++;
  }
}

void diagnostic_error(DiagnosticContext *ctx, SourceSpan span, const char *message) {
  Diagnostic *diag = diagnostic_create(DIAG_ERROR, span, message);
  diagnostic_add(ctx, diag);
}

void diagnostic_warning(DiagnosticContext *ctx, SourceSpan span, const char *message) {
  Diagnostic *diag = diagnostic_create(DIAG_WARNING, span, message);
  diagnostic_add(ctx, diag);
}

void diagnostic_note(DiagnosticContext *ctx, SourceSpan span, const char *message) {
  Diagnostic *diag = diagnostic_create(DIAG_NOTE, span, message);
  diagnostic_add(ctx, diag);
}

void diagnostic_help(DiagnosticContext *ctx, SourceSpan span, const char *message) {
  Diagnostic *diag = diagnostic_create(DIAG_HELP, span, message);
  diagnostic_add(ctx, diag);
}

void diagnostic_errorf(DiagnosticContext *ctx, SourceSpan span, const char *fmt, ...) {
  char buffer[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  
  diagnostic_error(ctx, span, buffer);
}

void diagnostic_warningf(DiagnosticContext *ctx, SourceSpan span, const char *fmt, ...) {
  char buffer[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  
  diagnostic_warning(ctx, span, buffer);
}

// ═══════════════════════════════════════════════════════════════════════════
// DIAGNOSTIC ENHANCEMENT
// ═══════════════════════════════════════════════════════════════════════════

void diagnostic_add_related(Diagnostic *diag, SourceSpan span, const char *message) {
  if (!diag) return;
  
  size_t new_count = diag->related_count + 1;
  diag->related_spans = (SourceSpan *)realloc(diag->related_spans, 
                                               new_count * sizeof(SourceSpan));
  diag->related_messages = (char **)realloc(diag->related_messages, 
                                             new_count * sizeof(char*));
  
  diag->related_spans[diag->related_count] = span;
  diag->related_messages[diag->related_count] = message ? strdup(message) : NULL;
  diag->related_count = new_count;
}

void diagnostic_add_suggestion(Diagnostic *diag, const char *suggestion) {
  if (!diag) return;
  
  if (diag->suggestion) {
    free(diag->suggestion);
  }
  diag->suggestion = suggestion ? strdup(suggestion) : NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// QUERYING
// ═══════════════════════════════════════════════════════════════════════════

bool diagnostic_has_errors(const DiagnosticContext *ctx) {
  return ctx && ctx->error_count > 0;
}

size_t diagnostic_error_count(const DiagnosticContext *ctx) {
  return ctx ? ctx->error_count : 0;
}

size_t diagnostic_warning_count(const DiagnosticContext *ctx) {
  return ctx ? ctx->warning_count : 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// SOURCE FILE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void diagnostic_register_source(DiagnosticContext *ctx, const char *filename, const char *content) {
  if (!ctx || !filename || !content) return;
  
  if (ctx->source_count >= ctx->source_capacity) {
    ctx->source_capacity *= 2;
    ctx->source_files = (char **)realloc(ctx->source_files, 
                                          ctx->source_capacity * sizeof(char*));
    ctx->source_contents = (char **)realloc(ctx->source_contents,
                                             ctx->source_capacity * sizeof(char*));
  }
  
  ctx->source_files[ctx->source_count] = strdup(filename);
  ctx->source_contents[ctx->source_count] = strdup(content);
  ctx->source_count++;
}

const char *diagnostic_get_source_line(DiagnosticContext *ctx, const char *filename, size_t line) {
  if (!ctx || !filename) return NULL;
  
  // Find source file
  const char *content = NULL;
  for (size_t i = 0; i < ctx->source_count; i++) {
    if (strcmp(ctx->source_files[i], filename) == 0) {
      content = ctx->source_contents[i];
      break;
    }
  }
  
  if (!content) return NULL;
  
  // Extract line
  size_t current_line = 1;
  const char *line_start = content;
  const char *p = content;
  
  while (*p && current_line < line) {
    if (*p == '\n') {
      current_line++;
      line_start = p + 1;
    }
    p++;
  }
  
  if (current_line != line) return NULL;
  
  // Find line end
  const char *line_end = line_start;
  while (*line_end && *line_end != '\n') {
    line_end++;
  }
  
  // Copy line
  size_t len = line_end - line_start;
  static char line_buffer[1024];
  if (len >= sizeof(line_buffer)) {
    len = sizeof(line_buffer) - 1;
  }
  memcpy(line_buffer, line_start, len);
  line_buffer[len] = '\0';
  
  return line_buffer;
}

// ═══════════════════════════════════════════════════════════════════════════
// PRINTING
// ═══════════════════════════════════════════════════════════════════════════

static const char *level_label(DiagnosticLevel level, bool use_color) {
  if (use_color) {
    switch (level) {
      case DIAG_ERROR:   return COLOR_BOLD COLOR_RED "error" COLOR_RESET;
      case DIAG_WARNING: return COLOR_BOLD COLOR_YELLOW "warning" COLOR_RESET;
      case DIAG_NOTE:    return COLOR_BOLD COLOR_CYAN "note" COLOR_RESET;
      case DIAG_HELP:    return COLOR_BOLD COLOR_BLUE "help" COLOR_RESET;
    }
  } else {
    switch (level) {
      case DIAG_ERROR:   return "error";
      case DIAG_WARNING: return "warning";
      case DIAG_NOTE:    return "note";
      case DIAG_HELP:    return "help";
    }
  }
  return "unknown";
}

void diagnostic_print(const Diagnostic *diag, const DiagnosticContext *ctx) {
  if (!diag) return;
  
  bool use_color = ctx && ctx->use_color;
  
  // Print header: error: message
  fprintf(stderr, "%s: %s\n", level_label(diag->level, use_color), diag->message);
  
  // Print location
  if (diag->span.filename) {
    fprintf(stderr, "  %s--> %s:%zu:%zu\n",
            use_color ? COLOR_BLUE : "",
            diag->span.filename,
            diag->span.line,
            diag->span.column);
    if (use_color) fprintf(stderr, COLOR_RESET);
  }
  
  // Print source context
  if (ctx && ctx->show_context && diag->span.filename) {
    const char *line = diagnostic_get_source_line((DiagnosticContext *)ctx, 
                                                    diag->span.filename, 
                                                    diag->span.line);
    if (line) {
      // Line number
      fprintf(stderr, "%s%5zu |%s ", 
              use_color ? COLOR_BLUE : "",
              diag->span.line,
              use_color ? COLOR_RESET : "");
      fprintf(stderr, "%s\n", line);
      
      // Underline
      fprintf(stderr, "      %s|%s ",
              use_color ? COLOR_BLUE : "",
              use_color ? COLOR_RESET : "");
      
      for (size_t i = 1; i < diag->span.column; i++) {
        fprintf(stderr, " ");
      }
      
      if (use_color) {
        fprintf(stderr, COLOR_RED);
      }
      
      for (size_t i = 0; i < diag->span.length && i < 80; i++) {
        fprintf(stderr, "^");
      }
      
      if (use_color) {
        fprintf(stderr, COLOR_RESET);
      }
      
      fprintf(stderr, "\n");
    }
  }
  
  // Print related information
  for (size_t i = 0; i < diag->related_count; i++) {
    fprintf(stderr, "  note: %s\n", diag->related_messages[i]);
    if (diag->related_spans[i].filename) {
      fprintf(stderr, "    --> %s:%zu:%zu\n",
              diag->related_spans[i].filename,
              diag->related_spans[i].line,
              diag->related_spans[i].column);
    }
  }
  
  // Print suggestion
  if (diag->suggestion) {
    fprintf(stderr, "  %shelp%s: %s\n",
            use_color ? COLOR_BOLD COLOR_BLUE : "",
            use_color ? COLOR_RESET : "",
            diag->suggestion);
  }
  
  fprintf(stderr, "\n");
}

void diagnostic_emit(DiagnosticContext *ctx, Diagnostic *diag) {
  diagnostic_print(diag, ctx);
}

void diagnostic_emit_all(DiagnosticContext *ctx) {
  if (!ctx) return;
  
  Diagnostic *diag = ctx->diagnostics;
  while (diag) {
    diagnostic_print(diag, ctx);
    diag = diag->next;
  }
  
  // Summary
  if (ctx->error_count > 0 || ctx->warning_count > 0) {
    fprintf(stderr, "Compilation %s with %zu error(s) and %zu warning(s)\n",
            ctx->error_count > 0 ? "failed" : "succeeded",
            ctx->error_count,
            ctx->warning_count);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// SPAN UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

SourceSpan span_create(const char *filename, size_t line, size_t column, size_t length) {
  SourceSpan span = {
    .filename = filename,
    .line = line,
    .column = column,
    .offset = 0,
    .length = length
  };
  return span;
}

SourceSpan span_from_offset(const char *filename, size_t offset, size_t length) {
  SourceSpan span = {
    .filename = filename,
    .line = 0,  // Would need to calculate from offset
    .column = 0,
    .offset = offset,
    .length = length
  };
  return span;
}

SourceSpan span_merge(SourceSpan a, SourceSpan b) {
  // Assumes same file
  size_t start = a.offset < b.offset ? a.offset : b.offset;
  size_t end = (a.offset + a.length) > (b.offset + b.length) ? 
               (a.offset + a.length) : (b.offset + b.length);
  
  return span_from_offset(a.filename, start, end - start);
}

bool span_contains(SourceSpan outer, SourceSpan inner) {
  if (outer.filename != inner.filename) return false;
  
  return inner.offset >= outer.offset && 
         (inner.offset + inner.length) <= (outer.offset + outer.length);
}
