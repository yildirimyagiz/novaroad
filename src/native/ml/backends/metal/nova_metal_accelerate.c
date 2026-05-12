/**
 * nova_metal_accelerate.c - Metal backend using Apple Accelerate (vDSP + BLAS)
 * 
 * This implementation uses Apple's Accelerate framework which provides:
 * - vDSP (vector digital signal processing)
 * - BLAS (Basic Linear Algebra Subroutines)
 * Optimized for Apple Silicon (ARM64)
 */

#ifdef __APPLE__

#define ACCELERATE_NEW_LAPACK
#include <Accelerate/Accelerate.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "nova_metal_gpu.h"

int64_t nova_metal_init(void) {
    return 1;
}

int64_t nova_metal_get_device_count(void) {
    return 1;
}

void nova_metal_cleanup(void) {
    /* no cleanup needed for Accelerate framework */
}

void nova_metal_print_info(void) {
    printf("Nova Metal Backend: Apple Accelerate (vDSP + BLAS)\n");
    printf("  Platform: macOS ARM64 (Apple Silicon)\n");
    printf("  Framework: Accelerate.framework\n");
    printf("  Components: vDSP, BLAS, LAPACK\n");
}

/**
 * Vector addition: c = a + b
 */
int64_t nova_metal_add(const float *a, const float *b, float *c, int64_t count) {
    if (!a || !b || !c || count <= 0) {
        return -1;
    }
    vDSP_vadd(a, 1, b, 1, c, 1, (vDSP_Length)count);
    return 0;
}

/**
 * Element-wise multiplication: c = a * b
 */
int64_t nova_metal_mul(const float *a, const float *b, float *c, int64_t count) {
    if (!a || !b || !c || count <= 0) {
        return -1;
    }
    vDSP_vmul(a, 1, b, 1, c, 1, (vDSP_Length)count);
    return 0;
}

/**
 * Dot product: result = a · b
 */
float nova_metal_dot(const float *a, const float *b, int64_t count) {
    if (!a || !b || count <= 0) {
        return 0.0f;
    }
    float result = 0.0f;
    vDSP_dotpr(a, 1, b, 1, &result, (vDSP_Length)count);
    return result;
}

/**
 * Matrix multiplication via cblas_sgemm: C = A * B
 * Assumes row-major storage:
 *   A is M×K, B is K×N, C is M×N
 */
int64_t nova_metal_matmul(const float *A, const float *B, float *C,
                           int64_t M, int64_t N, int64_t K) {
    if (!A || !B || !C || M <= 0 || N <= 0 || K <= 0) {
        return -1;
    }
    
    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                (int)M, (int)N, (int)K,
                1.0f,              /* alpha */
                A, (int)K,         /* A and lda */
                B, (int)N,         /* B and ldb */
                0.0f,              /* beta (C := alpha*A*B + beta*C) */
                C, (int)N);        /* C and ldc */
    return 0;
}

/**
 * ReLU activation: out = max(0, x)
 */
int64_t nova_metal_relu(const float *x, float *out, int64_t count) {
    if (!x || !out || count <= 0) {
        return -1;
    }
    
    float zero = 0.0f;
    /* vDSP_vthres: threshold operation - replaces values < threshold with threshold */
    vDSP_vthres(x, 1, &zero, out, 1, (vDSP_Length)count);
    return 0;
}

/**
 * Softmax: out = exp(x - max(x)) / sum(exp(x - max(x)))
 */
int64_t nova_metal_softmax(const float *x, float *out, int64_t count) {
    if (!x || !out || count <= 0) {
        return -1;
    }
    
    /* Step 1: Find maximum value for numerical stability */
    float max_val = 0.0f;
    vDSP_maxv(x, 1, &max_val, (vDSP_Length)count);
    
    /* Step 2: Subtract max from all values: out = x - max */
    float neg_max = -max_val;
    vDSP_vsadd(x, 1, &neg_max, out, 1, (vDSP_Length)count);
    
    /* Step 3: Apply exponential: out = exp(out) */
    int n = (int)count;
    vvexpf(out, out, &n);
    
    /* Step 4: Sum all exponentials */
    float sum = 0.0f;
    vDSP_sve(out, 1, &sum, (vDSP_Length)count);
    
    /* Step 5: Divide by sum: out = out / sum */
    if (sum > 1e-6f) {
        float inv_sum = 1.0f / sum;
        vDSP_vsmul(out, 1, &inv_sum, out, 1, (vDSP_Length)count);
    }
    
    return 0;
}

/* Stub implementations for functions not yet implemented via Accelerate */

int64_t nova_metal_copy_to_gpu(const void *host_data, int64_t size) {
    (void)host_data;
    (void)size;
    return -1;
}

int64_t nova_metal_copy_from_gpu(int64_t gpu_buffer, void *host_data, int64_t size) {
    (void)gpu_buffer;
    (void)host_data;
    (void)size;
    return -1;
}

int64_t nova_metal_free_buffer(int64_t gpu_buffer) {
    (void)gpu_buffer;
    return -1;
}

int64_t nova_metal_lora_forward(const float *i, const float *la,
                                 const float *lb, float *o, int64_t b,
                                 int64_t id, int64_t r, int64_t od) {
    (void)i;
    (void)la;
    (void)lb;
    (void)o;
    (void)b;
    (void)id;
    (void)r;
    (void)od;
    return -1;
}

int64_t nova_metal_lora_backward(const float *go, const float *i, float *gla,
                                  float *glb, int64_t b, int64_t id, int64_t r,
                                  int64_t od) {
    (void)go;
    (void)i;
    (void)gla;
    (void)glb;
    (void)b;
    (void)id;
    (void)r;
    (void)od;
    return -1;
}

#endif /* __APPLE__ */
