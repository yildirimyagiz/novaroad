
#ifndef __APPLE__
#error "Metal backend requires macOS"
#endif

#include "nova_metal_gpu.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global State
static id<MTLDevice> g_device = nil;
static id<MTLCommandQueue> g_queue = nil;
static id<MTLComputePipelineState> g_ps_add = nil;
static id<MTLComputePipelineState> g_ps_mul = nil;
static id<MTLComputePipelineState> g_ps_matmul = nil;
static id<MTLComputePipelineState> g_ps_relu = nil;
static int g_initialized = 0;

// Buffer Management
typedef struct {
  id<MTLBuffer> buf;
  int64_t size;
} NovaMetalBuf;

static NovaMetalBuf *g_bufs = NULL;
static int g_bufs_cap = 0;
static int g_bufs_len = 0;

// Shader Source (naive kernels)
static NSString *const kMetalSource =
    @"using namespace metal;\n"
     "struct MatMulArgs { uint M; uint N; uint K; };\n"
     "kernel void vadd(device const float* a [[buffer(0)]], device const "
     "float* b [[buffer(1)]], device float* c [[buffer(2)]], constant uint& "
     "count [[buffer(3)]], uint gid [[thread_position_in_grid]]) {\n"
     "  if (gid < count) c[gid] = a[gid] + b[gid];\n"
     "}\n"
     "kernel void vmul(device const float* a [[buffer(0)]], device const "
     "float* b [[buffer(1)]], device float* c [[buffer(2)]], constant uint& "
     "count [[buffer(3)]], uint gid [[thread_position_in_grid]]) {\n"
     "  if (gid < count) c[gid] = a[gid] * b[gid];\n"
     "}\n"
     "kernel void matmul(device const float* A [[buffer(0)]], device const "
     "float* B [[buffer(1)]], device float* C [[buffer(2)]], constant "
     "MatMulArgs& args [[buffer(3)]], uint2 tid [[thread_position_in_grid]]) "
     "{\n"
     "  uint i = tid.y; uint j = tid.x;\n"
     "  if (i >= args.M || j >= args.N) return;\n"
     "  float sum = 0.0f;\n"
     "  uint baseA = i * args.K;\n"
     "  for (uint p = 0; p < args.K; p++) sum += A[baseA + p] * B[p * args.N + "
     "j];\n"
     "  C[i * args.N + j] = sum;\n"
     "}\n"
     "kernel void vrelu(device const float* a [[buffer(0)]], device float* c "
     "[[buffer(1)]], constant uint& count [[buffer(2)]], uint gid "
     "[[thread_position_in_grid]]) {\n"
     "  if (gid < count) c[gid] = max(0.0f, a[gid]);\n"
     "}\n";

static int ensure_buf_table(void);
static int64_t buf_handle_alloc(id<MTLBuffer> buf, int64_t size);
static id<MTLBuffer> buf_from_handle(int64_t h);
static void buf_free_handle(int64_t h);

// ============================================================================
// Initialization
// ============================================================================

int64_t nova_metal_init(void) {
  if (g_initialized)
    return 1;

  @autoreleasepool {
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device)
      return 0;

    g_queue = [g_device newCommandQueue];
    if (!g_queue)
      return 0;

    NSError *err = nil;
    id<MTLLibrary> lib =
        [g_device newLibraryWithSource:kMetalSource options:nil error:&err];
    if (!lib)
      return 0;

    id<MTLFunction> fnAdd = [lib newFunctionWithName:@"vadd"];
    id<MTLFunction> fnMul = [lib newFunctionWithName:@"vmul"];
    id<MTLFunction> fnMatMul = [lib newFunctionWithName:@"matmul"];
    id<MTLFunction> fnRelu = [lib newFunctionWithName:@"vrelu"];

    g_ps_add = [g_device newComputePipelineStateWithFunction:fnAdd error:&err];
    g_ps_mul = [g_device newComputePipelineStateWithFunction:fnMul error:&err];
    g_ps_matmul =
        [g_device newComputePipelineStateWithFunction:fnMatMul error:&err];
    g_ps_relu =
        [g_device newComputePipelineStateWithFunction:fnRelu error:&err];

    [fnAdd release];
    [fnMul release];
    [fnMatMul release];
    [fnRelu release];
    [lib release];

    if (!g_ps_add || !g_ps_mul || !g_ps_matmul)
      return 0;
  }

  g_initialized = 1;
  return 1;
}

