/**
 * Nova String Implementation
 */

#include "nova_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define INITIAL_CAPACITY 16

// ═══════════════════════════════════════════════════════════════════════════
// STRING CREATION
// ═══════════════════════════════════════════════════════════════════════════

NovaString *nova_string_new(void) {
  NovaString *str = (NovaString *)malloc(sizeof(NovaString));
  if (!str) return NULL;
  
  str->capacity = INITIAL_CAPACITY;
  str->data = (char *)malloc(str->capacity);
  str->length = 0;
  str->is_owned = true;
  str->data[0] = '\0';
  
  return str;
}

NovaString *nova_string_from_cstr(const char *cstr) {
  if (!cstr) return nova_string_new();
  
  size_t len = strlen(cstr);
  NovaString *str = nova_string_with_capacity(len + 1);
  memcpy(str->data, cstr, len);
  str->data[len] = '\0';
  str->length = len;
  
  return str;
}

NovaString *nova_string_from_bytes(const char *bytes, size_t len) {
  NovaString *str = nova_string_with_capacity(len + 1);
  memcpy(str->data, bytes, len);
  str->data[len] = '\0';
  str->length = len;
  
  return str;
}

NovaString *nova_string_with_capacity(size_t capacity) {
  NovaString *str = (NovaString *)malloc(sizeof(NovaString));
  if (!str) return NULL;
  
  str->capacity = capacity;
  str->data = (char *)malloc(capacity);
  str->length = 0;
  str->is_owned = true;
  str->data[0] = '\0';
  
  return str;
}

NovaString *nova_string_clone(const NovaString *str) {
  if (!str) return NULL;
  return nova_string_from_bytes(str->data, str->length);
}

void nova_string_destroy(NovaString *str) {
  if (!str) return;
  
  if (str->is_owned && str->data) {
    free(str->data);
  }
  free(str);
}

// ═══════════════════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═══════════════════════════════════════════════════════════════════════════

static void nova_string_ensure_capacity(NovaString *str, size_t needed) {
  if (!str || !str->is_owned) return;
  
  if (needed <= str->capacity) return;
  
  size_t new_capacity = str->capacity * 2;
  while (new_capacity < needed) {
    new_capacity *= 2;
  }
  
  str->data = (char *)realloc(str->data, new_capacity);
  str->capacity = new_capacity;
}

// ═══════════════════════════════════════════════════════════════════════════
// STRING OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

void nova_string_push(NovaString *str, char c) {
  if (!str) return;
  
  nova_string_ensure_capacity(str, str->length + 2);
  str->data[str->length++] = c;
  str->data[str->length] = '\0';
}

void nova_string_append(NovaString *str, const char *cstr) {
  if (!str || !cstr) return;
  
  size_t len = strlen(cstr);
  nova_string_append_bytes(str, cstr, len);
}

void nova_string_append_string(NovaString *str, const NovaString *other) {
  if (!str || !other) return;
  nova_string_append_bytes(str, other->data, other->length);
}

void nova_string_append_bytes(NovaString *str, const char *bytes, size_t len) {
  if (!str || !bytes) return;
  
  nova_string_ensure_capacity(str, str->length + len + 1);
  memcpy(str->data + str->length, bytes, len);
  str->length += len;
  str->data[str->length] = '\0';
}

void nova_string_insert(NovaString *str, size_t index, char c) {
  if (!str || index > str->length) return;
  
  nova_string_ensure_capacity(str, str->length + 2);
  memmove(str->data + index + 1, str->data + index, str->length - index);
  str->data[index] = c;
  str->length++;
  str->data[str->length] = '\0';
}

void nova_string_remove(NovaString *str, size_t index) {
  if (!str || index >= str->length) return;
  
  memmove(str->data + index, str->data + index + 1, str->length - index - 1);
  str->length--;
  str->data[str->length] = '\0';
}

void nova_string_clear(NovaString *str) {
  if (!str) return;
  str->length = 0;
  str->data[0] = '\0';
}

// ═══════════════════════════════════════════════════════════════════════════
// QUERY
// ═══════════════════════════════════════════════════════════════════════════

size_t nova_string_len(const NovaString *str) {
  return str ? str->length : 0;
}

bool nova_string_is_empty(const NovaString *str) {
  return !str || str->length == 0;
}

char nova_string_char_at(const NovaString *str, size_t index) {
  if (!str || index >= str->length) return '\0';
  return str->data[index];
}

const char *nova_string_as_cstr(const NovaString *str) {
  return str ? str->data : "";
}

