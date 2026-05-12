/**
 * nova_backend_dispatch.c - Unified Backend Dispatcher Implementation
 *
 * Probes all backends at init, selects the best one, and dispatches
 * tensor operations accordingly.
 */

#include "nova_backend_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Backend headers
#include "cuda/nova_cuda.h"
#include "metal/nova_metal_gpu.h"
#include "nova_gpu_army.h" // 4LUA Army Backend
#include "opencl/nova_opencl.h"
#include "rocm/nova_rocm.h"
#include "vulkan/nova_vulkan.h"

// External references (CPU Backend - now int64_t returns)
extern int64_t nova_cpu_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n,
                               int64_t k);
extern int64_t nova_cpu_add(const float *a, const float *b, float *c, int64_t n);
extern int64_t nova_cpu_mul(const float *a, const float *b, float *c, int64_t n);
extern int64_t nova_cpu_relu(const float *input, float *output, int64_t n);
extern int64_t nova_cpu_softmax(const float *input, float *output, int64_t n);
extern int64_t nova_cpu_flash_attention(const float *Q, const float *K, const float *V, float *Out,
                                        int L, int D);
extern void nova_cpu_backend_init(void);

// ═══════════════════════════════════════════════════════════════════════════
// State
// ═══════════════════════════════════════════════════════════════════════════

static NovaBackendStatus g_status = {0};
static int g_dispatch_initialized = 0;

// ═══════════════════════════════════════════════════════════════════════════
// Profiling & Telemetry
// ═══════════════════════════════════════════════════════════════════════════

#include <time.h>

typedef struct {
    uint64_t call_count;
    uint64_t total_ns;
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t failure_count;
} backend_stats_t;

typedef struct {
    backend_stats_t matmul;
    backend_stats_t add;
    backend_stats_t mul;
    backend_stats_t relu;
    backend_stats_t softmax;
    backend_stats_t flash_attention;
} operation_stats_t;

static operation_stats_t g_backend_stats[7] = {0}; // One per backend type

// Adaptive backend switching
static int g_adaptive_mode = 0;
static uint64_t g_adaptive_threshold_ns = 10000000; // 10ms threshold

typedef struct {
    NovaBackendType backend;
    uint64_t avg_latency_ns;
} backend_ranking_t;

static inline uint64_t get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void record_call(backend_stats_t *stats, uint64_t duration_ns, int success)
{
    stats->call_count++;
    stats->total_ns += duration_ns;
    
    if (stats->min_ns == 0 || duration_ns < stats->min_ns)
        stats->min_ns = duration_ns;
    if (duration_ns > stats->max_ns)
        stats->max_ns = duration_ns;
    
    if (!success)
        stats->failure_count++;
}

static const char* get_backend_stat_name(int idx)
{
    const char* names[] = {"Auto", "CPU", "CUDA", "Metal", "ROCm", "Vulkan", "OpenCL", "GPU-Army"};
    return (idx >= 0 && idx < 8) ? names[idx] : "Unknown";
}

