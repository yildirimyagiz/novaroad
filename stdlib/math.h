/**
 * Nova Native Standard Library - Math Module
 */

#ifndef NOVA_STDLIB_MATH_H
#define NOVA_STDLIB_MATH_H

#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// BASIC MATH
// ═══════════════════════════════════════════════════════════════════════════

double nova_abs(double x);
double nova_floor(double x);
double nova_ceil(double x);
double nova_round(double x);
double nova_sqrt(double x);
double nova_pow(double base, double exp);
double nova_log(double x);
double nova_log10(double x);
double nova_exp(double x);

// ═══════════════════════════════════════════════════════════════════════════
// TRIGONOMETRY
// ═══════════════════════════════════════════════════════════════════════════

double nova_sin(double x);
double nova_cos(double x);
double nova_tan(double x);
double nova_asin(double x);
double nova_acos(double x);
double nova_atan(double x);
double nova_atan2(double y, double x);

// ═══════════════════════════════════════════════════════════════════════════
// LINEAR ALGEBRA
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  double *data;
  size_t rows;
  size_t cols;
} NovaMatrix;

typedef struct {
  double *data;
  size_t len;
} NovaVector;

// Vector operations
NovaVector *nova_vec_create(size_t len);
void nova_vec_free(NovaVector *v);
double nova_vec_dot(NovaVector *a, NovaVector *b);
NovaVector *nova_vec_add(NovaVector *a, NovaVector *b);
NovaVector *nova_vec_scale(NovaVector *v, double scalar);
double nova_vec_norm(NovaVector *v);

// Matrix operations
NovaMatrix *nova_mat_create(size_t rows, size_t cols);
void nova_mat_free(NovaMatrix *m);
NovaMatrix *nova_mat_mul(NovaMatrix *a, NovaMatrix *b);
NovaMatrix *nova_mat_transpose(NovaMatrix *m);
NovaVector *nova_mat_vec_mul(NovaMatrix *m, NovaVector *v);

// ═══════════════════════════════════════════════════════════════════════════
// STATISTICS
// ═══════════════════════════════════════════════════════════════════════════

double nova_mean(double *data, size_t len);
double nova_median(double *data, size_t len);
double nova_std(double *data, size_t len);
double nova_variance(double *data, size_t len);
double nova_min(double *data, size_t len);
double nova_max(double *data, size_t len);
double nova_sum(double *data, size_t len);

#endif // NOVA_STDLIB_MATH_H
