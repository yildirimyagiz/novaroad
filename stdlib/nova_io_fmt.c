/**
 * Nova I/O and Formatting Implementation
 */

#include "nova_io_fmt.h"
#include "nova_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ═══════════════════════════════════════════════════════════════════════════
// FORMATTER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

#define INITIAL_FMT_CAPACITY 256

NovaFormatter *nova_formatter_new(void) {
  NovaFormatter *fmt = (NovaFormatter *)malloc(sizeof(NovaFormatter));
  if (!fmt) return NULL;
  
  fmt->capacity = INITIAL_FMT_CAPACITY;
  fmt->buffer = (char *)malloc(fmt->capacity);
  fmt->length = 0;
  fmt->buffer[0] = '\0';
  
  return fmt;
}

void nova_formatter_destroy(NovaFormatter *fmt) {
  if (!fmt) return;
  free(fmt->buffer);
  free(fmt);
}

void nova_formatter_clear(NovaFormatter *fmt) {
  if (!fmt) return;
  fmt->length = 0;
  fmt->buffer[0] = '\0';
}

static void nova_formatter_ensure_capacity(NovaFormatter *fmt, size_t needed) {
  if (!fmt) return;
  
  if (needed <= fmt->capacity) return;
  
  size_t new_capacity = fmt->capacity * 2;
  while (new_capacity < needed) {
    new_capacity *= 2;
  }
  
  fmt->buffer = (char *)realloc(fmt->buffer, new_capacity);
  fmt->capacity = new_capacity;
}

void nova_formatter_write_str(NovaFormatter *fmt, const char *str) {
  if (!fmt || !str) return;
  
  size_t len = strlen(str);
  nova_formatter_ensure_capacity(fmt, fmt->length + len + 1);
  memcpy(fmt->buffer + fmt->length, str, len);
  fmt->length += len;
  fmt->buffer[fmt->length] = '\0';
}

void nova_formatter_write_char(NovaFormatter *fmt, char c) {
  if (!fmt) return;
  
  nova_formatter_ensure_capacity(fmt, fmt->length + 2);
  fmt->buffer[fmt->length++] = c;
  fmt->buffer[fmt->length] = '\0';
}

void nova_formatter_write_i64(NovaFormatter *fmt, int64_t value) {
  if (!fmt) return;
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
  nova_formatter_write_str(fmt, buffer);
}

void nova_formatter_write_u64(NovaFormatter *fmt, uint64_t value) {
  if (!fmt) return;
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)value);
  nova_formatter_write_str(fmt, buffer);
}

void nova_formatter_write_f64(NovaFormatter *fmt, double value) {
  if (!fmt) return;
  
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%g", value);
  nova_formatter_write_str(fmt, buffer);
}

void nova_formatter_write_bool(NovaFormatter *fmt, bool value) {
  if (!fmt) return;
  nova_formatter_write_str(fmt, value ? "true" : "false");
}

void nova_formatter_write_ptr(NovaFormatter *fmt, const void *ptr) {
  if (!fmt) return;
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%p", ptr);
  nova_formatter_write_str(fmt, buffer);
}

void nova_formatter_write_i64_width(NovaFormatter *fmt, int64_t value, size_t width, char fill) {
  if (!fmt) return;
  
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
  
  size_t len = strlen(buffer);
  if (len < width) {
    for (size_t i = 0; i < width - len; i++) {
      nova_formatter_write_char(fmt, fill);
    }
  }
  nova_formatter_write_str(fmt, buffer);
}

void nova_formatter_write_f64_precision(NovaFormatter *fmt, double value, size_t precision) {
  if (!fmt) return;
  
  char buffer[64];
  char format[16];
  snprintf(format, sizeof(format), "%%.%zuf", precision);
  snprintf(buffer, sizeof(buffer), format, value);
  nova_formatter_write_str(fmt, buffer);
}

const char *nova_formatter_as_str(const NovaFormatter *fmt) {
  return fmt ? fmt->buffer : "";
}

NovaString *nova_formatter_to_string(const NovaFormatter *fmt) {
  if (!fmt) return nova_string_new();
  return nova_string_from_bytes(fmt->buffer, fmt->length);
}

// ═══════════════════════════════════════════════════════════════════════════
// FORMAT STRING
// ═══════════════════════════════════════════════════════════════════════════

