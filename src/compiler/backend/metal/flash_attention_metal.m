/**
 * flash_attention_metal.c - Metal Flash Attention Bridge
 * 
 * C interface to Metal Flash Attention kernels
 */

#ifdef __APPLE__

#include "../../../../include/backend/nova_backend_dispatch.h"

#include <stdio.h>
#include <stdlib.h>

// Metal framework headers
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

static id<MTLDevice> g_device = nil;
static id<MTLCommandQueue> g_queue = nil;
static id<MTLLibrary> g_library = nil;
static id<MTLComputePipelineState> g_flash_attn_pipeline = nil;

/**
 * Initialize Metal backend for Flash Attention
 */
int nova_metal_flash_attention_init(void) {
    @autoreleasepool {
        // Get default Metal device
        g_device = MTLCreateSystemDefaultDevice();
        if (!g_device) {
            fprintf(stderr, "❌ Metal: No Metal device found\n");
            return -1;
        }
        
        // Create command queue
        g_queue = [g_device newCommandQueue];
        if (!g_queue) {
            fprintf(stderr, "❌ Metal: Failed to create command queue\n");
            return -1;
        }
        
        // Load Metal library
        NSError *error = nil;
        NSString *libraryPath = @"flash_attention_metal.metallib";
        
        // Try to load compiled library
        NSURL *libraryURL = [NSURL fileURLWithPath:libraryPath];
        g_library = [g_device newLibraryWithURL:libraryURL error:&error];
        
        if (!g_library) {
            // Fallback: compile from source
            NSString *sourcePath = [[NSBundle mainBundle] pathForResource:@"flash_attention_metal" 
                                                                    ofType:@"metal"];
            if (sourcePath) {
                NSString *source = [NSString stringWithContentsOfFile:sourcePath 
                                                              encoding:NSUTF8StringEncoding 
                                                                 error:&error];
                if (source) {
                    g_library = [g_device newLibraryWithSource:source options:nil error:&error];
                }
            }
        }
        
        if (!g_library) {
            fprintf(stderr, "❌ Metal: Failed to load library: %s\n", 
                    [[error localizedDescription] UTF8String]);
            return -1;
        }
        
        // Create Flash Attention pipeline
        id<MTLFunction> flashAttnFunc = [g_library newFunctionWithName:@"flash_attention_forward"];
        if (!flashAttnFunc) {
            fprintf(stderr, "❌ Metal: Flash Attention function not found\n");
            return -1;
        }
        
        g_flash_attn_pipeline = [g_device newComputePipelineStateWithFunction:flashAttnFunc 
                                                                         error:&error];
        if (!g_flash_attn_pipeline) {
            fprintf(stderr, "❌ Metal: Failed to create pipeline: %s\n",
                    [[error localizedDescription] UTF8String]);
            return -1;
        }
        
        printf("✅ Metal Flash Attention initialized\n");
        printf("   Device: %s\n", [[g_device name] UTF8String]);
        
        return 0;
    }
}

/**
 * Metal Flash Attention execution
 */
int64_t nova_metal_flash_attention(
    const float *Q,
    const float *K,
    const float *V,
    float *output,
    int64_t batch,
    int64_t num_heads,
    int64_t seq_len,
    int64_t head_dim,
    float scale,
    bool causal
) {
    @autoreleasepool {
        if (!g_device || !g_flash_attn_pipeline) {
            fprintf(stderr, "⚠️  Metal not initialized, falling back\n");
            return -1;
        }
        
        // Calculate buffer sizes
        size_t qkv_size = batch * num_heads * seq_len * head_dim * sizeof(float);
        
        // Create Metal buffers
        id<MTLBuffer> qBuffer = [g_device newBufferWithBytes:Q
                                                       length:qkv_size
                                                      options:MTLResourceStorageModeShared];
        id<MTLBuffer> kBuffer = [g_device newBufferWithBytes:K
                                                       length:qkv_size
                                                      options:MTLResourceStorageModeShared];
        id<MTLBuffer> vBuffer = [g_device newBufferWithBytes:V
                                                       length:qkv_size
                                                      options:MTLResourceStorageModeShared];
        id<MTLBuffer> outBuffer = [g_device newBufferWithLength:qkv_size
                                                         options:MTLResourceStorageModeShared];
        
        if (!qBuffer || !kBuffer || !vBuffer || !outBuffer) {
            fprintf(stderr, "❌ Metal: Failed to allocate buffers\n");
            return -1;
        }
        
        // Create command buffer
        id<MTLCommandBuffer> commandBuffer = [g_queue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        
        // Set pipeline state
        [encoder setComputePipelineState:g_flash_attn_pipeline];
        
        // Set buffers
        [encoder setBuffer:qBuffer offset:0 atIndex:0];
        [encoder setBuffer:kBuffer offset:0 atIndex:1];
        [encoder setBuffer:vBuffer offset:0 atIndex:2];
        [encoder setBuffer:outBuffer offset:0 atIndex:3];
        
        // Set parameters
        uint32_t params[4] = {
            (uint32_t)batch,
            (uint32_t)num_heads,
            (uint32_t)seq_len,
            (uint32_t)head_dim
        };
        [encoder setBytes:&params[0] length:sizeof(uint32_t) atIndex:4];
        [encoder setBytes:&params[1] length:sizeof(uint32_t) atIndex:5];
        [encoder setBytes:&params[2] length:sizeof(uint32_t) atIndex:6];
        [encoder setBytes:&params[3] length:sizeof(uint32_t) atIndex:7];
        [encoder setBytes:&scale length:sizeof(float) atIndex:8];
        [encoder setBytes:&causal length:sizeof(bool) atIndex:9];
        
        // Calculate grid size
        // Each threadgroup handles 64 queries
        NSUInteger numQTiles = (seq_len + 63) / 64;
        MTLSize gridSize = MTLSizeMake(num_heads, batch, numQTiles);
        MTLSize threadgroupSize = MTLSizeMake(256, 1, 1);
        
        // Dispatch
        [encoder dispatchThreadgroups:gridSize threadsPerThreadgroup:threadgroupSize];
        [encoder endEncoding];
        
        // Commit and wait
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        
        // Copy results back
        memcpy(output, [outBuffer contents], qkv_size);
        
        // Check for errors
        if (commandBuffer.error) {
            fprintf(stderr, "❌ Metal execution error: %s\n",
                    [[commandBuffer.error localizedDescription] UTF8String]);
            return -1;
        }
        
        return 0;
    }
}

/**
 * Cleanup
 */
void nova_metal_flash_attention_cleanup(void) {
    @autoreleasepool {
        g_flash_attn_pipeline = nil;
        g_library = nil;
        g_queue = nil;
        g_device = nil;
    }
}

#else

// Stub for non-Apple platforms
int nova_metal_flash_attention_init(void) { return -1; }
int64_t nova_metal_flash_attention(const float *Q, const float *K, const float *V,
                                    float *output, int64_t batch, int64_t num_heads,
                                    int64_t seq_len, int64_t head_dim,
                                    float scale, bool causal) { return -1; }
void nova_metal_flash_attention_cleanup(void) {}

#endif
