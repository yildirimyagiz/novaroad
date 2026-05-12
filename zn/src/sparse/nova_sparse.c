/**
 * nova_sparse.c - Sparse COO/CSR and sparse-dense matmul
 */

#include "../../include/nova_sparse.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

NovaSparseTensor *nova_sparse_create_coo(int64_t rows, int64_t cols,
                                             int64_t nnz,
                                             float *values,
                                             int64_t *row_indices,
                                             int64_t *col_indices) {
  NovaSparseTensor *s = (NovaSparseTensor *)calloc(1, sizeof(NovaSparseTensor));
  if (!s) return NULL;
  s->format = NOVA_SPARSE_COO;
  s->shape[0] = rows;
  s->shape[1] = cols;
  s->nnz = nnz;

  int own_vals = (values == NULL);
  int own_ri = (row_indices == NULL);
  int own_ci = (col_indices == NULL);
  if (own_vals) values = (float *)malloc((size_t)nnz * sizeof(float));
  if (own_ri) row_indices = (int64_t *)malloc((size_t)nnz * sizeof(int64_t));
  if (own_ci) col_indices = (int64_t *)malloc((size_t)nnz * sizeof(int64_t));
  if ((own_vals && !values) || (own_ri && !row_indices) || (own_ci && !col_indices)) {
    if (own_vals && values) free(values);
    if (own_ri && row_indices) free(row_indices);
    if (own_ci && col_indices) free(col_indices);
    free(s);
    return NULL;
  }
  s->values = values;
  s->row_indices = row_indices;
  s->col_indices = col_indices;
  s->row_offsets = NULL;
  s->own_values = (unsigned char)own_vals;
  s->own_row_indices = (unsigned char)own_ri;
  s->own_col_indices = (unsigned char)own_ci;
  s->own_row_offsets = 0;
  return s;
}

NovaSparseTensor *nova_sparse_create_csr(int64_t rows, int64_t cols,
                                             int64_t nnz,
                                             float *values,
                                             int64_t *col_indices,
                                             int64_t *row_offsets) {
  NovaSparseTensor *s = (NovaSparseTensor *)calloc(1, sizeof(NovaSparseTensor));
  if (!s) return NULL;
  s->format = NOVA_SPARSE_CSR;
  s->shape[0] = rows;
  s->shape[1] = cols;
  s->nnz = nnz;
  int own_vals = (values == NULL);
  int own_ci = (col_indices == NULL);
  int own_ro = (row_offsets == NULL);
  if (own_vals) values = (float *)malloc((size_t)nnz * sizeof(float));
  if (own_ci) col_indices = (int64_t *)malloc((size_t)nnz * sizeof(int64_t));
  if (own_ro) row_offsets = (int64_t *)malloc((size_t)(rows + 1) * sizeof(int64_t));
  if ((own_vals && !values) || (own_ci && !col_indices) || (own_ro && !row_offsets)) {
    if (own_vals && values) free(values);
    if (own_ci && col_indices) free(col_indices);
    if (own_ro && row_offsets) free(row_offsets);
    free(s);
    return NULL;
  }
  s->values = values;
  s->col_indices = col_indices;
  s->row_offsets = row_offsets;
  s->row_indices = NULL;
  s->own_values = (unsigned char)own_vals;
  s->own_col_indices = (unsigned char)own_ci;
  s->own_row_offsets = (unsigned char)own_ro;
  s->own_row_indices = 0;
  return s;
}

NovaSparseTensor *nova_sparse_from_dense(const float *dense,
                                              int64_t rows, int64_t cols,
                                              float threshold) {
  int64_t nnz = 0;
  for (int64_t i = 0; i < rows * cols; i++) {
    if (fabsf(dense[i]) > threshold) nnz++;
  }
  NovaSparseTensor *s = nova_sparse_create_coo(rows, cols, nnz, NULL, NULL, NULL);
  if (!s) return NULL;
  int64_t p = 0;
  for (int64_t r = 0; r < rows; r++) {
    for (int64_t c = 0; c < cols; c++) {
      float v = dense[r * cols + c];
      if (fabsf(v) > threshold) {
        s->values[p] = v;
        s->row_indices[p] = r;
        s->col_indices[p] = c;
        p++;
      }
    }
  }
  return s;
}