char *nova_vformat(const char *format_str, va_list args) {
  if (!format_str) return strdup("");
  
  NovaFormatter *fmt = nova_formatter_new();
  const char *p = format_str;
  
  while (*p) {
    if (*p == '{') {
      p++;
      if (*p == '{') {
        // Escaped brace
        nova_formatter_write_char(fmt, '{');
        p++;
      } else {
        // Format specifier
        char spec[32] = {0};
        size_t spec_len = 0;
        
        while (*p && *p != '}' && spec_len < sizeof(spec) - 1) {
          spec[spec_len++] = *p++;
        }
        
        if (*p == '}') p++;
        
        // Parse specifier
        if (spec_len == 0 || strcmp(spec, "d") == 0) {
          // Integer
          int64_t value = va_arg(args, int64_t);
          nova_formatter_write_i64(fmt, value);
        } else if (strcmp(spec, "x") == 0) {
          // Hex
          uint64_t value = va_arg(args, uint64_t);
          char buffer[32];
          snprintf(buffer, sizeof(buffer), "%llx", (unsigned long long)value);
          nova_formatter_write_str(fmt, buffer);
        } else if (strcmp(spec, "b") == 0) {
          // Binary
          uint64_t value = va_arg(args, uint64_t);
          char buffer[128];
          size_t idx = 0;
          for (int i = 63; i >= 0; i--) {
            if ((value >> i) & 1) {
              buffer[idx++] = '1';
            } else if (idx > 0) {
              buffer[idx++] = '0';
            }
          }
          if (idx == 0) buffer[idx++] = '0';
          buffer[idx] = '\0';
          nova_formatter_write_str(fmt, buffer);
        } else if (strcmp(spec, "f") == 0) {
          // Float
          double value = va_arg(args, double);
          nova_formatter_write_f64(fmt, value);
        } else if (strcmp(spec, "s") == 0) {
          // String
          const char *str = va_arg(args, const char*);
          nova_formatter_write_str(fmt, str ? str : "(null)");
        } else if (strcmp(spec, "c") == 0) {
          // Char
          char c = (char)va_arg(args, int);
          nova_formatter_write_char(fmt, c);
        } else if (strcmp(spec, "p") == 0) {
          // Pointer
          void *ptr = va_arg(args, void*);
          nova_formatter_write_ptr(fmt, ptr);
        } else if (strcmp(spec, "bool") == 0) {
          // Boolean
          bool value = va_arg(args, int);
          nova_formatter_write_bool(fmt, value);
        }
      }
    } else if (*p == '}') {
      p++;
      if (*p == '}') {
        nova_formatter_write_char(fmt, '}');
        p++;
      }
    } else {
      nova_formatter_write_char(fmt, *p++);
    }
  }
  
  char *result = strdup(fmt->buffer);
  nova_formatter_destroy(fmt);
  return result;
}

char *nova_format(const char *format_str, ...) {
  va_list args;
  va_start(args, format_str);
  char *result = nova_vformat(format_str, args);
  va_end(args);
  return result;
}

size_t nova_format_to(char *buffer, size_t size, const char *format_str, ...) {
  va_list args;
  va_start(args, format_str);
  char *result = nova_vformat(format_str, args);
  va_end(args);
  
  size_t len = strlen(result);
  if (len >= size) len = size - 1;
  memcpy(buffer, result, len);
  buffer[len] = '\0';
  
  free(result);
  return len;
}

// ═══════════════════════════════════════════════════════════════════════════
// PRINT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void nova_print(const char *str) {
  if (str) fputs(str, stdout);
}

void nova_println(const char *str) {
  if (str) fputs(str, stdout);
  putchar('\n');
  fflush(stdout);
}

void nova_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char *result = nova_vformat(format, args);
  va_end(args);
  
  fputs(result, stdout);
  free(result);
}

void nova_eprint(const char *str) {
  if (str) fputs(str, stderr);
}

void nova_eprintln(const char *str) {
  if (str) fputs(str, stderr);
  fputc('\n', stderr);
  fflush(stderr);
}

void nova_eprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char *result = nova_vformat(format, args);
  va_end(args);
  
  fputs(result, stderr);
  free(result);
}

void nova_print_i64(int64_t value) {
  printf("%lld", (long long)value);
  fflush(stdout);
}

void nova_print_u64(uint64_t value) {
  printf("%llu", (unsigned long long)value);
  fflush(stdout);
}

void nova_print_f64(double value) {
  printf("%g", value);
  fflush(stdout);
}

void nova_print_bool(bool value) {
  fputs(value ? "true" : "false", stdout);
  fflush(stdout);
}

