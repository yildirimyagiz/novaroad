/**
 * Nova String Library
 * String operations and utilities
 */

#ifndef NOVA_STRING_H
#define NOVA_STRING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// STRING TYPE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *data;
  size_t length;
  size_t capacity;
  bool is_owned;  // true if we own the data, false if it's a slice
} NovaString;

// ═══════════════════════════════════════════════════════════════════════════
// STRING CREATION
// ═══════════════════════════════════════════════════════════════════════════

NovaString *nova_string_new(void);
NovaString *nova_string_from_cstr(const char *cstr);
NovaString *nova_string_from_bytes(const char *bytes, size_t len);
NovaString *nova_string_with_capacity(size_t capacity);
NovaString *nova_string_clone(const NovaString *str);
void nova_string_destroy(NovaString *str);

// ═══════════════════════════════════════════════════════════════════════════
// STRING OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

// Append
void nova_string_push(NovaString *str, char c);
void nova_string_append(NovaString *str, const char *cstr);
void nova_string_append_string(NovaString *str, const NovaString *other);
void nova_string_append_bytes(NovaString *str, const char *bytes, size_t len);

// Insert/Remove
void nova_string_insert(NovaString *str, size_t index, char c);
void nova_string_remove(NovaString *str, size_t index);
void nova_string_clear(NovaString *str);

// Query
size_t nova_string_len(const NovaString *str);
bool nova_string_is_empty(const NovaString *str);
char nova_string_char_at(const NovaString *str, size_t index);
const char *nova_string_as_cstr(const NovaString *str);

// Comparison
bool nova_string_equals(const NovaString *a, const NovaString *b);
bool nova_string_equals_cstr(const NovaString *str, const char *cstr);
int nova_string_compare(const NovaString *a, const NovaString *b);

// Search
int64_t nova_string_find(const NovaString *str, const char *pattern);
int64_t nova_string_find_char(const NovaString *str, char c);
bool nova_string_starts_with(const NovaString *str, const char *prefix);
bool nova_string_ends_with(const NovaString *str, const char *suffix);
bool nova_string_contains(const NovaString *str, const char *pattern);

// Substring
NovaString *nova_string_substring(const NovaString *str, size_t start, size_t len);
NovaString *nova_string_slice(const NovaString *str, size_t start, size_t end);

// Split
NovaString **nova_string_split(const NovaString *str, const char *delimiter, size_t *count);
NovaString **nova_string_lines(const NovaString *str, size_t *count);

// Transform
NovaString *nova_string_to_upper(const NovaString *str);
NovaString *nova_string_to_lower(const NovaString *str);
NovaString *nova_string_trim(const NovaString *str);
NovaString *nova_string_trim_start(const NovaString *str);
NovaString *nova_string_trim_end(const NovaString *str);
NovaString *nova_string_replace(const NovaString *str, const char *from, const char *to);

// Join
NovaString *nova_string_join(NovaString **strings, size_t count, const char *separator);

// Conversion
int64_t nova_string_to_i64(const NovaString *str);
double nova_string_to_f64(const NovaString *str);
NovaString *nova_string_from_i64(int64_t value);
NovaString *nova_string_from_f64(double value);

// Format
NovaString *nova_string_format(const char *fmt, ...);

// UTF-8 support
size_t nova_string_char_count(const NovaString *str);  // Number of UTF-8 characters
bool nova_string_is_valid_utf8(const NovaString *str);

#endif // NOVA_STRING_H
