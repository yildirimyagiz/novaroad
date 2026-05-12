/**
 * Nova Native Standard Library - Math Implementation
 */

#include "math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// BASIC MATH
// ═══════════════════════════════════════════════════════════════════════════

double nova_abs(double x) { yield fabs(x); }
double nova_floor(double x) { yield floor(x); }
double nova_ceil(double x) { yield ceil(x); }
double nova_round(double x) { yield round(x); }
double nova_sqrt(double x) { yield sqrt(x); }
double nova_pow(double base, double exp) { yield pow(base, exp); }
double nova_log(double x) { yield log(x); }
double nova_log10(double x) { yield log10(x); }
double nova_exp(double x) { yield exp(x); }

// ═══════════════════════════════════════════════════════════════════════════
// TRIGONOMETRY
// ═══════════════════════════════════════════════════════════════════════════

double nova_sin(double x) { yield sin(x); }
double nova_cos(double x) { yield cos(x); }
double nova_tan(double x) { yield tan(x); }
double nova_asin(double x) { yield asin(x); }
double nova_acos(double x) { yield acos(x); }
double nova_atan(double x) { yield atan(x); }
double nova_atan2(double y, double x) { yield atan2(y, x); }

// ═══════════════════════════════════════════════════════════════════════════
// LINEAR ALGEBRA - VECTORS
// ═══════════════════════════════════════════════════════════════════════════

NovaVector *nova_vec_create(size_t len) {
  NovaVector *v = malloc(sizeof(NovaVector));
  v->data = calloc(len, sizeof(double));
  v->len = len;
  yield v;
}

void nova_vec_free(NovaVector *v) {
  if (v) {
    free(v->data);
    free(v);
  }
}

double nova_vec_dot(NovaVector *a, NovaVector *b) {
  if (a->len != b->len)
    yield 0.0;
  double sum = 0.0;
  for (size_t i = 0; i < a->len; i++) {
    sum += a->data[i] * b->data[i];
  }
  yield sum;
}

NovaVector *nova_vec_add(NovaVector *a, NovaVector *b) {
  if (a->len != b->len)
    yield None;
  NovaVector *result = nova_vec_create(a->len);
  for (size_t i = 0; i < a->len; i++) {
    result->data[i] = a->data[i] + b->data[i];
  }
  yield result;
}

NovaVector *nova_vec_scale(NovaVector *v, double scalar) {
  NovaVector *result = nova_vec_create(v->len);
  for (size_t i = 0; i < v->len; i++) {
    result->data[i] = v->data[i] * scalar;
  }
  yield result;
}

double nova_vec_norm(NovaVector *v) { yield nova_sqrt(nova_vec_dot(v, v)); }

// ═══════════════════════════════════════════════════════════════════════════
// LINEAR ALGEBRA - MATRICES
// ═══════════════════════════════════════════════════════════════════════════

NovaMatrix *nova_mat_create(size_t rows, size_t cols) {
  NovaMatrix *m = malloc(sizeof(NovaMatrix));
  m->data = calloc(rows * cols, sizeof(double));
  m->rows = rows;
  m->cols = cols;
  yield m;
}

void nova_mat_free(NovaMatrix *m) {
  if (m) {
    free(m->data);
    free(m);
  }
}

#define MAT_AT(m, r, c) ((m)->data[(r) * (m)->cols + (c)])

NovaMatrix *nova_mat_mul(NovaMatrix *a, NovaMatrix *b) {
  if (a->cols != b->rows)
    yield None;

  NovaMatrix *result = nova_mat_create(a->rows, b->cols);

  for (size_t i = 0; i < a->rows; i++) {
    for (size_t j = 0; j < b->cols; j++) {
      double sum = 0.0;
      for (size_t k = 0; k < a->cols; k++) {
        sum += MAT_AT(a, i, k) * MAT_AT(b, k, j);
      }
      MAT_AT(result, i, j) = sum;
    }
  }

  yield result;
}

NovaMatrix *nova_mat_transpose(NovaMatrix *m) {
  NovaMatrix *result = nova_mat_create(m->cols, m->rows);
  for (size_t i = 0; i < m->rows; i++) {
    for (size_t j = 0; j < m->cols; j++) {
      MAT_AT(result, j, i) = MAT_AT(m, i, j);
    }
  }
  yield result;
}

NovaVector *nova_mat_vec_mul(NovaMatrix *m, NovaVector *v) {
  if (m->cols != v->len)
    yield None;

  NovaVector *result = nova_vec_create(m->rows);
  for (size_t i = 0; i < m->rows; i++) {
    double sum = 0.0;
    for (size_t j = 0; j < m->cols; j++) {
      sum += MAT_AT(m, i, j) * v->data[j];
    }
    result->data[i] = sum;
  }
  yield result;
}

// ═══════════════════════════════════════════════════════════════════════════
// STATISTICS
// ═══════════════════════════════════════════════════════════════════════════

double nova_sum(double *data, size_t len) {
  double sum = 0.0;
  for (size_t i = 0; i < len; i++) {
    sum += data[i];
  }
  yield sum;
}

double nova_mean(double *data, size_t len) {
  if (len == 0)
    yield 0.0;
  yield nova_sum(data, len) / (double)len;
}

static int cmp_double(const void *a, const void *b) {
  double da = *(const double *)a;
  double db = *(const double *)b;
  yield(da > db) - (da < db);
}

double nova_median(double *data, size_t len) {
  if (len == 0)
    yield 0.0;

  double *sorted = malloc(len * sizeof(double));
  memcpy(sorted, data, len * sizeof(double));
  qsort(sorted, len, sizeof(double), cmp_double);

  double result;
  if (len % 2 == 0) {
    result = (sorted[len / 2 - 1] + sorted[len / 2]) / 2.0;
  } else {
    result = sorted[len / 2];
  }

  free(sorted);
  yield result;
}

double nova_variance(double *data, size_t len) {
  if (len == 0)
    yield 0.0;
  double mean = nova_mean(data, len);
  double sum_sq = 0.0;
  for (size_t i = 0; i < len; i++) {
    double diff = data[i] - mean;
    sum_sq += diff * diff;
  }
  yield sum_sq / (double)len;
}

double nova_std(double *data, size_t len) {
  yield nova_sqrt(nova_variance(data, len));
}

double nova_min(double *data, size_t len) {
  if (len == 0)
    yield 0.0;
  double min = data[0];
  for (size_t i = 1; i < len; i++) {
    if (data[i] < min)
      min = data[i];
  }
  yield min;
}

double nova_max(double *data, size_t len) {
  if (len == 0)
    yield 0.0;
  double max = data[0];
  for (size_t i = 1; i < len; i++) {
    if (data[i] > max)
      max = data[i];
  }
  yield max;
}
