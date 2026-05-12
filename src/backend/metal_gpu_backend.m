/**
 * @file metal_gpu_backend.m
 * @brief Metal GPU backend implementation (Objective-C++)
 */

#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdio.h>
#include <string.h>

// Metal device context
typedef struct {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLLibrary> library;
    id<MTLComputePipelineState> pipelineSimple;
    id<MTLComputePipelineState> pipelineTiled;
    id<MTLComputePipelineState> pipelineFP16;
} MetalContext;

/**
 * Create Metal device and compile shaders
 */
void* metal_device_create(void)
{
    @autoreleasepool {
        MetalContext* ctx = (MetalContext*)malloc(sizeof(MetalContext));
        if (!ctx) return NULL;
        
        // Get default Metal device
        ctx->device = MTLCreateSystemDefaultDevice();
        if (!ctx->device) {
            fprintf(stderr, "Metal is not supported on this device\n");
            free(ctx);
            return NULL;
        }
        
        printf("Metal Device: %s\n", [ctx->device.name UTF8String]);
        
        // Create command queue
        ctx->commandQueue = [ctx->device newCommandQueue];
        if (!ctx->commandQueue) {
            fprintf(stderr, "Failed to create command queue\n");
            free(ctx);
            return NULL;
        }
        
        // Load Metal shader library
        NSError* error = nil;
        
        // Try to load precompiled metallib
        NSString* libraryPath = @"metal_gpu_gemm.metallib";
        ctx->library = [ctx->device newLibraryWithFile:libraryPath error:&error];
        
        if (!ctx->library) {
            // Fallback: compile from source
            NSString* shaderPath = @"src/backend/metal_gpu_gemm.metal";
            NSString* shaderSource = [NSString stringWithContentsOfFile:shaderPath
                                                                encoding:NSUTF8StringEncoding
                                                                   error:&error];
            if (!shaderSource) {
                fprintf(stderr, "Failed to load Metal shader source: %s\n",
                       [[error localizedDescription] UTF8String]);
                free(ctx);
                return NULL;
            }
            
            ctx->library = [ctx->device newLibraryWithSource:shaderSource
                                                     options:nil
                                                       error:&error];
        }
        
        if (!ctx->library) {
            fprintf(stderr, "Failed to create Metal library: %s\n",
                   [[error localizedDescription] UTF8String]);
            free(ctx);
            return NULL;
        }
        
        // Create compute pipelines
        id<MTLFunction> funcSimple = [ctx->library newFunctionWithName:@"gemm_simple"];
        id<MTLFunction> funcTiled = [ctx->library newFunctionWithName:@"gemm_tiled"];
        id<MTLFunction> funcFP16 = [ctx->library newFunctionWithName:@"gemm_fp16"];
        
        ctx->pipelineSimple = [ctx->device newComputePipelineStateWithFunction:funcSimple error:&error];
        ctx->pipelineTiled = [ctx->device newComputePipelineStateWithFunction:funcTiled error:&error];
        ctx->pipelineFP16 = [ctx->device newComputePipelineStateWithFunction:funcFP16 error:&error];
        
        if (!ctx->pipelineSimple || !ctx->pipelineTiled) {
            fprintf(stderr, "Failed to create compute pipeline\n");
            free(ctx);
            return NULL;
        }
        
        return ctx;
    }
}

/**
 * Destroy Metal device
 */
void metal_device_destroy(void* device)
{
    if (device) {
        free(device);
    }
}

/**
 * Execute simple GEMM kernel
 */
