// ============================================================
// NOVA ULTRA MATMUL ENGINE v3.0 — Main + Benchmark Driver
// ============================================================
#include "nova_matmul.h"

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║   NOVA ULTRA MATMUL ENGINE v3.0                        ║\n");
    printf("║   Max GFLOPS + Caching Delta + Min Energy                ║\n");
    printf("║   ARM NEON + Metal FP32/FP16/INT8 Adaptive               ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    int sizes[] = {64, 128, 256, 512, 1024, 2048};
    int n_sizes = sizeof(sizes) / sizeof(sizes[0]);
    int runs = 5;

    benchmark_suite(sizes, n_sizes, runs);

    printf("\n── Wave Quantization Analysis ──────────────────────────────\n");
    printf("Optimal grid dims to avoid tail-wave waste:\n");
    for (int s = 0; s < n_sizes; s++) {
        int sz = sizes[s];
        int tiles_m = (sz + 64 - 1) / 64;
        int tiles_n = (sz + 64 - 1) / 64;
        int total   = tiles_m * tiles_n;
        int optimal = wave_optimal_blocks(total, GPU_SM_COUNT);
        printf("  %4d×%4d: %3d tiles → optimal=%3d (pad %d) → %.0f%% efficiency\n",
               sz, sz, total, optimal,
               optimal - total,
               100.0 * total / optimal);
    }

    printf("\n── Energy Budget Summary ────────────────────────────────────\n");
    printf("  Backend       Power     Use Case\n");
    printf("  CPU_BLOCKED   ~0.08W    ≤128×128, latency-critical\n");
    printf("  CPU_SIMD_16   ~0.15W    128–512, balanced\n");
    printf("  GPU_FP16      ~0.25W    512–1024, inference\n");
    printf("  GPU_INT8      ~0.10W    >1024, throughput-critical\n");
    printf("\n✅ Nova Ultra Adaptive Matmul Engine v3.0 Complete\n\n");
    return 0;
}