int64_t nova_metal_get_device_count(void) { return g_initialized ? 1 : 0; }

void nova_metal_cleanup(void) {
  if (!g_initialized)
    return;

  @autoreleasepool {
    if (g_bufs) {
      for (int i = 0; i < g_bufs_len; i++) {
        if (g_bufs[i].buf)
          [g_bufs[i].buf release];
      }
      free(g_bufs);
      g_bufs = NULL;
      g_bufs_cap = 0;
      g_bufs_len = 0;
    }

    if (g_ps_add) {
      [g_ps_add release];
      g_ps_add = nil;
    }
    if (g_ps_mul) {
      [g_ps_mul release];
      g_ps_mul = nil;
    }
    if (g_ps_matmul) {
      [g_ps_matmul release];
      g_ps_matmul = nil;
    }
    if (g_ps_relu) {
      [g_ps_relu release];
      g_ps_relu = nil;
    }
    if (g_queue) {
      [g_queue release];
      g_queue = nil;
    }
    if (g_device) {
      [g_device release];
      g_device = nil;
    }
  }

  g_initialized = 0;
}

void nova_metal_print_info(void) {
  if (g_initialized && g_device)
    printf("Metal Device: %s\n", [[g_device name] UTF8String]);
}

// ============================================================================
// Buffer Ops
// ============================================================================

int64_t nova_metal_copy_to_gpu(const void *host_data, int64_t size) {
  if (!g_initialized)
    return 0;

  id<MTLBuffer> buf =
      [g_device newBufferWithBytes:host_data
                            length:(NSUInteger)size
                           options:MTLResourceStorageModeShared];
  if (!buf)
    return 0;

  int64_t h = buf_handle_alloc(buf, size);
  [buf release];
  return h;
}

int64_t nova_metal_copy_from_gpu(int64_t gpu_buffer, void *host_data,
                                 int64_t size) {
  if (!g_initialized)
    return -1;

  id<MTLBuffer> buf = buf_from_handle(gpu_buffer);
  if (!buf)
    return -1;

  memcpy(host_data, [buf contents], (size_t)size);
  return 0;
}

int64_t nova_metal_free_buffer(int64_t gpu_buffer) {
  if (!g_initialized)
    return -1;
  buf_free_handle(gpu_buffer);
  return 0;
}

// ============================================================================
// Compute Ops
// ============================================================================

static inline NSUInteger clamp_tg(NSUInteger desired,
                                  id<MTLComputePipelineState> ps) {
  NSUInteger maxTG = ps.maxTotalThreadsPerThreadgroup;
  return desired > maxTG ? maxTG : desired;
}

int64_t nova_metal_add(const float *a, const float *b, float *c,
                       int64_t count) {
  if (!g_initialized)
    return -1;

  @autoreleasepool {
    uint32_t cnt32 = (uint32_t)count;
    NSUInteger bytes = (NSUInteger)count * sizeof(float);

    id<MTLBuffer> ba =
        [g_device newBufferWithBytes:a
                              length:bytes
                             options:MTLResourceStorageModeShared];
    id<MTLBuffer> bb =
        [g_device newBufferWithBytes:b
                              length:bytes
                             options:MTLResourceStorageModeShared];
    id<MTLBuffer> bc =
        [g_device newBufferWithLength:bytes
                              options:MTLResourceStorageModeShared];
    id<MTLBuffer> bcnt =
        [g_device newBufferWithBytes:&cnt32
                              length:sizeof(uint32_t)
                             options:MTLResourceStorageModeShared];

    id<MTLCommandBuffer> cb = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];

    [enc setComputePipelineState:g_ps_add];
    [enc setBuffer:ba offset:0 atIndex:0];
    [enc setBuffer:bb offset:0 atIndex:1];
    [enc setBuffer:bc offset:0 atIndex:2];
    [enc setBuffer:bcnt offset:0 atIndex:3];

    MTLSize grid = MTLSizeMake((NSUInteger)count, 1, 1);
    NSUInteger tgs = clamp_tg(256, g_ps_add);
    MTLSize tg = MTLSizeMake(tgs, 1, 1);

    [enc dispatchThreads:grid threadsPerThreadgroup:tg];
    [enc endEncoding];
    [cb commit];
    [cb waitUntilCompleted];

    memcpy(c, [bc contents], (size_t)bytes);

    [ba release];
    [bb release];
    [bc release];
    [bcnt release];
  }
  return 0;
}

