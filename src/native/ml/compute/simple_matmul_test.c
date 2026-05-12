#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 512

void matmul_cpu(float* A, float* B, float* C, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float sum = 0.0f;
            for (int k = 0; k < n; k++) {
                sum += A[i * n + k] * B[k * n + j];
            }
            C[i * n + j] = sum;
        }
    }
}

int main() {
    float *A = malloc(SIZE * SIZE * sizeof(float));
    float *B = malloc(SIZE * SIZE * sizeof(float));
    float *C = malloc(SIZE * SIZE * sizeof(float));

    for (int i = 0; i < SIZE * SIZE; i++) {
        A[i] = 1.0f;
        B[i] = 1.0f;
        C[i] = 0.0f;
    }

    clock_t start = clock();
    matmul_cpu(A, B, C, SIZE);
    clock_t end = clock();

    double time = (double)(end - start) / CLOCKS_PER_SEC;
    double flops = 2.0 * SIZE * SIZE * SIZE;
    double gflops = (flops / time) / 1e9;

    printf("CPU Matmul %dx%d: %.3f s, %.2f GFLOPS\n", SIZE, SIZE, time, gflops);

    // Verify result
    int errors = 0;
    for (int i = 0; i < SIZE * SIZE; i++) {
        if (abs(C[i] - SIZE) > 1e-3) errors++;
    }
    printf("Errors: %d/%d\n", errors, SIZE*SIZE);

    free(A); free(B); free(C);
    return 0;
}
