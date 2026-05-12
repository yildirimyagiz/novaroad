#ifndef ZENITH_CUDA_AVAILABLE
#error "This file requires CUDA. Define ZENITH_CUDA_AVAILABLE or compile with CUDA toolkit."
#endif

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>

// Include tensor header from correct location
#ifdef __cplusplus
extern "C" {
#endif

#include "../../../include/zenith_tensor.h"

#ifdef __cplusplus
}
#endif

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA CUDA KERNELS - GPU Acceleration
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define BLOCK_SIZE 256
#define TILE_SIZE 16

// ═══════════════════════════════════════════════════════════════════════════
// Activation Kernels
// ═══════════════════════════════════════════════════════════════════════════

__global__ void cuda_kernel_relu(const float *x, float *out, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        out[idx] = fmaxf(0.0f, x[idx]);
    }
}

__global__ void cuda_kernel_gelu(const float *x, float *out, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        float x_val = x[idx];
        const float sqrt_2_over_pi = 0.7978845608f;
        const float coeff = 0.044715f;
        float x_cubed = x_val * x_val * x_val;
        float inner = sqrt_2_over_pi * (x_val + coeff * x_cubed);
        out[idx] = 0.5f * x_val * (1.0f + tanhf(inner));
    }
}

__global__ void cuda_kernel_silu(const float *x, float *out, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        float x_val = x[idx];
        float sigmoid = 1.0f / (1.0f + expf(-x_val));
        out[idx] = x_val * sigmoid;
    }
}

__global__ void cuda_kernel_sigmoid(const float *x, float *out, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        out[idx] = 1.0f / (1.0f + expf(-x[idx]));
    }
}