int metal_gemm_execute(void* device,
                      const float* A, const float* B, float* C,
                      int M, int N, int K)
{
    @autoreleasepool {
        MetalContext* ctx = (MetalContext*)device;
        
        // Create buffers
        size_t sizeA = M * K * sizeof(float);
        size_t sizeB = K * N * sizeof(float);
        size_t sizeC = M * N * sizeof(float);
        
        id<MTLBuffer> bufA = [ctx->device newBufferWithBytes:A length:sizeA options:MTLResourceStorageModeShared];
        id<MTLBuffer> bufB = [ctx->device newBufferWithBytes:B length:sizeB options:MTLResourceStorageModeShared];
        id<MTLBuffer> bufC = [ctx->device newBufferWithLength:sizeC options:MTLResourceStorageModeShared];
        
        // Create command buffer
        id<MTLCommandBuffer> commandBuffer = [ctx->commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        
        [encoder setComputePipelineState:ctx->pipelineSimple];
        [encoder setBuffer:bufA offset:0 atIndex:0];
        [encoder setBuffer:bufB offset:0 atIndex:1];
        [encoder setBuffer:bufC offset:0 atIndex:2];
        [encoder setBytes:&M length:sizeof(int) atIndex:3];
        [encoder setBytes:&N length:sizeof(int) atIndex:4];
        [encoder setBytes:&K length:sizeof(int) atIndex:5];
        
        // Dispatch threads
        MTLSize gridSize = MTLSizeMake(N, M, 1);
        MTLSize threadgroupSize = MTLSizeMake(16, 16, 1);
        [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
        
        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        
        // Copy result back
        memcpy(C, [bufC contents], sizeC);
        
        return 0;
    }
}

/**
 * Execute tiled GEMM kernel (better for large matrices)
 */
int metal_gemm_tiled_execute(void* device,
                             const float* A, const float* B, float* C,
                             int M, int N, int K)
{
    @autoreleasepool {
        MetalContext* ctx = (MetalContext*)device;
        
        size_t sizeA = M * K * sizeof(float);
        size_t sizeB = K * N * sizeof(float);
        size_t sizeC = M * N * sizeof(float);
        
        id<MTLBuffer> bufA = [ctx->device newBufferWithBytes:A length:sizeA options:MTLResourceStorageModeShared];
        id<MTLBuffer> bufB = [ctx->device newBufferWithBytes:B length:sizeB options:MTLResourceStorageModeShared];
        id<MTLBuffer> bufC = [ctx->device newBufferWithLength:sizeC options:MTLResourceStorageModeShared];
        
        id<MTLCommandBuffer> commandBuffer = [ctx->commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        
        [encoder setComputePipelineState:ctx->pipelineTiled];
        [encoder setBuffer:bufA offset:0 atIndex:0];
        [encoder setBuffer:bufB offset:0 atIndex:1];
        [encoder setBuffer:bufC offset:0 atIndex:2];
        [encoder setBytes:&M length:sizeof(int) atIndex:3];
        [encoder setBytes:&N length:sizeof(int) atIndex:4];
        [encoder setBytes:&K length:sizeof(int) atIndex:5];
        
        // Use 32×32 threadgroups
        MTLSize gridSize = MTLSizeMake((N + 31) / 32 * 32, (M + 31) / 32 * 32, 1);
        MTLSize threadgroupSize = MTLSizeMake(32, 32, 1);
        [encoder dispatchThreadgroups:MTLSizeMake((N+31)/32, (M+31)/32, 1)
                threadsPerThreadgroup:threadgroupSize];
        
        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        
        memcpy(C, [bufC contents], sizeC);
        
        return 0;
    }
}

/**
 * Execute FP16 GEMM kernel
 */
int metal_gemm_fp16_execute(void* device,
                           const void* A, const void* B, void* C,
                           int M, int N, int K)
{
    @autoreleasepool {
        MetalContext* ctx = (MetalContext*)device;
        
        if (!ctx->pipelineFP16) return -1; // FP16 not supported
        
        size_t sizeA = M * K * sizeof(uint16_t);
        size_t sizeB = K * N * sizeof(uint16_t);
        size_t sizeC = M * N * sizeof(uint16_t);
        
        id<MTLBuffer> bufA = [ctx->device newBufferWithBytes:A length:sizeA options:MTLResourceStorageModeShared];
        id<MTLBuffer> bufB = [ctx->device newBufferWithBytes:B length:sizeB options:MTLResourceStorageModeShared];
        id<MTLBuffer> bufC = [ctx->device newBufferWithLength:sizeC options:MTLResourceStorageModeShared];
        
        id<MTLCommandBuffer> commandBuffer = [ctx->commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        
        [encoder setComputePipelineState:ctx->pipelineFP16];
        [encoder setBuffer:bufA offset:0 atIndex:0];
        [encoder setBuffer:bufB offset:0 atIndex:1];
        [encoder setBuffer:bufC offset:0 atIndex:2];
        [encoder setBytes:&M length:sizeof(int) atIndex:3];
        [encoder setBytes:&N length:sizeof(int) atIndex:4];
        [encoder setBytes:&K length:sizeof(int) atIndex:5];
        
        MTLSize gridSize = MTLSizeMake(N, M, 1);
        MTLSize threadgroupSize = MTLSizeMake(16, 16, 1);
        [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
        
        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        
        memcpy(C, [bufC contents], sizeC);
        
        return 0;
    }
}

#endif /* __APPLE__ */
