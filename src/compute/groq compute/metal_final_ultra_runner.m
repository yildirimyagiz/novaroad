#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <mach/mach_time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t M, N, K;
    uint32_t lda, ldb, ldc;
} MatmulParams;

static inline double now_seconds(void) {
    static mach_timebase_info_data_t tb;
    static uint64_t t0 = 0;
    if (!t0) { mach_timebase_info(&tb); t0 = mach_absolute_time(); }
    uint64_t t = mach_absolute_time() - t0;
    double ns = (double)t * (double)tb.numer / (double)tb.denom;
    return ns * 1e-9;
}

static void cpu_ref_matmul(const float *A, const float *B, float *C, int M, int N, int K) {
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double acc = 0.0;
            for (int k = 0; k < K; ++k) acc += (double)A[i*K + k] * (double)B[k*N + j];
            C[i*N + j] = (float)acc;
        }
    }
}

static int check(const float *C, const float *Ref, int n, float atol, float rtol) {
    int err = 0;
    for (int i = 0; i < n; ++i) {
        float a = C[i], b = Ref[i];
        float diff = fabsf(a - b);
        float tol = atol + rtol * fabsf(b);
        if (!(diff <= tol) && !(isnan(a) && isnan(b))) err++;
    }
    return err;
}

static void fill_rand(float *x, int n) {
    for (int i = 0; i < n; ++i) x[i] = (float)((rand() / (double)RAND_MAX) * 2.0 - 1.0);
}

static id<MTLComputePipelineState> make_pipeline(id<MTLDevice> dev, NSURL *libURL, NSString *fn) {
    NSError *err = nil;
    id<MTLLibrary> lib = [dev newLibraryWithURL:libURL error:&err];
    if (!lib) { fprintf(stderr, "Library load failed: %s\n", err.localizedDescription.UTF8String); exit(1); }
    id<MTLFunction> f = [lib newFunctionWithName:fn];
    if (!f) { fprintf(stderr, "Function not found: %s\n", fn.UTF8String); exit(1); }
    id<MTLComputePipelineState> pso = [dev newComputePipelineStateWithFunction:f error:&err];
    if (!pso) { fprintf(stderr, "PSO build failed: %s\n", err.localizedDescription.UTF8String); exit(1); }
    return pso;
}

