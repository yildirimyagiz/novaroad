#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// Self-Contained Reference Matrix Multiplication Implementation
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int rows;
    int cols;
    float *data;
} Matrix;

// Create a matrix
Matrix* matrix_create(int rows, int cols) {
    Matrix *m = (Matrix*)malloc(sizeof(Matrix));
    m->rows = rows;
    m->cols = cols;
    m->data = (float*)calloc(rows * cols, sizeof(float));
    return m;
}

// Free a matrix
void matrix_free(Matrix *m) {
    if (m) {
        free(m->data);
        free(m);
    }
}

// Set matrix element
void matrix_set(Matrix *m, int i, int j, float val) {
    m->data[i * m->cols + j] = val;
}

// Get matrix element
float matrix_get(Matrix *m, int i, int j) {
    return m->data[i * m->cols + j];
}

// Reference implementation: naive matmul
void matmul_reference(Matrix *A, Matrix *B, Matrix *C) {
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < B->cols; j++) {
            float sum = 0.0f;
            for (int k = 0; k < A->cols; k++) {
                sum += matrix_get(A, i, k) * matrix_get(B, k, j);
            }
            matrix_set(C, i, j, sum);
        }
    }
}

// Optimized matmul with blocking
void matmul_blocked(Matrix *A, Matrix *B, Matrix *C) {
    int block_size = 32;
    
    for (int ii = 0; ii < A->rows; ii += block_size) {
        for (int jj = 0; jj < B->cols; jj += block_size) {
            for (int kk = 0; kk < A->cols; kk += block_size) {
                int i_end = (ii + block_size > A->rows) ? A->rows : ii + block_size;
                int j_end = (jj + block_size > B->cols) ? B->cols : jj + block_size;
                int k_end = (kk + block_size > A->cols) ? A->cols : kk + block_size;
                
                for (int i = ii; i < i_end; i++) {
                    for (int j = jj; j < j_end; j++) {
                        float sum = matrix_get(C, i, j);
                        for (int k = kk; k < k_end; k++) {
                            sum += matrix_get(A, i, k) * matrix_get(B, k, j);
                        }
                        matrix_set(C, i, j, sum);
                    }
                }
            }
        }
    }
}

// Print matrix (for small matrices)
void matrix_print(Matrix *m, const char *name) {
    if (m->rows <= 4 && m->cols <= 4) {
        printf("%s (%d x %d):\n", name, m->rows, m->cols);
        for (int i = 0; i < m->rows; i++) {
            for (int j = 0; j < m->cols; j++) {
                printf("%.2f ", matrix_get(m, i, j));
            }
            printf("\n");
        }
    }
}

// Compare two matrices
int matrix_equal(Matrix *A, Matrix *B, float tolerance) {
    if (A->rows != B->rows || A->cols != B->cols) {
        return 0;
    }
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < A->cols; j++) {
            float a = matrix_get(A, i, j);
            float b = matrix_get(B, i, j);
            if (fabsf(a - b) > tolerance) {
                printf("  Mismatch at [%d,%d]: %.6f vs %.6f (diff=%.6f)\n", 
                       i, j, a, b, fabsf(a - b));
                return 0;
            }
        }
    }
    return 1;
}

// Get wall clock time in seconds
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// ═══════════════════════════════════════════════════════════════════════════
// Tests
// ═══════════════════════════════════════════════════════════════════════════

int test_identity_multiply() {
    printf("\n[TEST] Identity Matrix Multiplication\n");
    printf("─────────────────────────────────────\n");
    
    // Create matrices: A=[1,0;0,1], B=[2,3;4,5], C=A*B should equal B
    Matrix *A = matrix_create(2, 2);
    Matrix *B = matrix_create(2, 2);
    Matrix *C = matrix_create(2, 2);
    
    // A = identity matrix
    matrix_set(A, 0, 0, 1.0f);
    matrix_set(A, 0, 1, 0.0f);
    matrix_set(A, 1, 0, 0.0f);
    matrix_set(A, 1, 1, 1.0f);
    
    // B = [[2, 3], [4, 5]]
    matrix_set(B, 0, 0, 2.0f);
    matrix_set(B, 0, 1, 3.0f);
    matrix_set(B, 1, 0, 4.0f);
    matrix_set(B, 1, 1, 5.0f);
    
    matmul_reference(A, B, C);
    
    matrix_print(A, "A (Identity)");
    matrix_print(B, "B");
    matrix_print(C, "C = A*B");
    
    // Verify C equals B
    int passed = matrix_equal(B, C, 1e-5f);
    printf("✓ Identity multiply: %s\n", passed ? "PASS" : "FAIL");
    
    matrix_free(A);
    matrix_free(B);
    matrix_free(C);
    
    return passed;
}

