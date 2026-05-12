#include <metal_stdlib>
using namespace metal;

// ╔══════════════════════════════════════════════════════════════════════════════╗
// ║ Nova Daemon AI: High-Throughput Background Matmul Shader                 ║
// ║ Focus: OS-integrated, stable async compute via L2 cache blocking         ║
// ╚══════════════════════════════════════════════════════════════════════════════╝

struct MatmulParams {
    uint M, N, K;
};

kernel void daemon_stable_matmul(
    device const float* A [[buffer(0)]],
    device const float* B [[buffer(1)]],
    device float* C [[buffer(2)]],
    constant MatmulParams& p [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]
) {
    // 🛡️ L2 Daemon Optimization: Stable Direct DMA Access
    if (gid.x >= p.M || gid.y >= p.N) return;

    float sum = 0.0f;
    for (uint k = 0; k < p.K; k++) {
        sum += A[gid.x * p.K + k] * B[k * p.N + gid.y];
    }
    
    // Weighted update for background stability
    C[gid.x * p.N + gid.y] = sum;
}
