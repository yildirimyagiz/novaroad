/**
 * @file zenith_autocal_comprehensive.c
 * @brief Comprehensive autocal runner with all benchmarks
 */

#include "../../include/zenith_autocal.h"
#include "../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for all benchmark functions
extern void autocal_benchmark_flash_attention(NovaAutocalConfig *config);
extern void autocal_benchmark_graph_ops(NovaAutocalConfig *config);
extern void autocal_benchmark_kernel_fusion(NovaAutocalConfig *config);
extern void autocal_benchmark_llm_ops(NovaAutocalConfig *config);
extern void autocal_benchmark_llvm_opts(NovaAutocalConfig *config);
extern void autocal_benchmark_quantization(NovaAutocalConfig *config);
extern void autocal_benchmark_gpu_performance(NovaAutocalConfig *config);

/**
 * Run all benchmarks and configure 4LUA thresholds
 */
void zenith_autocal_run_comprehensive(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║          Nova 4LUA Auto-Calibration - Comprehensive Suite           ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║  This will benchmark your hardware across multiple workloads to     ║\n");
    printf("║  determine optimal execution thresholds for the 4-Layer Army.       ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    double total_start = zenith_timer_get_sec();
    
    // Reset to defaults first
    zenith_autocal_reset_defaults();
    
    // 1. Flash Attention
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    autocal_benchmark_flash_attention(&g_nova_autocal_config);
    printf("\n");
    
    // 2. Graph Operations
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    autocal_benchmark_graph_ops(&g_nova_autocal_config);
    printf("\n");
    
    // 3. Kernel Fusion
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    autocal_benchmark_kernel_fusion(&g_nova_autocal_config);
    printf("\n");
    
    // 4. LLM Operations
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    autocal_benchmark_llm_ops(&g_nova_autocal_config);
    printf("\n");
    
    // 5. LLVM Optimizations
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    autocal_benchmark_llvm_opts(&g_nova_autocal_config);
    printf("\n");
    
    // 6. Quantization
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    autocal_benchmark_quantization(&g_nova_autocal_config);
    printf("\n");
    
    // 7. GPU Performance (if available)
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    autocal_benchmark_gpu_performance(&g_nova_autocal_config);
    printf("\n");
    
    // Calculate optimal thresholds based on all benchmarks
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📊 Computing Optimal 4LUA Thresholds...\n");
    
    // L1 (Silicon Reflex): CPU-bound, small sizes
    if (g_nova_autocal_config.cpu_gflops > 50.0) {
        g_nova_autocal_config.l1_reflex_threshold = 2 * 1024 * 1024; // 2M elements
    } else if (g_nova_autocal_config.cpu_gflops > 20.0) {
        g_nova_autocal_config.l1_reflex_threshold = 1024 * 1024; // 1M elements
    } else {
        g_nova_autocal_config.l1_reflex_threshold = 512 * 1024; // 512K elements
    }
    
    // L2 (Kernel Daemon): GPU kicks in
    if (g_nova_autocal_config.gpu_gflops > 0) {
        double gpu_cpu_ratio = g_nova_autocal_config.gpu_gflops / g_nova_autocal_config.cpu_gflops;
        if (gpu_cpu_ratio > 10.0) {
            g_nova_autocal_config.l2_daemon_threshold = 4 * 1024 * 1024; // 4M
        } else {
            g_nova_autocal_config.l2_daemon_threshold = 8 * 1024 * 1024; // 8M
        }
    } else {
        g_nova_autocal_config.l2_daemon_threshold = 16 * 1024 * 1024; // 16M (CPU only)
    }
    
    // L3 (Web Nexus): Massive parallelism
    g_nova_autocal_config.l3_web_threshold = g_nova_autocal_config.l2_daemon_threshold * 4;
    
    double total_time = zenith_timer_get_sec() - total_start;
    
    // Final report
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                  Calibration Complete! ✅                            ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║  4LUA Tier Thresholds:                                               ║\n");
    printf("║    Tier L1 (Silicon Reflex):  < %10llu elements                 ║\n", 
           g_nova_autocal_config.l1_reflex_threshold);
    printf("║    Tier L2 (Kernel Daemon):   < %10llu elements                 ║\n", 
           g_nova_autocal_config.l2_daemon_threshold);
    printf("║    Tier L3 (Web Nexus):       < %10llu elements                 ║\n", 
           g_nova_autocal_config.l3_web_threshold);
    printf("║    Tier L4 (Mesh):              Unlimited (P2P distributed)          ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║  Performance Metrics:                                                ║\n");
    printf("║    CPU GFLOPS:         %.2f                                       ║\n", 
           g_nova_autocal_config.cpu_gflops);
    printf("║    GPU GFLOPS:         %.2f                                       ║\n", 
           g_nova_autocal_config.gpu_gflops);
    printf("║    P2P Bandwidth:      %.2f MB/s                                 ║\n", 
           g_nova_autocal_config.p2p_bandwidth_mbps);
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║  Total calibration time: %.2f seconds                              ║\n", total_time);
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // Save configuration
    zenith_autocal_save("nova_autocal_config.json");
}

/**
 * Quick calibration (subset of benchmarks)
 */
void zenith_autocal_run_quick(void)
{
    printf("🎯 [AUTOCAL] Running Quick Calibration...\n");
    
    zenith_autocal_reset_defaults();
    
    // Only run critical benchmarks
    autocal_benchmark_flash_attention(&g_nova_autocal_config);
    autocal_benchmark_gpu_performance(&g_nova_autocal_config);
    
    // Simple threshold calculation
    g_nova_autocal_config.l1_reflex_threshold = 1024 * 1024;
    g_nova_autocal_config.l2_daemon_threshold = 4 * 1024 * 1024;
    g_nova_autocal_config.l3_web_threshold = 16 * 1024 * 1024;
    
    printf("✅ Quick calibration complete\n");
    zenith_autocal_save("nova_autocal_config.json");
}