void nova_sparse_destroy(NovaSparseTensor *s) {
  if (!s) return;
  if (s->own_values && s->values) free(s->values);
  if (s->own_row_indices && s->row_indices) free(s->row_indices);
  if (s->own_col_indices && s->col_indices) free(s->col_indices);
  if (s->own_row_offsets && s->row_offsets) free(s->row_offsets);
  free(s);
}

static void sparse_dense_matmul_coo(const NovaSparseTensor *A,
                                    const float *B, int64_t B_cols,
                                    float *C) {
  for (int64_t p = 0; p < A->nnz; p++) {
    int64_t i = A->row_indices[p];
    int64_t j = A->col_indices[p];
    float v = A->values[p];
    const float *b_row = B + j * B_cols;
    float *c_row = C + i * B_cols;
    for (int64_t k = 0; k < B_cols; k++)
      c_row[k] += v * b_row[k];
  }
}

static void sparse_dense_matmul_csr(const NovaSparseTensor *A,
                                    const float *B, int64_t B_cols,
                                    float *C) {
  int64_t rows = A->shape[0];
  for (int64_t i = 0; i < rows; i++) {
    int64_t start = A->row_offsets[i];
    int64_t end = A->row_offsets[i + 1];
    float *c_row = C + i * B_cols;
    for (int64_t p = start; p < end; p++) {
      int64_t j = A->col_indices[p];
      float v = A->values[p];
      const float *b_row = B + j * B_cols;
      for (int64_t k = 0; k < B_cols; k++)
        c_row[k] += v * b_row[k];
    }
  }
}

void nova_sparse_dense_matmul(const NovaSparseTensor *A,
                                const float *B, int64_t B_cols,
                                float *C) {
  if (!A || !B || !C) return;
  if (A->format == NOVA_SPARSE_COO)
    sparse_dense_matmul_coo(A, B, B_cols, C);
  else
    sparse_dense_matmul_csr(A, B, B_cols, C);
}

NovaSparseTensor *nova_sparse_coo_to_csr(const NovaSparseTensor *coo) {
  if (!coo || coo->format != NOVA_SPARSE_COO) return NULL;
  int64_t rows = coo->shape[0];
  int64_t cols = coo->shape[1];
  int64_t nnz = coo->nnz;

  int64_t *row_offsets = (int64_t *)calloc((size_t)(rows + 1), sizeof(int64_t));
  if (!row_offsets) return NULL;
  for (int64_t p = 0; p < nnz; p++) {
    int64_t r = coo->row_indices[p];
    if (r >= 0 && r < rows) row_offsets[r + 1]++;
  }
  for (int64_t r = 0; r < rows; r++)
    row_offsets[r + 1] += row_offsets[r];

  int64_t *pos = (int64_t *)malloc((size_t)rows * sizeof(int64_t));
  if (!pos) { free(row_offsets); return NULL; }
  memcpy(pos, row_offsets, (size_t)rows * sizeof(int64_t));

  float *values = (float *)malloc((size_t)nnz * sizeof(float));
  int64_t *col_indices = (int64_t *)malloc((size_t)nnz * sizeof(int64_t));
  if (!values || !col_indices) {
    free(row_offsets); free(pos); free(values); free(col_indices);
    return NULL;
  }
  for (int64_t p = 0; p < nnz; p++) {
    int64_t r = coo->row_indices[p];
    int64_t idx = pos[r]++;
    values[idx] = coo->values[p];
    col_indices[idx] = coo->col_indices[p];
  }
  free(pos);

  NovaSparseTensor *csr = nova_sparse_create_csr(rows, cols, nnz,
                                                      values, col_indices, row_offsets);
  if (!csr) {
    free(values); free(col_indices); free(row_offsets);
    return NULL;
  }
  /* We allocated values/col_indices/row_offsets; csr must free them on destroy */
  csr->own_values = 1;
  csr->own_col_indices = 1;
  csr->own_row_offsets = 1;
  return csr;
}