int64_t nova_metal_mul(const float *a, const float *b, float *c,
                       int64_t count) {
  if (!g_initialized)
    return -1;

  @autoreleasepool {
    uint32_t cnt32 = (uint32_t)count;
    NSUInteger bytes = (NSUInteger)count * sizeof(float);

    id<MTLBuffer> ba =
        [g_device newBufferWithBytes:a
                              length:bytes
                             options:MTLResourceStorageModeShared];
    id<MTLBuffer> bb =
        [g_device newBufferWithBytes:b
                              length:bytes
                             options:MTLResourceStorageModeShared];
    id<MTLBuffer> bc =
        [g_device newBufferWithLength:bytes
                              options:MTLResourceStorageModeShared];
    id<MTLBuffer> bcnt =
        [g_device newBufferWithBytes:&cnt32
                              length:sizeof(uint32_t)
                             options:MTLResourceStorageModeShared];

    id<MTLCommandBuffer> cb = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];

    [enc setComputePipelineState:g_ps_mul];
    [enc setBuffer:ba offset:0 atIndex:0];
    [enc setBuffer:bb offset:0 atIndex:1];
    [enc setBuffer:bc offset:0 atIndex:2];
    [enc setBuffer:bcnt offset:0 atIndex:3];

    MTLSize grid = MTLSizeMake((NSUInteger)count, 1, 1);
    NSUInteger tgs = clamp_tg(256, g_ps_mul);
    MTLSize tg = MTLSizeMake(tgs, 1, 1);

    [enc dispatchThreads:grid threadsPerThreadgroup:tg];
    [enc endEncoding];
    [cb commit];
    [cb waitUntilCompleted];

    memcpy(c, [bc contents], (size_t)bytes);

    [ba release];
    [bb release];
    [bc release];
    [bcnt release];
  }
  return 0;
}

typedef struct {
  uint32_t M, N, K;
} MatMulArgs;

int64_t nova_metal_matmul(const float *a, const float *b, float *c, int64_t m,
                          int64_t n, int64_t k) {
  if (!g_initialized)
    return -1;
  if (m <= 0 || n <= 0 || k <= 0)
    return -1;

  @autoreleasepool {
    if (m > UINT32_MAX || n > UINT32_MAX || k > UINT32_MAX)
      return -1;

    NSUInteger bytesA = (NSUInteger)(m * k) * sizeof(float);
    NSUInteger bytesB = (NSUInteger)(k * n) * sizeof(float);
    NSUInteger bytesC = (NSUInteger)(m * n) * sizeof(float);

    id<MTLBuffer> ba =
        [g_device newBufferWithBytes:a
                              length:bytesA
                             options:MTLResourceStorageModeShared];
    id<MTLBuffer> bb =
        [g_device newBufferWithBytes:b
                              length:bytesB
                             options:MTLResourceStorageModeShared];
    id<MTLBuffer> bc =
        [g_device newBufferWithLength:bytesC
                              options:MTLResourceStorageModeShared];

    MatMulArgs args = {(uint32_t)m, (uint32_t)n, (uint32_t)k};
    id<MTLBuffer> bargs =
        [g_device newBufferWithBytes:&args
                              length:sizeof(MatMulArgs)
                             options:MTLResourceStorageModeShared];

    id<MTLCommandBuffer> cb = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];

    [enc setComputePipelineState:g_ps_matmul];
    [enc setBuffer:ba offset:0 atIndex:0];
    [enc setBuffer:bb offset:0 atIndex:1];
    [enc setBuffer:bc offset:0 atIndex:2];
    [enc setBuffer:bargs offset:0 atIndex:3];

    MTLSize grid = MTLSizeMake((NSUInteger)n, (NSUInteger)m, 1);
    MTLSize tg = MTLSizeMake(16, 16, 1);

    [enc dispatchThreads:grid threadsPerThreadgroup:tg];
    [enc endEncoding];
    [cb commit];
    [cb waitUntilCompleted];

    memcpy(c, [bc contents], (size_t)bytesC);

    [ba release];
    [bb release];
    [bc release];
    [bargs release];
  }
  return 0;
}

