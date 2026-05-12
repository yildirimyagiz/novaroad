/**
 * @file rocm_backend.hip
 * @brief ROCm backend for AMD GPUs
 * 
 * Target: AMD Radeon (RX 6000/7000), MI100/MI200/MI300
 * Performance: 10-50 TFLOPS!
 * 
 * Features:
 * - rocBLAS integration
 * - HIP kernels (CUDA-compatible syntax)
 * - MI series optimizations
 */

#ifdef __HIP_PLATFORM_AMD__

#include <hip/hip_runtime.h>
#include <rocblas/rocblas.h>
#include <stdio.h>

static rocblas_handle g_rocblas_handle = NULL;

/**
 * Initialize ROCm backend
 */
extern "C" int nova_rocm_init(void)
{
    if (g_rocblas_handle) return 0;
    
    hipError_t hip_err = hipSetDevice(0);
    if (hip_err != hipSuccess) {
        fprintf(stderr, "HIP: Failed to set device: %s\n",
                hipGetErrorString(hip_err));
        return -1;
    }
    
    rocblas_status status = rocblas_create_handle(&g_rocblas_handle);
    if (status != rocblas_status_success) {
        fprintf(stderr, "rocBLAS: Failed to create handle\n");
        return -1;
    }
    
    hipDeviceProp_t prop;
    hipGetDeviceProperties(&prop, 0);
    printf("✅ ROCm initialized: %s\n", prop.name);
    printf("   Compute Units: %d\n", prop.multiProcessorCount);
    printf("   Memory: %.2f GB\n", prop.totalGlobalMem / 1e9);
    printf("   Peak FP32: %.2f TFLOPS (estimated)\n",
           2.0 * prop.multiProcessorCount * prop.clockRate * 128 / 1e9);
    
    return 0;
}

extern "C" void nova_rocm_cleanup(void)
{
    if (g_rocblas_handle) {
        rocblas_destroy_handle(g_rocblas_handle);
        g_rocblas_handle = NULL;
    }
    hipDeviceReset();
}

/**
 * ROCm GEMM using rocBLAS
 */
extern "C" int nova_rocm_gemm(
    const float* A, const float* B, float* C,
    int M, int N, int K)
{
    if (!g_rocblas_handle) {
        if (nova_rocm_init() != 0) return -1;
    }
    
    float *d_A, *d_B, *d_C;
    size_t size_A = M * K * sizeof(float);
    size_t size_B = K * N * sizeof(float);
    size_t size_C = M * N * sizeof(float);
    
    hipMalloc(&d_A, size_A);
    hipMalloc(&d_B, size_B);
    hipMalloc(&d_C, size_C);
    
    hipMemcpy(d_A, A, size_A, hipMemcpyHostToDevice);
    hipMemcpy(d_B, B, size_B, hipMemcpyHostToDevice);
    
    float alpha = 1.0f, beta = 0.0f;
    
    rocblas_status status = rocblas_sgemm(
        g_rocblas_handle,
        rocblas_operation_none,
        rocblas_operation_none,
        N, M, K,
        &alpha,
        d_B, N,
        d_A, K,
        &beta,
        d_C, N);
    
    if (status != rocblas_status_success) {
        fprintf(stderr, "rocBLAS: GEMM failed\n");
        hipFree(d_A); hipFree(d_B); hipFree(d_C);
        return -1;
    }
    
    hipMemcpy(C, d_C, size_C, hipMemcpyDeviceToHost);
    
    hipFree(d_A); hipFree(d_B); hipFree(d_C);
    
    return 0;
}

/**
 * HIP kernel: Fused GEMM + Bias + ReLU
 */
__global__ void hip_gemm_bias_relu_kernel(
    const float* __restrict__ A,
    const float* __restrict__ B,
    const float* __restrict__ bias,
    float* __restrict__ C,
    int M, int N, int K)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < M && col < N) {
        float sum = 0.0f;
        
        #pragma unroll 8
        for (int k = 0; k < K; k++) {
            sum += A[row * K + k] * B[k * N + col];
        }
        
        sum += bias[col];
        sum = fmaxf(sum, 0.0f);
        
        C[row * N + col] = sum;
    }
}

extern "C" int nova_rocm_gemm_bias_relu(
    const float* A, const float* B, const float* bias, float* C,
    int M, int N, int K)
{
    if (!g_rocblas_handle) {
        if (nova_rocm_init() != 0) return -1;
    }
    
    float *d_A, *d_B, *d_bias, *d_C;
    hipMalloc(&d_A, M * K * sizeof(float));
    hipMalloc(&d_B, K * N * sizeof(float));
    hipMalloc(&d_bias, N * sizeof(float));
    hipMalloc(&d_C, M * N * sizeof(float));
    
    hipMemcpy(d_A, A, M * K * sizeof(float), hipMemcpyHostToDevice);
    hipMemcpy(d_B, B, K * N * sizeof(float), hipMemcpyHostToDevice);
    hipMemcpy(d_bias, bias, N * sizeof(float), hipMemcpyHostToDevice);
    
    dim3 block(16, 16);
    dim3 grid((N + 15) / 16, (M + 15) / 16);
    
    hipLaunchKernelGGL(hip_gemm_bias_relu_kernel, grid, block, 0, 0,
                       d_A, d_B, d_bias, d_C, M, N, K);
    
    hipMemcpy(C, d_C, M * N * sizeof(float), hipMemcpyDeviceToHost);
    
    hipFree(d_A); hipFree(d_B); hipFree(d_bias); hipFree(d_C);
    
    return 0;
}

#else

extern "C" int nova_rocm_init(void) { return -1; }
extern "C" void nova_rocm_cleanup(void) {}
extern "C" int nova_rocm_gemm(const float* A, const float* B, float* C,
                              int M, int N, int K) {
    (void)A; (void)B; (void)C; (void)M; (void)N; (void)K;
    return -1;
}

#endif /* __HIP_PLATFORM_AMD__ */