// ═══════════════════════════════════════════════════════════════════════════
// COMPARISON
// ═══════════════════════════════════════════════════════════════════════════

bool nova_string_equals(const NovaString *a, const NovaString *b) {
  if (!a || !b) return false;
  if (a->length != b->length) return false;
  return memcmp(a->data, b->data, a->length) == 0;
}

bool nova_string_equals_cstr(const NovaString *str, const char *cstr) {
  if (!str || !cstr) return false;
  return strcmp(str->data, cstr) == 0;
}

int nova_string_compare(const NovaString *a, const NovaString *b) {
  if (!a || !b) return 0;
  return strcmp(a->data, b->data);
}

// ═══════════════════════════════════════════════════════════════════════════
// SEARCH
// ═══════════════════════════════════════════════════════════════════════════

int64_t nova_string_find(const NovaString *str, const char *pattern) {
  if (!str || !pattern) return -1;
  
  const char *found = strstr(str->data, pattern);
  if (!found) return -1;
  
  return found - str->data;
}

int64_t nova_string_find_char(const NovaString *str, char c) {
  if (!str) return -1;
  
  const char *found = strchr(str->data, c);
  if (!found) return -1;
  
  return found - str->data;
}

bool nova_string_starts_with(const NovaString *str, const char *prefix) {
  if (!str || !prefix) return false;
  
  size_t prefix_len = strlen(prefix);
  if (prefix_len > str->length) return false;
  
  return memcmp(str->data, prefix, prefix_len) == 0;
}

bool nova_string_ends_with(const NovaString *str, const char *suffix) {
  if (!str || !suffix) return false;
  
  size_t suffix_len = strlen(suffix);
  if (suffix_len > str->length) return false;
  
  return memcmp(str->data + str->length - suffix_len, suffix, suffix_len) == 0;
}