int test_matmul_correctness() {
    printf("\n[TEST] Matrix Multiplication Correctness\n");
    printf("───────────────────────────────────────\n");
    
    // Test: A=[2,3], B=[4;5], C=A*B = [2*4+3*5] = [23]
    Matrix *A = matrix_create(1, 2);
    Matrix *B = matrix_create(2, 1);
    Matrix *C_ref = matrix_create(1, 1);
    Matrix *C_opt = matrix_create(1, 1);
    
    matrix_set(A, 0, 0, 2.0f);
    matrix_set(A, 0, 1, 3.0f);
    matrix_set(B, 0, 0, 4.0f);
    matrix_set(B, 1, 0, 5.0f);
    
    matmul_reference(A, B, C_ref);
    
    float expected = 2.0f * 4.0f + 3.0f * 5.0f; // 23
    printf("A = [2, 3]\n");
    printf("B = [4; 5]\n");
    printf("Expected C = %.1f\n", expected);
    printf("Computed C = %.1f\n", matrix_get(C_ref, 0, 0));
    
    int passed = fabsf(matrix_get(C_ref, 0, 0) - expected) < 1e-5f;
    printf("✓ Correctness: %s\n", passed ? "PASS" : "FAIL");
    
    matrix_free(A);
    matrix_free(B);
    matrix_free(C_ref);
    matrix_free(C_opt);
    
    return passed;
}

int test_matmul_performance() {
    printf("\n[TEST] Matrix Multiplication Performance (64x64)\n");
    printf("─────────────────────────────────────────────\n");
    
    int size = 64;
    Matrix *A = matrix_create(size, size);
    Matrix *B = matrix_create(size, size);
    Matrix *C = matrix_create(size, size);
    
    // Fill with random-like values
    for (int i = 0; i < size * size; i++) {
        A->data[i] = (float)(i % 100) / 100.0f;
        B->data[i] = (float)((i * 7) % 100) / 100.0f;
    }
    
    // Time reference implementation
    double t0 = get_time();
    matmul_reference(A, B, C);
    double t_ref = get_time() - t0;
    
    printf("Reference (naive):   %.4f ms\n", t_ref * 1000);
    
    // Reset C for blocked version
    memset(C->data, 0, size * size * sizeof(float));
    
    // Time blocked implementation
    t0 = get_time();
    matmul_blocked(A, B, C);
    double t_blocked = get_time() - t0;
    
    printf("Blocked (cache-opt): %.4f ms\n", t_blocked * 1000);
    
    // Calculate GFLOPS
    double ops = (double)size * size * size * 2;  // 2 ops per multiply-add
    double gflops = ops / t_blocked / 1e9;
    printf("Performance: %.2f GFLOPS\n", gflops);
    
    printf("✓ Performance: PASS (completed without error)\n");
    
    matrix_free(A);
    matrix_free(B);
    matrix_free(C);
    
    return 1;
}

int test_large_matmul() {
    printf("\n[TEST] Larger Matrix Multiplication (256x256)\n");
    printf("──────────────────────────────────────────\n");
    
    int size = 256;
    Matrix *A = matrix_create(size, size);
    Matrix *B = matrix_create(size, size);
    Matrix *C = matrix_create(size, size);
    
    // Fill with values
    for (int i = 0; i < size * size; i++) {
        A->data[i] = 0.1f;
        B->data[i] = 0.1f;
    }
    
    double t0 = get_time();
    matmul_blocked(A, B, C);
    double t_elapsed = get_time() - t0;
    
    printf("256x256 matmul: %.4f ms\n", t_elapsed * 1000);
    
    // Verify one element: C[0][0] should be 256 * 0.1 * 0.1 = 2.56
    float expected = 256.0f * 0.1f * 0.1f;
    float actual = matrix_get(C, 0, 0);
    int passed = fabsf(actual - expected) < 1e-5f;
    printf("C[0][0] expected: %.4f, actual: %.4f\n", expected, actual);
    printf("✓ Large matmul: %s\n", passed ? "PASS" : "FAIL");
    
    matrix_free(A);
    matrix_free(B);
    matrix_free(C);
    
    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════
// Main Test Suite
// ═══════════════════════════════════════════════════════════════════════════

int main(int argc, char *argv[]) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║         Nova Compute Test Suite (Self-Contained)              ║\n");
    printf("║            Matrix Multiplication Validation                   ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    int passed = 0;
    int total = 0;
    
    total++; if (test_identity_multiply()) passed++;
    total++; if (test_matmul_correctness()) passed++;
    total++; if (test_matmul_performance()) passed++;
    total++; if (test_large_matmul()) passed++;
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                        Test Results                           ║\n");
    printf("║  Passed: %d/%d                                                 ║\n", passed, total);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return (passed == total) ? 0 : 1;
}
