/**
 * Nova Built-in Functions
 * Core runtime functions available to all programs
 */

#ifndef NOVA_BUILTIN_H
#define NOVA_BUILTIN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Print functions
void nova_print_i32(int32_t value);
void nova_print_i64(int64_t value);
void nova_print_f32(float value);
void nova_print_f64(double value);
void nova_print_str(const char* str);
void nova_print_bool(bool value);
void nova_print_newline(void);

// String functions
size_t nova_str_len(const char* str);
char* nova_str_concat(const char* a, const char* b);
bool nova_str_eq(const char* a, const char* b);
char* nova_str_format(const char* fmt, ...);

// Range/iteration
typedef struct {
    int64_t start;
    int64_t end;
    int64_t step;
    int64_t current;
} NovaRange;

NovaRange* nova_range_new(int64_t start, int64_t end);
NovaRange* nova_range_new_step(int64_t start, int64_t end, int64_t step);
bool nova_range_has_next(NovaRange* range);
int64_t nova_range_next(NovaRange* range);
void nova_range_free(NovaRange* range);

// Math functions
int64_t nova_pow_i64(int64_t base, int64_t exp);
double nova_pow_f64(double base, double exp);
double nova_sqrt(double x);
double nova_abs(double x);

// Type conversions
int64_t nova_to_i64(const char* str);
double nova_to_f64(const char* str);
char* nova_to_str_i64(int64_t value);
char* nova_to_str_f64(double value);

#endif // NOVA_BUILTIN_H
