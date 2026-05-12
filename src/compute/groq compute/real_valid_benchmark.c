#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

// Real, measurable benchmark for M1 Mac
// Fixes: Volatile results, proper timing, no dead code elimination

typedef struct {
    float *data;
    int rows, cols;
} Matrix;

// Get time in seconds (high precision)
double get_real_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void matmul_real(Matrix *A, Matrix *B, volatile Matrix *C) {
    // Volatile to prevent optimization
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < B->cols; j++) {
            volatile float sum = 0.0f;
            for (int k = 0; k < A->cols; k++) {
                sum += A->data[i * A->cols + k] * B->data[k * B->cols + j];
            }
            C->data[i * C->cols + j] = sum;
        }
    }
}

void benchmark_real_matmul(int size, int runs) {
    Matrix A = {malloc(size * size * sizeof(float)), size, size};
    Matrix B = {malloc(size * size * sizeof(float)), size, size};
    Matrix C = {malloc(size * size * sizeof(float)), size, size};

    // Fill with known values for verification
    for (int i = 0; i < size * size; i++) {
        A.data[i] = 1.0f;  // All ones
        B.data[i] = 1.0f;
    }

    double total_time = 0.0;
    for (int r = 0; r < runs; r++) {
        double start = get_real_time();
        matmul_real(&A, &B, (volatile Matrix*)&C);
        double end = get_real_time();
        total_time += (end - start);
    }

    double avg_time = total_time / runs;
    double flops = 2.0 * size * size * size;  // Multiply + add
    double gflops = (flops / avg_time) / 1e9;

    // Verify result: All elements should be size (since A and B are 1.0)
    int errors = 0;
    for (int i = 0; i < size * size; i++) {
        if (fabs(C.data[i] - size) > 1e-3) errors++;
    }

    printf("Real Matmul %dx%d (%d runs): Avg %.6f s, %.2f GFLOPS\n",
           size, size, runs, avg_time, gflops);
    printf("Verification: %d/%d correct elements\n", (size*size - errors), size*size);

    // Compare to theoretical M1 limits
    double m1_fp32_limit = 2.5;  // TFLOPS
    printf("M1 FP32 Theoretical: %.1f TFLOPS - Our: %.1f TFLOPS (%.1f%%)\n",
           m1_fp32_limit, gflops, (gflops / m1_fp32_limit) * 100);

    free(A.data); free(B.data); free(C.data);
}

int main() {
    printf("Real Measurable M1 Benchmark - No Simulation\n");
    printf("Volatile results, proper timing, verification\n\n");

    // Start small to verify
    benchmark_real_matmul(16, 10);   // Tiny, should be fast
    benchmark_real_matmul(64, 5);    // Small
    benchmark_real_matmul(128, 3);   // Medium

    printf("\nNow we have real numbers. What do you see?\n");
    return 0;
}