int64_t nova_metal_relu(const float *in, float *out, int64_t n) {
  if (!g_initialized || !g_ps_relu)
    return -1;
  @autoreleasepool {
    uint32_t cnt32 = (uint32_t)n;
    NSUInteger bytes = (NSUInteger)n * sizeof(float);
    id<MTLBuffer> bin =
        [g_device newBufferWithBytes:in
                              length:bytes
                             options:MTLResourceStorageModeShared];
    id<MTLBuffer> bout =
        [g_device newBufferWithLength:bytes
                              options:MTLResourceStorageModeShared];
    id<MTLBuffer> bcnt =
        [g_device newBufferWithBytes:&cnt32
                              length:sizeof(uint32_t)
                             options:MTLResourceStorageModeShared];
    id<MTLCommandBuffer> cb = [g_queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
    [enc setComputePipelineState:g_ps_relu];
    [enc setBuffer:bin offset:0 atIndex:0];
    [enc setBuffer:bout offset:0 atIndex:1];
    [enc setBuffer:bcnt offset:0 atIndex:2];
    MTLSize grid = MTLSizeMake((NSUInteger)n, 1, 1);
    NSUInteger tgs = clamp_tg(256, g_ps_relu);
    [enc dispatchThreads:grid threadsPerThreadgroup:MTLSizeMake(tgs, 1, 1)];
    [enc endEncoding];
    [cb commit];
    [cb waitUntilCompleted];
    memcpy(out, [bout contents], (size_t)bytes);
    [bin release];
    [bout release];
    [bcnt release];
  }
  return 0;
}

int64_t nova_metal_softmax(const float *in, float *out, int64_t n) {
  (void)in;
  (void)out;
  (void)n;
  return -1; /* not implemented on Metal; dispatch falls back to CPU */
}

// LoRA Stubs
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

// ============================================================================
// Internal Helpers (buffer handle table)
// ============================================================================

static int ensure_buf_table(void) {
  if (!g_bufs) {
    g_bufs_cap = 128;
    g_bufs_len = 0;
    g_bufs = (NovaMetalBuf *)calloc((size_t)g_bufs_cap, sizeof(NovaMetalBuf));
  }
  return g_bufs ? 1 : 0;
}

static int64_t buf_handle_alloc(id<MTLBuffer> buf, int64_t size) {
  if (!ensure_buf_table())
    return 0;

  for (int i = 0; i < g_bufs_len; i++) {
    if (!g_bufs[i].buf) {
      g_bufs[i].buf = buf;
      [buf retain];
      g_bufs[i].size = size;
      return (int64_t)i + 1;
    }
  }

  if (g_bufs_len >= g_bufs_cap) {
    int new_cap = g_bufs_cap * 2;
    NovaMetalBuf *nb =
        (NovaMetalBuf *)realloc(g_bufs, (size_t)new_cap * sizeof(NovaMetalBuf));
    if (!nb)
      return 0;
    g_bufs = nb;
    memset(g_bufs + g_bufs_cap, 0,
           (size_t)(new_cap - g_bufs_cap) * sizeof(NovaMetalBuf));
    g_bufs_cap = new_cap;
  }

  g_bufs[g_bufs_len].buf = buf;
  [buf retain];
  g_bufs[g_bufs_len].size = size;
  g_bufs_len += 1;
  return (int64_t)g_bufs_len;
}

static id<MTLBuffer> buf_from_handle(int64_t h) {
  if (!g_bufs || h <= 0 || h > g_bufs_len)
    return nil;
  return g_bufs[h - 1].buf;
}

static void buf_free_handle(int64_t h) {
  if (!g_bufs || h <= 0 || h > g_bufs_len)
    return;
  if (g_bufs[h - 1].buf) {
    [g_bufs[h - 1].buf release];
    g_bufs[h - 1].buf = nil;
    g_bufs[h - 1].size = 0;
  }
}