void nova_print_char(char c) {
  putchar(c);
  fflush(stdout);
}

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG PRINT
// ═══════════════════════════════════════════════════════════════════════════

void nova_debug_print(const char *file, int line, const char *format, ...) {
  fprintf(stderr, "[DEBUG %s:%d] ", file, line);
  
  va_list args;
  va_start(args, format);
  char *result = nova_vformat(format, args);
  va_end(args);
  
  fputs(result, stderr);
  fputc('\n', stderr);
  fflush(stderr);
  
  free(result);
}

// ═══════════════════════════════════════════════════════════════════════════
// INPUT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

char *nova_read_line(void) {
  char buffer[1024];
  if (fgets(buffer, sizeof(buffer), stdin)) {
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
      buffer[len - 1] = '\0';
    }
    return strdup(buffer);
  }
  return NULL;
}

NovaString *nova_read_line_string(void) {
  char *line = nova_read_line();
  if (!line) return nova_string_new();
  
  NovaString *str = nova_string_from_cstr(line);
  free(line);
  return str;
}

char *nova_read_all(void) {
  size_t capacity = 1024;
  size_t length = 0;
  char *buffer = (char *)malloc(capacity);
  
  int c;
  while ((c = getchar()) != EOF) {
    if (length + 1 >= capacity) {
      capacity *= 2;
      buffer = (char *)realloc(buffer, capacity);
    }
    buffer[length++] = (char)c;
  }
  
  buffer[length] = '\0';
  return buffer;
}

int64_t nova_parse_i64(const char *str) {
  if (!str) return 0;
  return strtoll(str, NULL, 10);
}

double nova_parse_f64(const char *str) {
  if (!str) return 0.0;
  return strtod(str, NULL);
}

bool nova_parse_bool(const char *str) {
  if (!str) return false;
  return strcmp(str, "true") == 0 || strcmp(str, "1") == 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// FILE I/O
// ═══════════════════════════════════════════════════════════════════════════

char *nova_read_file(const char *path) {
  if (!path) return NULL;
  
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *content = (char *)malloc(size + 1);
  fread(content, 1, size, f);
  content[size] = '\0';
  
  fclose(f);
  return content;
}

bool nova_write_file(const char *path, const char *content) {
  if (!path || !content) return false;
  
  FILE *f = fopen(path, "wb");
  if (!f) return false;
  
  fputs(content, f);
  fclose(f);
  return true;
}

bool nova_append_file(const char *path, const char *content) {
  if (!path || !content) return false;
  
  FILE *f = fopen(path, "ab");
  if (!f) return false;
  
  fputs(content, f);
  fclose(f);
  return true;
}

bool nova_file_exists(const char *path) {
  if (!path) return false;
  
  FILE *f = fopen(path, "r");
  if (f) {
    fclose(f);
    return true;
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// STRING CONVERSION
// ═══════════════════════════════════════════════════════════════════════════

char *nova_i64_to_str(int64_t value) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
  return strdup(buffer);
}

char *nova_u64_to_str(uint64_t value) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)value);
  return strdup(buffer);
}

char *nova_f64_to_str(double value) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%g", value);
  return strdup(buffer);
}

char *nova_bool_to_str(bool value) {
  return strdup(value ? "true" : "false");
}

char *nova_char_to_str(char value) {
  char buffer[2] = {value, '\0'};
  return strdup(buffer);
}

char *nova_i64_to_str_base(int64_t value, int base) {
  char buffer[128];
  
  if (base == 2) {
    // Binary
    size_t idx = 0;
    uint64_t uvalue = (uint64_t)value;
    for (int i = 63; i >= 0; i--) {
      if ((uvalue >> i) & 1) {
        buffer[idx++] = '1';
      } else if (idx > 0) {
        buffer[idx++] = '0';
      }
    }
    if (idx == 0) buffer[idx++] = '0';
    buffer[idx] = '\0';
  } else if (base == 8) {
    snprintf(buffer, sizeof(buffer), "%llo", (unsigned long long)value);
  } else if (base == 16) {
    snprintf(buffer, sizeof(buffer), "%llx", (unsigned long long)value);
  } else {
    snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
  }
  
  return strdup(buffer);
}

char *nova_f64_to_str_precision(double value, int precision) {
  char buffer[128];
  char format[16];
  snprintf(format, sizeof(format), "%%.%df", precision);
  snprintf(buffer, sizeof(buffer), format, value);
  return strdup(buffer);
}