__global__ void cuda_kernel_tanh_activation(const float *x, float *out, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        out[idx] = tanhf(x[idx]);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Softmax with Warp Reduction
// ═══════════════════════════════════════════════════════════════════════════

__device__ float warp_reduce_max(float val) {
    for (int offset = 16; offset > 0; offset /= 2) {
        val = fmaxf(val, __shfl_down_sync(0xffffffff, val, offset));
    }
    return val;
}

__device__ float warp_reduce_sum(float val) {
    for (int offset = 16; offset > 0; offset /= 2) {
        val += __shfl_down_sync(0xffffffff, val, offset);
    }
    return val;
}

__global__ void cuda_kernel_softmax(const float *x, float *out, size_t n) {
    __shared__ float shared_max;
    __shared__ float shared_sum;
    
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Find max
    float local_max = (idx < n) ? x[idx] : -INFINITY;
    local_max = warp_reduce_max(local_max);
    
    if (tid % 32 == 0) {
        atomicMaxFloat(&shared_max, local_max);
    }
    __syncthreads();
    
    // Compute exp and sum
    float exp_val = 0.0f;
    if (idx < n) {
        exp_val = expf(x[idx] - shared_max);
    }
    
    float local_sum = warp_reduce_sum(exp_val);
    if (tid % 32 == 0) {
        atomicAdd(&shared_sum, local_sum);
    }
    __syncthreads();
    
    // Normalize
    if (idx < n) {
        out[idx] = exp_val / shared_sum;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Element-wise Operations
// ═══════════════════════════════════════════════════════════════════════════

__global__ void cuda_kernel_add(const float *a, const float *b, float *c, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}

__global__ void cuda_kernel_mul(const float *a, const float *b, float *c, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] * b[idx];
    }
}

__global__ void cuda_kernel_scalar_mul(const float *a, float s, float *c, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] * s;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Matrix Multiplication (Tiled)
// ═══════════════════════════════════════════════════════════════════════════

__global__ void cuda_kernel_matmul_tiled(const float *A, const float *B, float *C,
                                         int M, int N, int K) {
    __shared__ float tile_A[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_B[TILE_SIZE][TILE_SIZE];
    
    int row = blockIdx.y * TILE_SIZE + threadIdx.y;
    int col = blockIdx.x * TILE_SIZE + threadIdx.x;
    
    float sum = 0.0f;
    
    // Loop over tiles
    for (int t = 0; t < (K + TILE_SIZE - 1) / TILE_SIZE; t++) {
        // Load tile from A
        int a_col = t * TILE_SIZE + threadIdx.x;
        if (row < M && a_col < K) {
            tile_A[threadIdx.y][threadIdx.x] = A[row * K + a_col];
        } else {
            tile_A[threadIdx.y][threadIdx.x] = 0.0f;
        }
        
        // Load tile from B
        int b_row = t * TILE_SIZE + threadIdx.y;
        if (b_row < K && col < N) {
            tile_B[threadIdx.y][threadIdx.x] = B[b_row * N + col];
        } else {
            tile_B[threadIdx.y][threadIdx.x] = 0.0f;
        }
        
        __syncthreads();
        
        // Compute partial dot product
        for (int k = 0; k < TILE_SIZE; k++) {
            sum += tile_A[threadIdx.y][k] * tile_B[k][threadIdx.x];
        }
        
        __syncthreads();
    }
    
    // Write result
    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Host Wrapper Functions
// ═══════════════════════════════════════════════════════════════════════════

extern "C" {

void zenith_cuda_gelu(ZenithTensor *x, ZenithTensor *out) {
    size_t n = x->total_elements;
    int blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    float *d_x, *d_out;
    CUDA_CHECK(cudaMalloc(&d_x, n * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out, n * sizeof(float)));
    
    CUDA_CHECK(cudaMemcpy(d_x, x->data, n * sizeof(float), cudaMemcpyHostToDevice));
    
    cuda_kernel_gelu<<<blocks, BLOCK_SIZE>>>(d_x, d_out, n);
    CUDA_CHECK(cudaGetLastError());
    
    CUDA_CHECK(cudaMemcpy(out->data, d_out, n * sizeof(float), cudaMemcpyDeviceToHost));
    
    cudaFree(d_x);
    cudaFree(d_out);
}

void zenith_cuda_silu(ZenithTensor *x, ZenithTensor *out) {
    size_t n = x->total_elements;
    int blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    float *d_x, *d_out;
    CUDA_CHECK(cudaMalloc(&d_x, n * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out, n * sizeof(float)));
    
    CUDA_CHECK(cudaMemcpy(d_x, x->data, n * sizeof(float), cudaMemcpyHostToDevice));
    
    cuda_kernel_silu<<<blocks, BLOCK_SIZE>>>(d_x, d_out, n);
    CUDA_CHECK(cudaGetLastError());
    
    CUDA_CHECK(cudaMemcpy(out->data, d_out, n * sizeof(float), cudaMemcpyDeviceToHost));
    
    cudaFree(d_x);
    cudaFree(d_out);
}

void zenith_cuda_relu(ZenithTensor *x, ZenithTensor *out) {
    size_t n = x->total_elements;
    int blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    float *d_x, *d_out;
    CUDA_CHECK(cudaMalloc(&d_x, n * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out, n * sizeof(float)));
    
    CUDA_CHECK(cudaMemcpy(d_x, x->data, n * sizeof(float), cudaMemcpyHostToDevice));
    
    cuda_kernel_relu<<<blocks, BLOCK_SIZE>>>(d_x, d_out, n);
    CUDA_CHECK(cudaGetLastError());
    
    CUDA_CHECK(cudaMemcpy(out->data, d_out, n * sizeof(float), cudaMemcpyDeviceToHost));
    
    cudaFree(d_x);
    cudaFree(d_out);
}

void zenith_cuda_matmul(ZenithTensor *A, ZenithTensor *B, ZenithTensor *C) {
    int M = A->shape[0];
    int K = A->shape[1];
    int N = B->shape[1];
    
    float *d_A, *d_B, *d_C;
    CUDA_CHECK(cudaMalloc(&d_A, M * K * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_B, K * N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_C, M * N * sizeof(float)));
    
    CUDA_CHECK(cudaMemcpy(d_A, A->data, M * K * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, B->data, K * N * sizeof(float), cudaMemcpyHostToDevice));
    
    dim3 block(TILE_SIZE, TILE_SIZE);
    dim3 grid((N + TILE_SIZE - 1) / TILE_SIZE, (M + TILE_SIZE - 1) / TILE_SIZE);
    
    cuda_kernel_matmul_tiled<<<grid, block>>>(d_A, d_B, d_C, M, N, K);
    CUDA_CHECK(cudaGetLastError());
    
    CUDA_CHECK(cudaMemcpy(C->data, d_C, M * N * sizeof(float), cudaMemcpyDeviceToHost));
    
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
}

} // extern "C"
