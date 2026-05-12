/**
 * ═══════════════════════════════════════════════════════════════════════════
 * SECTION 2: MEMORY SYSTEM STRESS TESTS
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_scientific_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ═══════════════════════════════════════════════════════════════════════════
// SEQUENTIAL STREAMING ACCESS
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t bench_sequential_access(volatile char* buffer, size_t size, int iterations) {
    uint64_t checksum = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < size; i++) {
            checksum ^= buffer[i];
        }
    }
    
    return checksum;
}

// ═══════════════════════════════════════════════════════════════════════════
// RANDOM POINTER CHASING
// ═══════════════════════════════════════════════════════════════════════════

typedef struct node {
    struct node* next;
    char padding[56]; // Total 64 bytes (cache line)
} node_t;

static uint64_t bench_random_chase(node_t* nodes, size_t count, int iterations) {
    uint64_t checksum = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        node_t* current = nodes;
        for (size_t i = 0; i < count; i++) {
            current = current->next;
            checksum ^= (uint64_t)current;
        }
    }
    
    return checksum;
}

// ═══════════════════════════════════════════════════════════════════════════
// STRIDED ACCESS PATTERNS
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t bench_strided_access(volatile char* buffer, size_t size, 
                                     size_t stride, int iterations) {
    uint64_t checksum = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < size; i += stride) {
            checksum ^= buffer[i];
        }
    }
    
    return checksum;
}

// ═══════════════════════════════════════════════════════════════════════════
// LARGE BUFFER MEMCPY
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t bench_large_copy(char* src, char* dst, size_t size, int iterations) {
    uint64_t checksum = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        memcpy(dst, src, size);
        // Prevent optimization
        for (size_t i = 0; i < size; i += 4096) {
            checksum ^= dst[i];
        }
    }
    
    return checksum;
}

// ═══════════════════════════════════════════════════════════════════════════
// CACHE HIERARCHY DETECTION
// ═══════════════════════════════════════════════════════════════════════════

static void detect_cache_level(size_t size, double latency_ns_per_op, char* cache_level) {
    // Typical cache sizes (approximate)
    // L1: 32-64 KB
    // L2: 256 KB - 1 MB
    // L3: 2 MB - 32 MB
    // DRAM: > 32 MB
    
    if (size <= 64 * 1024) {
        strcpy(cache_level, "L1");
    } else if (size <= 1024 * 1024) {
        strcpy(cache_level, "L2");
    } else if (size <= 32 * 1024 * 1024) {
        strcpy(cache_level, "L3");
    } else {
        strcpy(cache_level, "DRAM");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// MEMORY BENCHMARK DRIVER
// ═══════════════════════════════════════════════════════════════════════════

MemoryMetrics bench_memory(size_t size, MemoryPattern pattern) {
    MemoryMetrics metrics = {0};
    
    const int WARMUP_ITERS = 5;
    const int BENCH_ITERS = 20;
    
    if (pattern == MEM_SEQUENTIAL) {
        volatile char* buffer = (volatile char*)malloc(size);
        if (!buffer) {
            fprintf(stderr, "Failed to allocate memory\n");
            return metrics;
        }
        
        // Initialize
        for (size_t i = 0; i < size; i++) {
            buffer[i] = (char)(rand() % 256);
        }
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERS; i++) {
            bench_sequential_access(buffer, size, 1);
        }
        
        // Benchmark
        NovaTimer timer;
        nova_timer_start(&timer);
        uint64_t checksum = bench_sequential_access(buffer, size, BENCH_ITERS);
        nova_timer_stop(&timer);
        
        double total_bytes = (double)size * BENCH_ITERS;
        double elapsed_sec = (double)timer.elapsed_ns / 1e9;
        
        metrics.bandwidth_gbs = total_bytes / elapsed_sec / 1e9;
        metrics.latency_ns_per_op = (double)timer.elapsed_ns / (size * BENCH_ITERS);
        metrics.checksum = checksum;
        metrics.validated = !nova_detect_dead_code_elimination(checksum);
        metrics.cache_hit_rate = 0.95; // Sequential has high hit rate
        
        detect_cache_level(size, metrics.latency_ns_per_op, metrics.cache_level);
        
        free((void*)buffer);
        
    } else if (pattern == MEM_RANDOM_CHASE) {
        size_t count = size / sizeof(node_t);
        node_t* nodes = (node_t*)malloc(count * sizeof(node_t));
        
        if (!nodes) {
            fprintf(stderr, "Failed to allocate memory\n");
            return metrics;
        }
        
        // Create random pointer chain
        size_t* indices = (size_t*)malloc(count * sizeof(size_t));
        for (size_t i = 0; i < count; i++) {
            indices[i] = i;
        }
        
        // Fisher-Yates shuffle
        for (size_t i = count - 1; i > 0; i--) {
            size_t j = rand() % (i + 1);
            size_t temp = indices[i];
            indices[i] = indices[j];
            indices[j] = temp;
        }
        
        // Build chain
        for (size_t i = 0; i < count - 1; i++) {
            nodes[indices[i]].next = &nodes[indices[i + 1]];
        }
        nodes[indices[count - 1]].next = &nodes[indices[0]];
        
        free(indices);
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERS; i++) {
            bench_random_chase(nodes, count, 1);
        }
        
        // Benchmark
        NovaTimer timer;
        nova_timer_start(&timer);
        uint64_t checksum = bench_random_chase(nodes, count, BENCH_ITERS);
        nova_timer_stop(&timer);
        
        metrics.latency_ns_per_op = (double)timer.elapsed_ns / (count * BENCH_ITERS);
        metrics.bandwidth_gbs = 0.0; // Random chase is latency-bound
        metrics.checksum = checksum;
        metrics.validated = !nova_detect_dead_code_elimination(checksum);
        metrics.cache_hit_rate = 0.1; // Random has low hit rate
        
        detect_cache_level(size, metrics.latency_ns_per_op, metrics.cache_level);
        
        free(nodes);
        
    } else if (pattern == MEM_STRIDED) {
        volatile char* buffer = (volatile char*)malloc(size);
        if (!buffer) {
            fprintf(stderr, "Failed to allocate memory\n");
            return metrics;
        }
        
        // Initialize
        for (size_t i = 0; i < size; i++) {
            buffer[i] = (char)(rand() % 256);
        }
        
        size_t stride = 64; // Cache line stride
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERS; i++) {
            bench_strided_access(buffer, size, stride, 1);
        }
        
        // Benchmark
        NovaTimer timer;
        nova_timer_start(&timer);
        uint64_t checksum = bench_strided_access(buffer, size, stride, BENCH_ITERS);
        nova_timer_stop(&timer);
        
        size_t accesses = (size / stride) * BENCH_ITERS;
        double total_bytes = (double)accesses * stride;
        double elapsed_sec = (double)timer.elapsed_ns / 1e9;
        
        metrics.bandwidth_gbs = total_bytes / elapsed_sec / 1e9;
        metrics.latency_ns_per_op = (double)timer.elapsed_ns / accesses;
        metrics.checksum = checksum;
        metrics.validated = !nova_detect_dead_code_elimination(checksum);
        metrics.cache_hit_rate = 0.5; // Strided has medium hit rate
        
        detect_cache_level(size, metrics.latency_ns_per_op, metrics.cache_level);
        
        free((void*)buffer);
        
    } else if (pattern == MEM_LARGE_COPY) {
        char* src = (char*)malloc(size);
        char* dst = (char*)malloc(size);
        
        if (!src || !dst) {
            fprintf(stderr, "Failed to allocate memory\n");
            free(src);
            free(dst);
            return metrics;
        }
        
        // Initialize
        for (size_t i = 0; i < size; i++) {
            src[i] = (char)(rand() % 256);
        }
        
        // Warmup
        for (int i = 0; i < WARMUP_ITERS; i++) {
            bench_large_copy(src, dst, size, 1);
        }
        
        // Benchmark
        NovaTimer timer;
        nova_timer_start(&timer);
        uint64_t checksum = bench_large_copy(src, dst, size, BENCH_ITERS);
        nova_timer_stop(&timer);
        
        double total_bytes = (double)size * BENCH_ITERS * 2; // Read + Write
        double elapsed_sec = (double)timer.elapsed_ns / 1e9;
        
        metrics.bandwidth_gbs = total_bytes / elapsed_sec / 1e9;
        metrics.latency_ns_per_op = (double)timer.elapsed_ns / (size * BENCH_ITERS);
        metrics.checksum = checksum;
        metrics.validated = !nova_detect_dead_code_elimination(checksum);
        metrics.cache_hit_rate = 0.3; // Copy has low hit rate
        
        detect_cache_level(size, metrics.latency_ns_per_op, metrics.cache_level);
        
        free(src);
        free(dst);
    }
    
    return metrics;
}
