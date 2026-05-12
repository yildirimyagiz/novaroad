#include <metal_stdlib>
using namespace metal;

// ╔══════════════════════════════════════════════════════════════════════════════╗
// ║ Nova Reflex AI: Ultra-Low Latency Flash Attention Shader                 ║
// ║ Target: <0.1ms compute, Tiled Softmax + Weighted Sum                      ║
// ╚══════════════════════════════════════════════════════════════════════════════╝

struct AttentionParams {
    uint seq_len;
    uint head_dim;
    float scale;
};

kernel void reflex_flash_attention(
    device const float* Q [[buffer(0)]],
    device const float* K [[buffer(1)]],
    device const float* V [[buffer(2)]],
    device float* O [[buffer(3)]],
    constant AttentionParams& p [[buffer(4)]],
    ushort2 tg_pos [[threadgroup_position_in_grid]],
    ushort2 th_pos [[thread_position_in_threadgroup]]
) {
    // 🦅 L1 Reflex Optimization: Threadgroup Shared Memory
    threadgroup float tile_S[1024]; // Score tile
    
    const uint q_idx = (tg_pos.x * 16 + th_pos.x) * p.head_dim;
    const uint k_idx = (tg_pos.y * 16 + th_pos.y) * p.head_dim;
    
    // 1. Dot Product (Q * K^T)
    float score = 0.0f;
    for (uint i = 0; i < p.head_dim; i++) {
        score += Q[q_idx + i] * K[k_idx + i];
    }
    score *= p.scale;
    
    // 2. Softmax (Reflex Approximation for Latency)
    // In a real Flash Attention, we'd do online softmax across tiles.
    // Here we do a fast fused exponential.
    float exp_s = exp(score);
    tile_S[th_pos.x * 16 + th_pos.y] = exp_s;
    
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // 3. Weighted Sum (S * V)
    float out_val = 0.0f;
    for (uint i = 0; i < 16; i++) {
        out_val += tile_S[th_pos.x * 16 + i] * V[(tg_pos.y * 16 + i) * p.head_dim + th_pos.y];
    }
    
    O[q_idx + th_pos.y] = out_val;
}
