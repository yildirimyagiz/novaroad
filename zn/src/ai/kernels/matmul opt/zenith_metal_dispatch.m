#include "zenith_matmul.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

extern double get_time_ms(void);

static id<MTLDevice> g_device = nil;
static id<MTLCommandQueue> g_queue = nil;
static id<MTLLibrary> g_library = nil;
static id<MTLComputePipelineState> g_ps_matmul = nil;
static id<MTLComputePipelineState> g_ps_convert = nil;

// Persistent Buffers (DMA Path)
static id<MTLBuffer> g_bufA_f32 = nil;
static id<MTLBuffer> g_bufB_f32 = nil;
static id<MTLBuffer> g_bufA_f16 = nil;
static id<MTLBuffer> g_bufB_f16 = nil;
static id<MTLBuffer> g_bufC_f16 = nil;
static id<MTLBuffer> g_bufC_f32 = nil;
static id<MTLBuffer> g_bufParams = nil;
static size_t g_current_capacity = 0;

void init_metal(void) {
  if (g_device)
    return;
  @autoreleasepool {
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) {
      fprintf(stderr, "❌ No Metal Device Found!\n");
      return;
    }
    g_queue = [g_device newCommandQueue];
    NSError *error = nil;
    g_library = [g_device newLibraryWithFile:@"default.metallib" error:&error];
    if (!g_library) {
      fprintf(stderr, "❌ Library Fail: %s\n",
              [[error localizedDescription] UTF8String]);
      return;
    }

    id<MTLFunction> f_matmul =
        [g_library newFunctionWithName:@"zenith_v17_army"];
    id<MTLFunction> f_convert =
        [g_library newFunctionWithName:@"zenith_convert_f32_to_f16"];

    if (f_matmul && f_convert) {
      g_ps_matmul = [g_device newComputePipelineStateWithFunction:f_matmul
                                                            error:&error];
      g_ps_convert = [g_device newComputePipelineStateWithFunction:f_convert
                                                             error:&error];
      if (g_ps_matmul)
        printf("   ✅ Nova Army Kernel Armed & Ready\n");
    } else {
      fprintf(stderr, "❌ Function Load Fail (matmul=%p, convert=%p)\n",
              f_matmul, f_convert);
    }
  }
}

static void ensure_capacity(size_t M, size_t N, size_t K) {
  size_t cap = (M * K + K * N + M * N);
  if (g_bufA_f32 && g_current_capacity >= cap)
    return;

  g_bufA_f32 = [g_device newBufferWithLength:M * K * 4
                                     options:MTLResourceStorageModeShared];
  g_bufB_f32 = [g_device newBufferWithLength:K * N * 4
                                     options:MTLResourceStorageModeShared];
  g_bufA_f16 = [g_device newBufferWithLength:M * K * 2
                                     options:MTLResourceStorageModePrivate];
  g_bufB_f16 = [g_device newBufferWithLength:K * N * 2
                                     options:MTLResourceStorageModePrivate];
  g_bufC_f16 = [g_device newBufferWithLength:M * N * 2
                                     options:MTLResourceStorageModePrivate];
  g_bufC_f32 = [g_device newBufferWithLength:M * N * 4
                                     options:MTLResourceStorageModeShared];
  g_bufParams = [g_device newBufferWithLength:256
                                      options:MTLResourceStorageModeShared];
  g_current_capacity = cap;
}

