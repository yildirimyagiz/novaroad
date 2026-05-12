/**
 * Nova I/O and Formatting Library
 * Printf-style formatting and I/O utilities
 */

#ifndef NOVA_IO_FMT_H
#define NOVA_IO_FMT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

// Forward declaration
typedef struct NovaString NovaString;

// ═══════════════════════════════════════════════════════════════════════════
// FORMATTER
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *buffer;
  size_t length;
  size_t capacity;
} NovaFormatter;

// Formatter lifecycle
NovaFormatter *nova_formatter_new(void);
void nova_formatter_destroy(NovaFormatter *fmt);
void nova_formatter_clear(NovaFormatter *fmt);

// Format operations
void nova_formatter_write_str(NovaFormatter *fmt, const char *str);
void nova_formatter_write_char(NovaFormatter *fmt, char c);
void nova_formatter_write_i64(NovaFormatter *fmt, int64_t value);
void nova_formatter_write_u64(NovaFormatter *fmt, uint64_t value);
void nova_formatter_write_f64(NovaFormatter *fmt, double value);
void nova_formatter_write_bool(NovaFormatter *fmt, bool value);
void nova_formatter_write_ptr(NovaFormatter *fmt, const void *ptr);

// Format with options
void nova_formatter_write_i64_width(NovaFormatter *fmt, int64_t value, size_t width, char fill);
void nova_formatter_write_f64_precision(NovaFormatter *fmt, double value, size_t precision);

// Get result
const char *nova_formatter_as_str(const NovaFormatter *fmt);
NovaString *nova_formatter_to_string(const NovaFormatter *fmt);

// ═══════════════════════════════════════════════════════════════════════════
// FORMAT STRING
// ═══════════════════════════════════════════════════════════════════════════

// Format a string with placeholders
// Supports: {}, {:d}, {:x}, {:b}, {:f}, {:s}, etc.
char *nova_format(const char *format_str, ...);
char *nova_vformat(const char *format_str, va_list args);

// Format to existing buffer
size_t nova_format_to(char *buffer, size_t size, const char *format_str, ...);

// ═══════════════════════════════════════════════════════════════════════════
// PRINT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Print to stdout
void nova_print(const char *str);
void nova_println(const char *str);
void nova_printf(const char *format, ...);

// Print to stderr
void nova_eprint(const char *str);
void nova_eprintln(const char *str);
void nova_eprintf(const char *format, ...);

// Print values
void nova_print_i64(int64_t value);
void nova_print_u64(uint64_t value);
void nova_print_f64(double value);
void nova_print_bool(bool value);
void nova_print_char(char c);

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG PRINT
// ═══════════════════════════════════════════════════════════════════════════

// Debug print with file:line info
void nova_debug_print(const char *file, int line, const char *format, ...);

#define NOVA_DEBUG(...) nova_debug_print(__FILE__, __LINE__, __VA_ARGS__)

// ═══════════════════════════════════════════════════════════════════════════
// INPUT FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Read a line from stdin
char *nova_read_line(void);
NovaString *nova_read_line_string(void);

// Read until EOF
char *nova_read_all(void);

// Parse input
int64_t nova_parse_i64(const char *str);
double nova_parse_f64(const char *str);
bool nova_parse_bool(const char *str);

// ═══════════════════════════════════════════════════════════════════════════
// FILE I/O
// ═══════════════════════════════════════════════════════════════════════════

// Read entire file
char *nova_read_file(const char *path);
bool nova_write_file(const char *path, const char *content);

// Append to file
bool nova_append_file(const char *path, const char *content);

// Check file existence
bool nova_file_exists(const char *path);

// ═══════════════════════════════════════════════════════════════════════════
// STRING CONVERSION
// ═══════════════════════════════════════════════════════════════════════════

// Convert values to string
char *nova_i64_to_str(int64_t value);
char *nova_u64_to_str(uint64_t value);
char *nova_f64_to_str(double value);
char *nova_bool_to_str(bool value);
char *nova_char_to_str(char value);

// With formatting options
char *nova_i64_to_str_base(int64_t value, int base);  // base 2, 8, 10, 16
char *nova_f64_to_str_precision(double value, int precision);

#endif // NOVA_IO_FMT_H
