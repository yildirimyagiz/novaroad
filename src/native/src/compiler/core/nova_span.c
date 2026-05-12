/**
 * Nova Source Span Implementation
 */

#include "nova_span.h"

SourceSpan span_new(const char *filename, size_t line, size_t column, size_t length) {
  SourceSpan span = {
    .filename = filename,
    .line = line,
    .column = column,
    .offset = 0,
    .length = length
  };
  return span;
}

SourceSpan span_empty(void) {
  SourceSpan span = {
    .filename = NULL,
    .line = 0,
    .column = 0,
    .offset = 0,
    .length = 0
  };
  return span;
}

bool span_is_empty(SourceSpan span) {
  return span.filename == NULL && span.length == 0;
}
