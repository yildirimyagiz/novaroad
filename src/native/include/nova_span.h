/**
 * Nova Source Span
 * Lightweight source location tracking
 */

#ifndef NOVA_SPAN_H
#define NOVA_SPAN_H

#include <stddef.h>
#include <stdbool.h>

// Forward declaration - full definition in nova_diagnostics.h
typedef struct {
  const char *filename;
  size_t line;
  size_t column;
  size_t offset;
  size_t length;
} SourceSpan;

// Quick span creation
SourceSpan span_new(const char *filename, size_t line, size_t column, size_t length);
SourceSpan span_empty(void);
bool span_is_empty(SourceSpan span);

#endif // NOVA_SPAN_H
