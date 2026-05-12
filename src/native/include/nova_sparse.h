/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_sparse.h - Sparse tensor (COO / CSR) and sparse-dense ops
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_SPARSE_H
#define NOVA_SPARSE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  NOVA_SPARSE_COO,
  NOVA_SPARSE_CSR
} NovaSparseFormat;

/**
 * COO: values[nnz], row_indices[nnz], col_indices[nnz], shape[2] = {rows, cols}.
 * CSR: values[nnz], col_indices[nnz], row_offsets[rows+1], shape[2] = {rows, cols}.
 */
typedef struct NovaSparseTensor {
  NovaSparseFormat format;
  int64_t shape[2];   /* rows, cols */
  int64_t nnz;

  float *values;
  int64_t *col_indices;   /* COO and CSR */
  int64_t *row_indices;   /* COO only */
  int64_t *row_offsets;   /* CSR only: row_offsets[i]..row_offsets[i+1]-1 */

  unsigned char own_values;
  unsigned char own_col_indices;
  unsigned char own_row_indices;
  unsigned char own_row_offsets;
} NovaSparseTensor;

/**
 * Create sparse tensor in COO format (caller provides buffers or NULL for allocate).
 * If values/row_indices/col_indices are NULL, they are allocated and must be freed by destroy.
 */
NovaSparseTensor *nova_sparse_create_coo(int64_t rows, int64_t cols,
                                             int64_t nnz,
                                             float *values,
                                             int64_t *row_indices,
                                             int64_t *col_indices);

/**
 * Create sparse tensor in CSR format. If values/col_indices/row_offsets are NULL, allocated by destroy.
 */
NovaSparseTensor *nova_sparse_create_csr(int64_t rows, int64_t cols,
                                             int64_t nnz,
                                             float *values,
                                             int64_t *col_indices,
                                             int64_t *row_offsets);

/**
 * Build COO sparse from dense matrix; entries with |x| <= threshold are skipped.
 * Returns new NovaSparseTensor (caller must nova_sparse_destroy).
 */
NovaSparseTensor *nova_sparse_from_dense(const float *dense,
                                              int64_t rows, int64_t cols,
                                              float threshold);

/**
 * Free sparse tensor and, if owned, its value/index buffers.
 */
void nova_sparse_destroy(NovaSparseTensor *s);

/**
 * Sparse-dense matmul: C = A * B where A is sparse [rows x K], B dense [K x cols], C dense [rows x cols].
 * C must be pre-allocated and can be zeroed before call (this function adds into C).
 */
void nova_sparse_dense_matmul(const NovaSparseTensor *A,
                                const float *B, int64_t B_cols,
                                float *C);

/**
 * Convert COO to CSR in-place or into a new tensor (caller gets new tensor to destroy).
 */
NovaSparseTensor *nova_sparse_coo_to_csr(const NovaSparseTensor *coo);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_SPARSE_H */