const char *nova_backend_name(NovaBackendType type)
{
    switch (type) {
    case NOVA_BACKEND_CPU:
        return "CPU";
    case NOVA_BACKEND_CUDA:
        return "CUDA (NVIDIA)";
    case NOVA_BACKEND_METAL:
        return "Metal (Apple)";
    case NOVA_BACKEND_ROCM:
        return "ROCm (AMD)";
    case NOVA_BACKEND_VULKAN:
        return "Vulkan";
    case NOVA_BACKEND_OPENCL:
        return "OpenCL";
    case NOVA_BACKEND_GPU_ARMY:
        return "🦅 GPU-Army (4LUA Tiered)";
    default:
        return "Auto";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Init
// ═══════════════════════════════════════════════════════════════════════════

int nova_backend_init(NovaBackendType preferred)
{
    if (g_dispatch_initialized)
        return 0;

    printf("═══ Nova Backend System ═══\n");
    printf("Probing available compute backends...\n\n");

    // Init CPU Backend (Thread Pool etc.)
    nova_cpu_backend_init();

    // Probe all backends
    g_status.cuda_available = (nova_cuda_init() == 1);
    if (g_status.cuda_available)
        printf("  ✅ CUDA\n");
    else
        printf("  ❌ CUDA\n");

#ifdef __APPLE__
    g_status.metal_available = (nova_metal_init() == 1);
    if (g_status.metal_available)
        printf("  ✅ Metal\n");
    else
        printf("  ❌ Metal\n");
#else
    g_status.metal_available = false;
    printf("  ⬜ Metal (macOS only)\n");
#endif

    g_status.rocm_available = nova_rocm_is_available();
    if (g_status.rocm_available) {
        nova_rocm_init();
        printf("  ✅ ROCm\n");
    } else {
        printf("  ❌ ROCm\n");
    }

    g_status.vulkan_available = nova_vk_is_available();
    if (g_status.vulkan_available)
        printf("  ✅ Vulkan\n");
    else
        printf("  ❌ Vulkan\n");

    g_status.opencl_available = nova_cl_is_available();
    if (g_status.opencl_available) {
        nova_cl_init();
        printf("  ✅ OpenCL\n");
    } else {
        printf("  ❌ OpenCL\n");
    }

    printf("  ✅ CPU (always available)\n");

    // Select backend with intelligent strategy
    if (preferred != NOVA_BACKEND_AUTO) {
        g_status.active = preferred;
        printf("\n🎯 User-selected Backend: %s\n\n", nova_backend_name(g_status.active));
    } else {
        // Auto-select with workload-aware strategy
        nova_gpu_army_init();
        
        // Strategy: GPU-Army for massive parallel, Metal for Apple, CUDA for NVIDIA
        if (g_status.cuda_available) {
            g_status.active = NOVA_BACKEND_GPU_ARMY; // 4LUA tiered approach
            printf("\n🎯 Auto-selected: %s (optimal for large-scale workloads)\n\n", 
                   nova_backend_name(g_status.active));
        } else if (g_status.metal_available) {
            g_status.active = NOVA_BACKEND_METAL;
            printf("\n🎯 Auto-selected: %s (Apple Silicon optimized)\n\n", 
                   nova_backend_name(g_status.active));
        } else if (g_status.rocm_available) {
            g_status.active = NOVA_BACKEND_ROCM;
            printf("\n🎯 Auto-selected: %s (AMD GPU)\n\n", 
                   nova_backend_name(g_status.active));
        } else if (g_status.vulkan_available) {
            g_status.active = NOVA_BACKEND_VULKAN;
            printf("\n🎯 Auto-selected: %s (cross-platform GPU)\n\n", 
                   nova_backend_name(g_status.active));
        } else if (g_status.opencl_available) {
            g_status.active = NOVA_BACKEND_OPENCL;
            printf("\n🎯 Auto-selected: %s (legacy GPU support)\n\n", 
                   nova_backend_name(g_status.active));
        } else {
            g_status.active = NOVA_BACKEND_CPU;
            printf("\n🎯 Auto-selected: %s (fallback, no GPU detected)\n\n", 
                   nova_backend_name(g_status.active));
        }
    }
    
    g_dispatch_initialized = 1;
    return 0;
}

NovaBackendStatus nova_backend_status(void)
{
    return g_status;
}

void nova_backend_cleanup(void)
{
    if (!g_dispatch_initialized)
        return;

    if (g_status.cuda_available)
        nova_cuda_cleanup();
    if (g_status.rocm_available)
        nova_rocm_cleanup();
    if (g_status.opencl_available)
        nova_cl_cleanup();
    if (g_status.vulkan_available)
        nova_vk_cleanup();
#ifdef __APPLE__
    if (g_status.metal_available)
        nova_metal_cleanup();
#endif

    nova_gpu_army_cleanup(); // 4LUA Cleanup

    g_dispatch_initialized = 0;
    memset(&g_status, 0, sizeof(g_status));
}

void nova_backend_print_all(void)
{
    printf("╔═══ Nova Backend Status ═══╗\n");
    printf("║ Active: %s\n", nova_backend_name(g_status.active));
    printf("║ CUDA:   %s\n", g_status.cuda_available ? "✔" : "✘");
    printf("║ Metal:  %s\n", g_status.metal_available ? "✔" : "✘");
    printf("║ ROCm:   %s\n", g_status.rocm_available ? "✔" : "✘");
    printf("║ Vulkan: %s\n", g_status.vulkan_available ? "✔" : "✘");
    printf("║ OpenCL: %s\n", g_status.opencl_available ? "✔" : "✘");
    printf("║ CPU:    ✔ (always)\n");
    printf("╚══════════════════════════════╝\n");
}

void nova_backend_print_stats(void)
{
    printf("\n╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    Nova Backend Performance Stats                        ║\n");
    printf("╠═══════════════════════════════════════════════════════════════════════════╣\n");
    
    for (int i = 1; i < 7; i++) {
        operation_stats_t *stats = &g_backend_stats[i];
        uint64_t total_calls = stats->matmul.call_count + stats->add.call_count +
                               stats->mul.call_count + stats->relu.call_count +
                               stats->softmax.call_count + stats->flash_attention.call_count;
        
        if (total_calls == 0) continue;
        
        printf("║ Backend: %-20s                                         ║\n", 
               get_backend_stat_name(i));
        
        if (stats->matmul.call_count > 0) {
            printf("║   MatMul:    %8lu calls | Avg: %6lu μs | Min: %6lu μs | Max: %6lu μs ║\n",
                   stats->matmul.call_count,
                   stats->matmul.total_ns / stats->matmul.call_count / 1000,
                   stats->matmul.min_ns / 1000,
                   stats->matmul.max_ns / 1000);
        }
        
        if (stats->flash_attention.call_count > 0) {
            printf("║   FlashAttn: %8lu calls | Avg: %6lu μs | Min: %6lu μs | Max: %6lu μs ║\n",
                   stats->flash_attention.call_count,
                   stats->flash_attention.total_ns / stats->flash_attention.call_count / 1000,
                   stats->flash_attention.min_ns / 1000,
                   stats->flash_attention.max_ns / 1000);
        }
        
        uint64_t total_failures = stats->matmul.failure_count + stats->add.failure_count +
                                  stats->mul.failure_count + stats->relu.failure_count +
                                  stats->softmax.failure_count + stats->flash_attention.failure_count;
        
        if (total_failures > 0) {
            printf("║   ⚠️  Failures: %lu (%.1f%%)                                              ║\n",
                   total_failures, (total_failures * 100.0) / total_calls);
        }
        printf("╠═══════════════════════════════════════════════════════════════════════════╣\n");
    }
    
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
}

void nova_backend_reset_stats(void)
{
    memset(g_backend_stats, 0, sizeof(g_backend_stats));
    printf("🔄 Backend statistics reset\n");
}

void nova_backend_enable_adaptive(int enable)
{
    g_adaptive_mode = enable;
    if (enable) {
        printf("🧠 Adaptive backend switching: ENABLED\n");
        printf("   → Will auto-switch if latency > %lu ms\n", 
               g_adaptive_threshold_ns / 1000000);
    } else {
        printf("🔒 Adaptive backend switching: DISABLED\n");
    }
}

void nova_backend_set_threshold(uint64_t threshold_ms)
{
    g_adaptive_threshold_ns = threshold_ms * 1000000;
    printf("⏱️  Adaptive threshold set to %lu ms\n", threshold_ms);
}

// Get best backend for operation based on stats
static NovaBackendType get_best_backend_for_matmul(void)
{
    if (!g_adaptive_mode) return g_status.active;
    
    NovaBackendType best = g_status.active;
    uint64_t best_latency = UINT64_MAX;
    
    // Check all available backends
    for (int i = 1; i < 7; i++) {
        if (g_backend_stats[i].matmul.call_count < 5) continue; // Need min samples
        
        uint64_t avg = g_backend_stats[i].matmul.total_ns / 
                       g_backend_stats[i].matmul.call_count;
        
        // Check if this backend is available and faster
        bool available = false;
        switch (i) {
            case NOVA_BACKEND_CPU: available = true; break;
            case NOVA_BACKEND_CUDA: available = g_status.cuda_available; break;
            case NOVA_BACKEND_METAL: available = g_status.metal_available; break;
            case NOVA_BACKEND_ROCM: available = g_status.rocm_available; break;
            case NOVA_BACKEND_VULKAN: available = g_status.vulkan_available; break;
            case NOVA_BACKEND_OPENCL: available = g_status.opencl_available; break;
        }
        
        if (available && avg < best_latency) {
            best_latency = avg;
            best = (NovaBackendType)i;
        }
    }
    
    return best;
}

// ═══════════════════════════════════════════════════════════════════════════
// Dispatch Operations (fallback to CPU on error: ret < 0)
// ═══════════════════════════════════════════════════════════════════════════

int64_t nova_dispatch_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n,
                             int64_t k)
{
    uint64_t start = get_time_ns();
    int64_t ret;
    
    // Adaptive backend selection based on historical performance
    NovaBackendType backend = g_adaptive_mode ? get_best_backend_for_matmul() : g_status.active;
    
    switch (backend) {
    case NOVA_BACKEND_CUDA:
        ret = nova_cuda_matmul(a, b, c, m, n, k);
        break;
    case NOVA_BACKEND_METAL:
        ret = nova_metal_matmul(a, b, c, m, n, k);
        break;
    case NOVA_BACKEND_ROCM:
        ret = nova_rocm_matmul(a, b, c, m, n, k);
        break;
    case NOVA_BACKEND_VULKAN:
        ret = nova_vk_matmul(a, b, c, m, n, k);
        break;
    case NOVA_BACKEND_OPENCL:
        ret = nova_cl_matmul(a, b, c, m, n, k);
        break;
    case NOVA_BACKEND_GPU_ARMY:
        ret = nova_gpu_army_matmul(a, b, c, m, n, k);
        break;
    default:
        backend = NOVA_BACKEND_CPU;
        ret = nova_cpu_matmul(a, b, c, m, n, k);
    }
    
    // Fallback to CPU on failure
    if (ret < 0) {
        record_call(&g_backend_stats[backend].matmul, get_time_ns() - start, 0);
        backend = NOVA_BACKEND_CPU;
        start = get_time_ns();
        ret = nova_cpu_matmul(a, b, c, m, n, k);
    }
    
    uint64_t duration = get_time_ns() - start;
    record_call(&g_backend_stats[backend].matmul, duration, 1);
    
    // Check if we should recommend switching backend
    if (g_adaptive_mode && duration > g_adaptive_threshold_ns) {
        static int warning_count = 0;
        if (++warning_count % 10 == 0) {
            printf("⚠️  MatMul latency high (%lu ms) on %s - consider profiling\n",
                   duration / 1000000, nova_backend_name(backend));
        }
    }
    
    return ret;
}

int64_t nova_dispatch_add(const float *a, const float *b, float *c, int64_t n)
{
    int64_t ret;
    switch (g_status.active) {
    case NOVA_BACKEND_CUDA:
        ret = nova_cuda_add(a, b, c, n);
        break;
    case NOVA_BACKEND_METAL:
        ret = nova_metal_add(a, b, c, n);
        break;
    case NOVA_BACKEND_ROCM:
        ret = nova_rocm_add(a, b, c, n);
        break;
    case NOVA_BACKEND_OPENCL:
        ret = nova_cl_add(a, b, c, n);
        break;
    case NOVA_BACKEND_VULKAN:
        ret = nova_vk_add(a, b, c, n);
        break;
    default:
        return nova_cpu_add(a, b, c, n);
    }
    if (ret < 0)
        return nova_cpu_add(a, b, c, n);
    return ret;
}

int64_t nova_dispatch_mul(const float *a, const float *b, float *c, int64_t n)
{
    int64_t ret;
    switch (g_status.active) {
    case NOVA_BACKEND_CUDA:
        ret = nova_cuda_mul(a, b, c, n);
        break;
    case NOVA_BACKEND_METAL:
        ret = nova_metal_mul(a, b, c, n);
        break;
    case NOVA_BACKEND_ROCM:
        ret = nova_rocm_mul(a, b, c, n);
        break;
    case NOVA_BACKEND_OPENCL:
        ret = nova_cl_mul(a, b, c, n);
        break;
    case NOVA_BACKEND_VULKAN:
        ret = nova_vk_mul(a, b, c, n);
        break;
    default:
        return nova_cpu_mul(a, b, c, n);
    }
    if (ret < 0)
        return nova_cpu_mul(a, b, c, n);
    return ret;
}

int64_t nova_dispatch_relu(const float *in, float *out, int64_t n)
{
    int64_t ret;
    switch (g_status.active) {
    case NOVA_BACKEND_METAL:
        ret = nova_metal_relu(in, out, n);
        break;
    case NOVA_BACKEND_ROCM:
        ret = nova_rocm_relu(in, out, n);
        break;
    case NOVA_BACKEND_OPENCL:
        ret = nova_cl_relu(in, out, n);
        break;
    case NOVA_BACKEND_VULKAN:
        ret = nova_vk_relu(in, out, n);
        break;
    case NOVA_BACKEND_CUDA:
    default:
        return nova_cpu_relu(in, out, n);
    }
    if (ret < 0)
        return nova_cpu_relu(in, out, n);
    return ret;
}

int64_t nova_dispatch_softmax(const float *in, float *out, int64_t n)
{
    int64_t ret;
    switch (g_status.active) {
    case NOVA_BACKEND_ROCM:
        ret = nova_rocm_softmax(in, out, n);
        break;
    case NOVA_BACKEND_OPENCL:
        ret = nova_cl_softmax(in, out, n);
        break;
    case NOVA_BACKEND_VULKAN:
    case NOVA_BACKEND_CUDA:
    case NOVA_BACKEND_METAL:
    default:
        return nova_cpu_softmax(in, out, n);
    }
    if (ret < 0)
        return nova_cpu_softmax(in, out, n);
    return ret;
}

int64_t nova_dispatch_flash_attention(const float *Q, const float *K, const float *V, float *Out,
                                      int L, int D)
{
    uint64_t start = get_time_ns();
    int64_t ret;
    NovaBackendType backend = g_status.active;
    
    if (backend == NOVA_BACKEND_GPU_ARMY) {
        ret = nova_gpu_army_flash_attention(Q, K, V, Out, L, D);
        if (ret < 0) {
            record_call(&g_backend_stats[backend].flash_attention, get_time_ns() - start, 0);
            backend = NOVA_BACKEND_CPU;
            start = get_time_ns();
            ret = nova_cpu_flash_attention(Q, K, V, Out, L, D);
        }
    } else {
        backend = NOVA_BACKEND_CPU;
        ret = nova_cpu_flash_attention(Q, K, V, Out, L, D);
    }
    
    uint64_t duration = get_time_ns() - start;
    record_call(&g_backend_stats[backend].flash_attention, duration, 1);
    
    return ret;
}