static double run_case(id<MTLDevice> dev,
                       id<MTLCommandQueue> q,
                       id<MTLComputePipelineState> pso,
                       int M, int N, int K,
                       int warmup, int iters,
                       int *outErrors)
{
    const int aCount = M*K;
    const int bCount = K*N;
    const int cCount = M*N;

    float *A = (float *)malloc((size_t)aCount * sizeof(float));
    float *B = (float *)malloc((size_t)bCount * sizeof(float));
    float *Ref = (float *)malloc((size_t)cCount * sizeof(float));
    memset(Ref, 0, (size_t)cCount * sizeof(float));

    fill_rand(A, aCount);
    fill_rand(B, bCount);
    cpu_ref_matmul(A, B, Ref, M, N, K);

    id<MTLBuffer> bufA = [dev newBufferWithBytes:A length:(NSUInteger)aCount*sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufB = [dev newBufferWithBytes:B length:(NSUInteger)bCount*sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufC = [dev newBufferWithLength:(NSUInteger)cCount*sizeof(float) options:MTLResourceStorageModeShared];
    MatmulParams params = { (uint32_t)M, (uint32_t)N, (uint32_t)K, (uint32_t)K, (uint32_t)N, (uint32_t)N };
    id<MTLBuffer> bufP = [dev newBufferWithBytes:&params length:sizeof(params) options:MTLResourceStorageModeShared];

    // threadgroup: 16x16
    MTLSize tgs = MTLSizeMake(16, 16, 1);
    MTLSize grid = MTLSizeMake((NSUInteger)((N + 15) & ~15), (NSUInteger)((M + 15) & ~15), 1);
    MTLSize tgCount = MTLSizeMake(grid.width / tgs.width, grid.height / tgs.height, 1);

    // Warmup
    for (int i = 0; i < warmup; ++i) {
        id<MTLCommandBuffer> cb = [q commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
        [enc setComputePipelineState:pso];
        [enc setBuffer:bufA offset:0 atIndex:0];
        [enc setBuffer:bufB offset:0 atIndex:1];
        [enc setBuffer:bufC offset:0 atIndex:2];
        [enc setBuffer:bufP offset:0 atIndex:3];
        [enc dispatchThreadgroups:tgCount threadsPerThreadgroup:tgs];
        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];
    }

    double t0 = now_seconds();
    for (int i = 0; i < iters; ++i) {
        id<MTLCommandBuffer> cb = [q commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
        [enc setComputePipelineState:pso];
        [enc setBuffer:bufA offset:0 atIndex:0];
        [enc setBuffer:bufB offset:0 atIndex:1];
        [enc setBuffer:bufC offset:0 atIndex:2];
        [enc setBuffer:bufP offset:0 atIndex:3];
        [enc dispatchThreadgroups:tgCount threadsPerThreadgroup:tgs];
        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];
    }
    double t1 = now_seconds();

    float *C = (float *)bufC.contents;
    int errors = check(C, Ref, cCount, 1e-2f, 1e-3f);
    *outErrors = errors;

    free(A); free(B); free(Ref);

    return (t1 - t0) / (double)iters;
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        srand(0);

        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        if (!dev) { fprintf(stderr, "No Metal device\n"); return 1; }
        id<MTLCommandQueue> q = [dev newCommandQueue];
        if (!q) { fprintf(stderr, "No command queue\n"); return 1; }

        NSURL *libURL = [NSURL fileURLWithPath:@"./matmul.metallib"];
        id<MTLComputePipelineState> pso16 = make_pipeline(dev, libURL, @"matmul_tiled_fp32_16x16x16");
        id<MTLComputePipelineState> pso32 = make_pipeline(dev, libURL, @"matmul_tiled_fp32_16x16x32");
        id<MTLComputePipelineState> pso32x8 = make_pipeline(dev, libURL, @"matmul_tiled_fp32_32x32x8");
        id<MTLComputePipelineState> pso32x16 = make_pipeline(dev, libURL, @"matmul_tiled_fp32_32x32x16");

        printf("Final Ultra Metal GPU Matmul: Correctness + Perf\n\n");

        struct { int n; } cases[] = {{512},{1024}};
        for (int ci = 0; ci < 2; ++ci) {
            int n = cases[ci].n;
            int errs16 = 0, errs32 = 0, errs32x8 = 0, errs32x16 = 0;

            double sec16 = run_case(dev, q, pso16, n, n, n, 2, 10, &errs16);
            double gflops16 = (2.0 * (double)n * (double)n * (double)n) / sec16 / 1e9;

            double sec32 = run_case(dev, q, pso32, n, n, n, 2, 10, &errs32);
            double gflops32 = (2.0 * (double)n * (double)n * (double)n) / sec32 / 1e9;

            double sec32x8 = run_case(dev, q, pso32x8, n, n, n, 2, 10, &errs32x8);
            double gflops32x8 = (2.0 * (double)n * (double)n * (double)n) / sec32x8 / 1e9;

            double sec32x16 = run_case(dev, q, pso32x16, n, n, n, 2, 10, &errs32x16);
            double gflops32x16 = (2.0 * (double)n * (double)n * (double)n) / sec32x16 / 1e9;

            printf("FP32 %dx%d (TK=16): %.6f s, %.2f GFLOPS, Errors: %d/%d\n",
                   n, n, sec16, gflops16, errs16, n*n);

            printf("FP32 %dx%d (TK=32): %.6f s, %.2f GFLOPS, Errors: %d/%d\n",
                   n, n, sec32, gflops32, errs32, n*n);

            printf("FP32 %dx%d (32x32x8): %.6f s, %.2f GFLOPS, Errors: %d/%d\n",
                   n, n, sec32x8, gflops32x8, errs32x8, n*n);

            printf("FP32 %dx%d (32x32x16): %.6f s, %.2f GFLOPS, Errors: %d/%d\n\n",
                   n, n, sec32x16, gflops32x16, errs32x16, n*n);
        }
    }
    return 0;
}
