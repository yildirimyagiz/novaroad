/**
 * @file cuda_kernels.cu
 * @brief CUDA GPU kernels for Nova AI operations
 */

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>
#include <stdio.h>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error: %s\n", cudaGetErrorString(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define BLOCK_SIZE 256
#define TILE_SIZE 16

/* ReLU activation */
__global__ void cuda_relu_kernel(const float *x, float *out, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        out[idx] = fmaxf(0.0f, x[idx]);
    }
}

/* GELU activation (GPT-style) */
__global__ void cuda_gelu_kernel(const float *x, float *out, size_t n) {
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

/* Warp reduction */
__device__ float warp_reduce_sum(float val) {
    for (int offset = 16; offset > 0; offset /= 2) {
        val += __shfl_down_sync(0xffffffff, val, offset);
    }
    return val;
}

/* Matrix multiplication (tiled) */
__global__ void cuda_matmul_kernel(const float *A, const float *B, float *C,
                                   int M, int N, int K) {
    __shared__ float As[TILE_SIZE][TILE_SIZE];
    __shared__ float Bs[TILE_SIZE][TILE_SIZE];
    
    int row = blockIdx.y * TILE_SIZE + threadIdx.y;
    int col = blockIdx.x * TILE_SIZE + threadIdx.x;
    
    float sum = 0.0f;
    
    for (int t = 0; t < (K + TILE_SIZE - 1) / TILE_SIZE; t++) {
        if (row < M && t * TILE_SIZE + threadIdx.x < K)
            As[threadIdx.y][threadIdx.x] = A[row * K + t * TILE_SIZE + threadIdx.x];
        else
            As[threadIdx.y][threadIdx.x] = 0.0f;
            
        if (col < N && t * TILE_SIZE + threadIdx.y < K)
            Bs[threadIdx.y][threadIdx.x] = B[(t * TILE_SIZE + threadIdx.y) * N + col];
        else
            Bs[threadIdx.y][threadIdx.x] = 0.0f;
            
        __syncthreads();
        
        for (int k = 0; k < TILE_SIZE; k++) {
            sum += As[threadIdx.y][k] * Bs[k][threadIdx.x];
        }
        
        __syncthreads();
    }
    
    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}

/* C interface */
extern "C" {

void nova_cuda_relu(const float *x, float *out, size_t n) {
    int blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    cuda_relu_kernel<<<blocks, BLOCK_SIZE>>>(x, out, n);
    CUDA_CHECK(cudaDeviceSynchronize());
}

void nova_cuda_gelu(const float *x, float *out, size_t n) {
    int blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    cuda_gelu_kernel<<<blocks, BLOCK_SIZE>>>(x, out, n);
    CUDA_CHECK(cudaDeviceSynchronize());
}

void nova_cuda_matmul(const float *A, const float *B, float *C,
                     int M, int N, int K) {
    dim3 blocks((N + TILE_SIZE - 1) / TILE_SIZE, 
                (M + TILE_SIZE - 1) / TILE_SIZE);
    dim3 threads(TILE_SIZE, TILE_SIZE);
    
    cuda_matmul_kernel<<<blocks, threads>>>(A, B, C, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());
}

}