bool nova_string_contains(const NovaString *str, const char *pattern) {
  return nova_string_find(str, pattern) >= 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// SUBSTRING
// ═══════════════════════════════════════════════════════════════════════════

NovaString *nova_string_substring(const NovaString *str, size_t start, size_t len) {
  if (!str || start >= str->length) return nova_string_new();
  
  if (start + len > str->length) {
    len = str->length - start;
  }
  
  return nova_string_from_bytes(str->data + start, len);
}

NovaString *nova_string_slice(const NovaString *str, size_t start, size_t end) {
  if (!str || start >= str->length) return nova_string_new();
  
  if (end > str->length) end = str->length;
  if (end <= start) return nova_string_new();
  
  return nova_string_from_bytes(str->data + start, end - start);
}

// ═══════════════════════════════════════════════════════════════════════════
// SPLIT
// ═══════════════════════════════════════════════════════════════════════════

NovaString **nova_string_split(const NovaString *str, const char *delimiter, size_t *count) {
  if (!str || !delimiter || !count) {
    *count = 0;
    return NULL;
  }
  
  size_t capacity = 8;
  NovaString **result = (NovaString **)malloc(capacity * sizeof(NovaString*));
  *count = 0;
  
  size_t delim_len = strlen(delimiter);
  const char *start = str->data;
  const char *end = str->data;
  
  while (*end) {
    if (strncmp(end, delimiter, delim_len) == 0) {
      // Found delimiter
      if (*count >= capacity) {
        capacity *= 2;
        result = (NovaString **)realloc(result, capacity * sizeof(NovaString*));
      }
      result[(*count)++] = nova_string_from_bytes(start, end - start);
      end += delim_len;
      start = end;
    } else {
      end++;
    }
  }
  
  // Last part
  if (*count >= capacity) {
    result = (NovaString **)realloc(result, (capacity + 1) * sizeof(NovaString*));
  }
  result[(*count)++] = nova_string_from_bytes(start, end - start);
  
  return result;
}

NovaString **nova_string_lines(const NovaString *str, size_t *count) {
  return nova_string_split(str, "\n", count);
}

// ═══════════════════════════════════════════════════════════════════════════
// TRANSFORM
// ═══════════════════════════════════════════════════════════════════════════

NovaString *nova_string_to_upper(const NovaString *str) {
  if (!str) return NULL;
  
  NovaString *result = nova_string_clone(str);
  for (size_t i = 0; i < result->length; i++) {
    result->data[i] = toupper(result->data[i]);
  }
  
  return result;
}

NovaString *nova_string_to_lower(const NovaString *str) {
  if (!str) return NULL;
  
  NovaString *result = nova_string_clone(str);
  for (size_t i = 0; i < result->length; i++) {
    result->data[i] = tolower(result->data[i]);
  }
  
  return result;
}

NovaString *nova_string_trim(const NovaString *str) {
  if (!str || str->length == 0) return nova_string_new();
  
  size_t start = 0;
  while (start < str->length && isspace(str->data[start])) {
    start++;
  }
  
  size_t end = str->length;
  while (end > start && isspace(str->data[end - 1])) {
    end--;
  }
  
  return nova_string_from_bytes(str->data + start, end - start);
}

NovaString *nova_string_trim_start(const NovaString *str) {
  if (!str) return nova_string_new();
  
  size_t start = 0;
  while (start < str->length && isspace(str->data[start])) {
    start++;
  }
  
  return nova_string_from_bytes(str->data + start, str->length - start);
}

NovaString *nova_string_trim_end(const NovaString *str) {
  if (!str) return nova_string_new();
  
  size_t end = str->length;
  while (end > 0 && isspace(str->data[end - 1])) {
    end--;
  }
  
  return nova_string_from_bytes(str->data, end);
}

NovaString *nova_string_replace(const NovaString *str, const char *from, const char *to) {
  if (!str || !from || !to) return nova_string_clone(str);
  
  NovaString *result = nova_string_new();
  size_t from_len = strlen(from);
  size_t to_len = strlen(to);
  
  const char *current = str->data;
  const char *found;
  
  while ((found = strstr(current, from)) != NULL) {
    // Copy before match
    nova_string_append_bytes(result, current, found - current);
    // Append replacement
    nova_string_append_bytes(result, to, to_len);
    current = found + from_len;
  }
  
  // Copy remainder
  nova_string_append(result, current);
  
  return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// JOIN
// ═══════════════════════════════════════════════════════════════════════════

NovaString *nova_string_join(NovaString **strings, size_t count, const char *separator) {
  if (!strings || count == 0) return nova_string_new();
  
  NovaString *result = nova_string_new();
  
  for (size_t i = 0; i < count; i++) {
    if (i > 0 && separator) {
      nova_string_append(result, separator);
    }
    nova_string_append_string(result, strings[i]);
  }
  
  return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// CONVERSION
// ═══════════════════════════════════════════════════════════════════════════

int64_t nova_string_to_i64(const NovaString *str) {
  if (!str) return 0;
  return strtoll(str->data, NULL, 10);
}

double nova_string_to_f64(const NovaString *str) {
  if (!str) return 0.0;
  return strtod(str->data, NULL);
}

NovaString *nova_string_from_i64(int64_t value) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
  return nova_string_from_cstr(buffer);
}

NovaString *nova_string_from_f64(double value) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%g", value);
  return nova_string_from_cstr(buffer);
}

NovaString *nova_string_format(const char *fmt, ...) {
  if (!fmt) return nova_string_new();
  
  va_list args;
  va_start(args, fmt);
  
  // Calculate needed size
  va_list args_copy;
  va_copy(args_copy, args);
  int needed = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);
  
  if (needed < 0) {
    va_end(args);
    return nova_string_new();
  }
  
  NovaString *str = nova_string_with_capacity(needed + 1);
  vsnprintf(str->data, needed + 1, fmt, args);
  str->length = needed;
  
  va_end(args);
  return str;
}

// ═══════════════════════════════════════════════════════════════════════════
// UTF-8 SUPPORT
// ═══════════════════════════════════════════════════════════════════════════

size_t nova_string_char_count(const NovaString *str) {
  if (!str) return 0;
  
  size_t count = 0;
  const unsigned char *p = (const unsigned char *)str->data;
  
  while (*p) {
    if ((*p & 0x80) == 0) {
      // ASCII
      p++;
    } else if ((*p & 0xE0) == 0xC0) {
      // 2-byte
      p += 2;
    } else if ((*p & 0xF0) == 0xE0) {
      // 3-byte
      p += 3;
    } else if ((*p & 0xF8) == 0xF0) {
      // 4-byte
      p += 4;
    } else {
      // Invalid
      p++;
    }
    count++;
  }
  
  return count;
}

bool nova_string_is_valid_utf8(const NovaString *str) {
  if (!str) return true;
  
  const unsigned char *p = (const unsigned char *)str->data;
  
  while (*p) {
    if ((*p & 0x80) == 0) {
      p++;
    } else if ((*p & 0xE0) == 0xC0) {
      if ((p[1] & 0xC0) != 0x80) return false;
      p += 2;
    } else if ((*p & 0xF0) == 0xE0) {
      if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return false;
      p += 3;
    } else if ((*p & 0xF8) == 0xF0) {
      if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) return false;
      p += 4;
    } else {
      return false;
    }
  }
  
  return true;
}
