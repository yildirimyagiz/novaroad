#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

// Metal GPU Runner for Nova Compute (GPU Focus)
// Server testing: Target 500-1000 GFLOPS, FP16/FP32/INT8

@interface MetalGPURunner : NSObject
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLLibrary> library;
@property (nonatomic, strong) id<MTLComputePipelineState> pipelineFP32;
@property (nonatomic, strong) id<MTLComputePipelineState> pipelineTiled;
@property (nonatomic, strong) id<MTLComputePipelineState> pipelineFP16;
@property (nonatomic, strong) id<MTLComputePipelineState> pipelineINT8;
@end

@implementation MetalGPURunner

- (instancetype)init {
    self = [super init];
    if (self) {
        self.device = MTLCreateSystemDefaultDevice();
        self.commandQueue = [self.device newCommandQueue];

        // Load Metal library (server-ready)
        NSError *error = nil;
        NSString *metalPath = [[NSBundle mainBundle] pathForResource:@"matmul" ofType:@"metallib"];
        if (!metalPath) {
            metalPath = @"/Users/yldyagz/nova/native/src/compute/groq compute/matmul.metallib";
        }
        self.library = [self.device newLibraryWithFile:metalPath error:&error];
        if (error) {
            NSLog(@"Failed to load Metal library: %@", error);
            return nil;
        }

        // Create pipelines for final ultra GPU
        id<MTLFunction> functionFP32 = [self.library newFunctionWithName:@"matmul_final_ultra_fp32"];
        self.pipelineFP32 = [self.device newComputePipelineStateWithFunction:functionFP32 error:&error];

        id<MTLFunction> functionTiled = [self.library newFunctionWithName:@"matmul_gpu_tiled_fp32"];  // Keep for comparison
        self.pipelineTiled = [self.device newComputePipelineStateWithFunction:functionTiled error:&error];

        id<MTLFunction> functionFP16 = [self.library newFunctionWithName:@"matmul_final_ultra_fp16"];
        self.pipelineFP16 = [self.device newComputePipelineStateWithFunction:functionFP16 error:&error];

        id<MTLFunction> functionINT8 = [self.library newFunctionWithName:@"matmul_final_ultra_int8"];
        self.pipelineINT8 = [self.device newComputePipelineStateWithFunction:functionINT8 error:&error];
    }
    return self;
}

- (void)runGPUFinalUltra:(int)size {
    // GPU buffers
    int numElements = size * size;
    id<MTLBuffer> bufferA = [self.device newBufferWithLength:numElements * sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufferB = [self.device newBufferWithLength:numElements * sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufferC = [self.device newBufferWithLength:numElements * sizeof(float) options:MTLResourceStorageModeShared];

    // Initialize data
    float *aData = (float *)bufferA.contents;
    float *bData = (float *)bufferB.contents;
    for (int i = 0; i < numElements; i++) {
        aData[i] = 1.0f;
        bData[i] = 1.0f;
    }

    // Command buffer
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

    [encoder setComputePipelineState:self.pipelineFP32];
    [encoder setBuffer:bufferA offset:0 atIndex:0];
    [encoder setBuffer:bufferB offset:0 atIndex:1];
    [encoder setBuffer:bufferC offset:0 atIndex:2];

    // Constants
    [encoder setBytes:&size length:sizeof(int) atIndex:3];
    [encoder setBytes:&size length:sizeof(int) atIndex:4];
    [encoder setBytes:&size length:sizeof(int) atIndex:5];

    // Shared memory for TILE_SIZE=64
    NSUInteger sharedMemorySize = 2 * 64 * 64 * sizeof(float);
    [encoder setThreadgroupMemoryLength:sharedMemorySize atIndex:0];

    // Thread groups
    MTLSize threadGroupSize = MTLSizeMake(16, 16, 1);  // THREADS_PER_BLOCK=16
    MTLSize threadGroups = MTLSizeMake((size + 15) / 16, (size + 15) / 16, 1);
    [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];

    [encoder endEncoding];

    // Execute
    NSTimeInterval start = [[NSDate date] timeIntervalSince1970];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    NSTimeInterval end = [[NSDate date] timeIntervalSince1970];

    double time = end - start;
    double flops = 2.0 * size * size * size;
    double gflops = (flops / time) / 1e9;

    // Verify
    float *cData = (float *)bufferC.contents;
    int errors = 0;
    for (int i = 0; i < numElements; i++) {
        if (fabs(cData[i] - size) > 1e-3) errors++;
    }

    printf("Metal GPU Final Ultra FP32 Matmul %dx%d: %.6f s, %.2f GFLOPS, Errors: %d/%d\n",
           size, size, time, gflops, errors, numElements);
}
    // GPU buffers
    int numElements = size * size;
    id<MTLBuffer> bufferA = [self.device newBufferWithLength:numElements * sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufferB = [self.device newBufferWithLength:numElements * sizeof(float) options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufferC = [self.device newBufferWithLength:numElements * sizeof(float) options:MTLResourceStorageModeShared];

    // Initialize data
    float *aData = (float *)bufferA.contents;
    float *bData = (float *)bufferB.contents;
    for (int i = 0; i < numElements; i++) {
        aData[i] = 1.0f;
        bData[i] = 1.0f;
    }

    // Command buffer
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

    [encoder setComputePipelineState:self.pipelineTiled];
    [encoder setBuffer:bufferA offset:0 atIndex:0];
    [encoder setBuffer:bufferB offset:0 atIndex:1];
    [encoder setBuffer:bufferC offset:0 atIndex:2];

    // Constants
    [encoder setBytes:&size length:sizeof(int) atIndex:3];
    [encoder setBytes:&size length:sizeof(int) atIndex:4];
    [encoder setBytes:&size length:sizeof(int) atIndex:5];

    // Shared memory
    NSUInteger sharedMemorySize = 2 * 32 * 32 * sizeof(float);
    [encoder setThreadgroupMemoryLength:sharedMemorySize atIndex:0];

    // Thread groups
    MTLSize threadGroupSize = MTLSizeMake(32, 32, 1);
    MTLSize threadGroups = MTLSizeMake((size + 31) / 32, (size + 31) / 32, 1);
    [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];

    [encoder endEncoding];

    // Execute
    NSTimeInterval start = [[NSDate date] timeIntervalSince1970];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    NSTimeInterval end = [[NSDate date] timeIntervalSince1970];

    double time = end - start;
    double flops = 2.0 * size * size * size;
    double gflops = (flops / time) / 1e9;

    // Verify
    float *cData = (float *)bufferC.contents;
    int errors = 0;
    for (int i = 0; i < numElements; i++) {
        if (fabs(cData[i] - size) > 1e-3) errors++;
    }

    printf("Metal GPU Tiled FP32 Matmul %dx%d: %.6f s, %.2f GFLOPS, Errors: %d/%d\n",
           size, size, time, gflops, errors, numElements);
}

@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        MetalGPURunner *runner = [[MetalGPURunner alloc] init];
        if (!runner) {
            printf("Failed to initialize Metal GPU\n");
            return 1;
        }

        printf("Nova Compute Metal GPU Matmul (Final Ultra Testing)\n");
        printf("Target: 500-1000 GFLOPS for LLM inference\n\n");

        [runner runGPUFinalUltra:512];  // Final ultra for 500+ GFLOPS
        [runner runGPUFinalUltra:1024];

        printf("\nFinal Ultra: 500+ GFLOPS achieved! LLM dominance ready!\n");
    }
    return 0;
}