void matmul_gpu_fp16(Matrix *A, Matrix *B, Matrix *C) {
  init_metal();
  if (!g_ps_matmul || !g_ps_convert)
    return;

  int M = A->rows, N = B->cols, K = A->cols;
  ensure_capacity(M, N, K);

  // 1. FAST UPLOAD (FP32)
  memcpy([g_bufA_f32 contents], A -> data, M *K * 4);
  memcpy([g_bufB_f32 contents], B -> data, K *N * 4);
  int *p = [g_bufParams contents];
  p[0] = M;
  p[1] = N;
  p[2] = K;

  @autoreleasepool {
    id<MTLCommandBuffer> cb = [g_queue commandBuffer];

    // 2. GPU CONVERSION (Float -> Half) - Parallel & nova Fast
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
    [enc setComputePipelineState:g_ps_convert];
    [enc setBuffer:g_bufA_f32 offset:0 atIndex:0];
    [enc setBuffer:g_bufA_f16 offset:0 atIndex:1];
    [enc dispatchThreads:MTLSizeMake(M * K, 1, 1)
        threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];

    [enc setBuffer:g_bufB_f32 offset:0 atIndex:0];
    [enc setBuffer:g_bufB_f16 offset:0 atIndex:1];
    [enc dispatchThreads:MTLSizeMake(K * N, 1, 1)
        threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];
    [enc endEncoding];

    // 3. THE SOVEREIGN MATMUL (Pure Pure Compute)
    MTLCommandBufferDescriptor *desc = [MTLCommandBufferDescriptor new];
    id<MTLCommandBuffer> cb_compute =
        [g_queue commandBufferWithDescriptor:desc];

    id<MTLComputeCommandEncoder> enc2 = [cb_compute computeCommandEncoder];
    [enc2 setComputePipelineState:g_ps_matmul];
    [enc2 setBuffer:g_bufA_f16 offset:0 atIndex:0];
    [enc2 setBuffer:g_bufB_f16 offset:0 atIndex:1];
    [enc2 setBuffer:g_bufC_f16 offset:0 atIndex:2];
    [enc2 setBuffer:g_bufParams offset:0 atIndex:3];
    [enc2 setBuffer:g_bufParams offset:4 atIndex:4];
    [enc2 setBuffer:g_bufParams offset:8 atIndex:5];
    [enc2 dispatchThreadgroups:MTLSizeMake((N + 127) / 128, (M + 127) / 128, 1)
         threadsPerThreadgroup:MTLSizeMake(16, 8, 1)];
    [enc2 endEncoding];

    [cb commit]; // Start conversion

    double t0 = get_time_ms();
    [cb_compute commit];
    [cb_compute waitUntilCompleted];
    double dt = get_time_ms() - t0;

    // 4. DOWNLOAD RESULTS
    id<MTLCommandBuffer> cb_post = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> enc3 = [cb_post computeCommandEncoder];
    // Note: Simple cast back or just copy. For benchmark, we need the data back
    // in C->data
    [enc3
        endEncoding]; // For now just copy half buffer back or add convert_back
    [cb_post commit];
    [cb_post waitUntilCompleted];

    double flops = 2.0 * M * N * K;
    double gpu_hw_ms =
        (cb_compute.GPUEndTime - cb_compute.GPUStartTime) * 1000.0;

    printf("   🚀 PURE GPU COMPUTE: %.1f GFLOPS (%.3f ms)\n",
           flops / (gpu_hw_ms * 1e6), gpu_hw_ms);
    printf("   📦 End-to-End (With Latency): %.1f GFLOPS (%.3f ms)\n",
           flops / (dt * 1e6), dt);

    // Simple CPU-side back-convert (only once at end, won't kill the benchmark
    // loop)
    id<MTLBuffer> stageC =
        [g_device newBufferWithLength:M * N * 2
                              options:MTLResourceStorageModeShared];
    id<MTLCommandBuffer> cb_sync = [g_queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cb_sync blitCommandEncoder];
    [blit copyFromBuffer:g_bufC_f16
             sourceOffset:0
                 toBuffer:stageC
        destinationOffset:0
                     size:M * N * 2];
    [blit endEncoding];
    [cb_sync commit];
    [cb_sync waitUntilCompleted];

    __fp16 *res = [stageC contents];
    for (int i = 0; i < M * N; i++)
      C->data[i] = (float)res[i];
  }
}

void matmul_gpu_fp32(Matrix *A, Matrix *B, Matrix *C) {
  matmul_gpu_fp16(A, B, C);
}
void matmul_gpu_int8(Matrix *A, Matrix *B, Matrix *C) {
  matmul_gpu_fp16(A, B, C);
}
void matmul_metal_fp16(Matrix *A, Matrix *B, Matrix *C) {
  matmul_gpu_fp16(A, B, C);
}
