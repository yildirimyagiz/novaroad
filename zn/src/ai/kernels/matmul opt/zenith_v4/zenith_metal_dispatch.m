// ============================================================
// NOVA ULTRA MATMUL ENGINE v3.0 — Metal GPU Dispatch Backend
// Calls Metal kernels: FP32 coarsened, FP16 with FP32 acc, INT8 scaled
// ============================================================

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "zenith_matmul.h"

// ── Global Metal Objects ─────────────────────────────────────
static id<MTLDevice> g_device = nil;
static id<MTLCommandQueue> g_queue = nil;
static id<MTLLibrary> g_library = nil;

// Pipeline states
static id<MTLComputePipelineState> g_ps_fp32 = nil;
static id<MTLComputePipelineState> g_ps_fp16 = nil;
static id<MTLComputePipelineState> g_ps_int8 = nil;

// ── Init Metal ──────────────────────────────────────────────
void init_metal(void) {
    if (g_device) return;  // Already initialized

    g_device = MTLCreateSystemDefaultDevice();
    g_queue = [g_device newCommandQueue];

    // Load Metal library (compiled .metallib)
    NSString *libPath = [[NSBundle mainBundle] pathForResource:@"default" ofType:@"metallib"];
    if (!libPath) {
        // Fallback: current directory
        libPath = @"default.metallib";
    }
    NSError *error = nil;
    g_library = [g_device newLibraryWithFile:libPath error:&error];
    if (!g_library) {
        fprintf(stderr, "Failed to load Metal library: %s\n", [[error localizedDescription] UTF8String]);
        return;
    }

    // Create pipelines
    id<MTLFunction> func_fp32 = [g_library newFunctionWithName:@"matmul_fp32_coarsened"];
    g_ps_fp32 = [g_device newComputePipelineStateWithFunction:func_fp32 error:&error];
    if (!g_ps_fp32) fprintf(stderr, "FP32 pipeline error: %s\n", [[error localizedDescription] UTF8String]);

    id<MTLFunction> func_fp16 = [g_library newFunctionWithName:@"matmul_fp16_fp32acc"];
    g_ps_fp16 = [g_device newComputePipelineStateWithFunction:func_fp16 error:&error];
    if (!g_ps_fp16) fprintf(stderr, "FP16 pipeline error: %s\n", [[error localizedDescription] UTF8String]);

    id<MTLFunction> func_int8 = [g_library newFunctionWithName:@"matmul_int8_scaled"];
    g_ps_int8 = [g_device newComputePipelineStateWithFunction:func_int8 error:&error];
    if (!g_ps_int8) fprintf(stderr, "INT8 pipeline error: %s\n", [[error localizedDescription] UTF8String]);
}

// ── Dispatch Helper ──────────────────────────────────────────
static void dispatch_kernel(id<MTLComputePipelineState> ps,
                            id<MTLBuffer> bufA, id<MTLBuffer> bufB, id<MTLBuffer> bufC,
                            id<MTLBuffer> bufParams,
                            id<MTLBuffer> smA, id<MTLBuffer> smB,  // SMEM
                            int M, int N, int K) {
    id<MTLCommandBuffer> cmdBuf = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [cmdBuf computeCommandEncoder];

    [encoder setComputePipelineState:ps];
    [encoder setBuffer:bufA offset:0 atIndex:0];
    [encoder setBuffer:bufB offset:0 atIndex:1];
    [encoder setBuffer:bufC offset:0 atIndex:2];
    [encoder setBuffer:bufParams offset:0 atIndex:3];
    [encoder setBuffer:smA offset:0 atIndex:4];  // threadgroup
    [encoder setBuffer:smB offset:0 atIndex:5];

    // Threadgroup size: 16x16 for coarsening
    MTLSize tgSize = MTLSizeMake(16, 16, 1);
    MTLSize gridSize = MTLSizeMake((N + 63)/64 * 16, (M + 63)/64 * 16, 1);  // Wave-quantized

    [encoder dispatchThreads:gridSize threadsPerThreadgroup:tgSize];
    [encoder endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];
}

// ── GPU FP32 Dispatch ────────────────────────────────────────
void matmul_gpu_fp32(Matrix *A, Matrix *B, Matrix *C) {
    init_metal();
    if (!g_ps_fp32) return;

    int M = A->rows, N = B->cols, K = A->cols;

    // Allocate buffers
    id<MTLBuffer> bufA = [g_device newBufferWithBytes:A->data length:M*K*sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufB = [g_device newBufferWithBytes:B->data length:K*N*sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufC = [g_device newBufferWithBytes:C->data length:M*N*sizeof(float) options:MTLResourceStorageModeShared];

    // Params buffer: M, N, K
    int params[3] = {M, N, K};
    id<MTLBuffer> bufParams = [g_device newBufferWithBytes:params length:3*sizeof(int) options:MTLResourceStorageModeShared];

    // SMEM: BM*BK + BK*BN = 64*16*4 * 2 = 8KB each
    id<MTLBuffer> smA = [g_device newBufferWithLength:64*16*sizeof(float) options:MTLResourceStorageModePrivate];
    id<MTLBuffer> smB = [g_device newBufferWithLength:16*64*sizeof(float) options:MTLResourceStorageModePrivate];

    dispatch_kernel(g_ps_fp32, bufA, bufB, bufC, bufParams, smA, smB, M, N, K);

    // Copy back C
    memcpy(C->data, [bufC contents], M*N*sizeof(float));
}

// ── GPU FP16 Dispatch ────────────────────────────────────────
void matmul_gpu_fp16(Matrix *A, Matrix *B, Matrix *C) {
    init_metal();
    if (!g_ps_fp16) return;

    int M = A->rows, N = B->cols, K = A->cols;

    // Convert to half (FP16)
    __fp16 *A_half = (__fp16*)malloc(M*K*sizeof(__fp16));
    __fp16 *B_half = (__fp16*)malloc(K*N*sizeof(__fp16));
    for (int i=0; i<M*K; i++) A_half[i] = (__fp16)A->data[i];
    for (int i=0; i<K*N; i++) B_half[i] = (__fp16)B->data[i];

    id<MTLBuffer> bufA = [g_device newBufferWithBytes:A_half length:M*K*sizeof(__fp16) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufB = [g_device newBufferWithBytes:B_half length:K*N*sizeof(__fp16) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufC = [g_device newBufferWithBytes:C->data length:M*N*sizeof(float) options:MTLResourceStorageModeShared];

    int params[3] = {M, N, K};
    id<MTLBuffer> bufParams = [g_device newBufferWithBytes:params length:3*sizeof(int) options:MTLResourceStorageModeShared];

    // SMEM for FP16
    id<MTLBuffer> smA = [g_device newBufferWithLength:64*32*sizeof(__fp16) options:MTLResourceStorageModePrivate];
    id<MTLBuffer> smB = [g_device newBufferWithLength:32*64*sizeof(__fp16) options:MTLResourceStorageModePrivate];

    dispatch_kernel(g_ps_fp16, bufA, bufB, bufC, bufParams, smA, smB, M, N, K);

    memcpy(C->data, [bufC contents], M*N*sizeof(float));
    free(A_half);
    free(B_half);
}

// ── GPU INT8 Dispatch ────────────────────────────────────────
void matmul_gpu_int8(Matrix *A, Matrix *B, Matrix *C) {
    init_metal();
    if (!g_ps_int8) return;

    int M = A->rows, N = B->cols, K = A->cols;

    // Pack INT8 as char4 (4 bytes per element)
    int8_t *A_int8 = A->data_int8;
    int8_t *B_int8 = B->data_int8;
    // Assume A and B are already quantized

    id<MTLBuffer> bufA = [g_device newBufferWithBytes:A_int8 length:M*K*sizeof(int8_t) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufB = [g_device newBufferWithBytes:B_int8 length:K*N*sizeof(int8_t) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufC = [g_device newBufferWithBytes:C->data length:M*N*sizeof(float) options:MTLResourceStorageModeShared];

    int params[5] = {M, N, K, 0, 0};  // M,N,K,scaleA,scaleB
    float scales[2] = {A->scale, B->scale};
    id<MTLBuffer> bufParams = [g_device newBufferWithBytes:params length:3*sizeof(int) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufScales = [g_device newBufferWithBytes:scales length:2*sizeof(float) options:MTLResourceStorageModeShared];

    // SMEM for INT8
    id<MTLBuffer> smA = [g_device newBufferWithLength:64*64*sizeof(int8_t) options:MTLResourceStorageModePrivate];
    id<MTLBuffer> smB = [g_device newBufferWithLength:64*64*sizeof(int8_t) options:MTLResourceStorageModePrivate];

    // Custom dispatch for INT8 (different buffer indices)
    id<MTLCommandBuffer> cmdBuf = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [cmdBuf computeCommandEncoder];
    [encoder setComputePipelineState:g_ps_int8];
    [encoder setBuffer:bufA offset:0 atIndex:0];
    [encoder setBuffer:bufB offset:0 atIndex:1];
    [encoder setBuffer:bufC offset:0 atIndex:2];
    [encoder setBuffer:bufParams offset:0 atIndex:3];
    [encoder setBuffer:bufScales offset:0 atIndex:4];
    [encoder setBuffer:smA offset:0 atIndex:5];
    [encoder setBuffer:smB offset:0 atIndex:6];

    MTLSize tgSize = MTLSizeMake(16, 16, 1);
    MTLSize gridSize = MTLSizeMake((N + 63)/64 * 16, (M + 63)/64 * 16, 1);
    [encoder dispatchThreads:gridSize threadsPerThreadgroup:tgSize];
    [encoder endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];

    memcpy(C->data, [bufC contents], M*N*sizeof(float));
}
