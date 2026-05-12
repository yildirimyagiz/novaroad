/**
 * flash_attention_metal.metal - Flash Attention-2 for Apple Metal
 * 
 * Optimized Flash Attention implementation for Apple GPU
 * Expected performance: 100-300× faster than CPU on M1/M2
 */

#include <metal_stdlib>
using namespace metal;

// Constants for tiling
constant uint TILE_SIZE_Q [[function_constant(0)]];
constant uint TILE_SIZE_K [[function_constant(1)]];
constant uint HEAD_DIM [[function_constant(2)]];

/**
 * Flash Attention Kernel
 * 
 * Input layout: [batch, num_heads, seq_len, head_dim]
 * 
 * Each threadgroup processes a tile of queries
 * Uses shared memory (threadgroup memory) for tiling
 */
kernel void flash_attention_forward(
    device const float* Q [[buffer(0)]],           // Query: [B, H, N, D]
    device const float* K [[buffer(1)]],           // Key: [B, H, N, D]
    device const float* V [[buffer(2)]],           // Value: [B, H, N, D]
    device float* output [[buffer(3)]],            // Output: [B, H, N, D]
    constant uint& batch_size [[buffer(4)]],
    constant uint& num_heads [[buffer(5)]],
    constant uint& seq_len [[buffer(6)]],
    constant uint& head_dim [[buffer(7)]],
    constant float& scale [[buffer(8)]],
    constant bool& causal [[buffer(9)]],
    uint3 gid [[threadgroup_position_in_grid]],    // (head, batch, q_tile)
    uint3 tid [[thread_position_in_threadgroup]],  // (thread_in_tile, 0, 0)
    uint tg_size [[threads_per_threadgroup]])
{
    // Threadgroup shared memory for tiles
    threadgroup float q_tile[64 * 64];    // Query tile
    threadgroup float k_tile[64 * 64];    // Key tile
    threadgroup float v_tile[64 * 64];    // Value tile
    threadgroup float scores_tile[64 * 64]; // Attention scores
    threadgroup float max_vals[64];       // Max values per query
    threadgroup float sum_exp[64];        // Sum of exponentials
    
    uint batch = gid.y;
    uint head = gid.x;
    uint q_tile_idx = gid.z;
    
    uint q_start = q_tile_idx * 64;
    uint q_end = min(q_start + 64, seq_len);
    uint q_size = q_end - q_start;
    
    // Base offset for this batch and head
    uint base_offset = (batch * num_heads + head) * seq_len * head_dim;
    
    // Initialize output accumulator
    threadgroup float out_tile[64 * 64];
    for (uint i = tid.x; i < q_size * head_dim; i += tg_size) {
        out_tile[i] = 0.0f;
    }
    
    // Initialize stats
    if (tid.x < q_size) {
        max_vals[tid.x] = -INFINITY;
        sum_exp[tid.x] = 0.0f;
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Load Q tile
    for (uint i = tid.x; i < q_size * head_dim; i += tg_size) {
        uint q_idx = q_start + i / head_dim;
        uint d_idx = i % head_dim;
        q_tile[i] = Q[base_offset + q_idx * head_dim + d_idx];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Process K, V in tiles
    uint num_k_tiles = (seq_len + 63) / 64;
    
    for (uint k_tile_idx = 0; k_tile_idx < num_k_tiles; k_tile_idx++) {
        uint k_start = k_tile_idx * 64;
        uint k_end = min(k_start + 64, seq_len);
        uint k_size = k_end - k_start;
        
        // Load K tile
        for (uint i = tid.x; i < k_size * head_dim; i += tg_size) {
            uint k_idx = k_start + i / head_dim;
            uint d_idx = i % head_dim;
            k_tile[i] = K[base_offset + k_idx * head_dim + d_idx];
        }
        
        // Load V tile
        for (uint i = tid.x; i < k_size * head_dim; i += tg_size) {
            uint v_idx = k_start + i / head_dim;
            uint d_idx = i % head_dim;
            v_tile[i] = V[base_offset + v_idx * head_dim + d_idx];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
        
        // Compute Q @ K^T for this tile
        for (uint q_idx = tid.x; q_idx < q_size; q_idx += tg_size) {
            for (uint k_idx = 0; k_idx < k_size; k_idx++) {
                float score = 0.0f;
                
                // Dot product
                for (uint d = 0; d < head_dim; d++) {
                    score += q_tile[q_idx * head_dim + d] * k_tile[k_idx * head_dim + d];
                }
                
                score *= scale;
                
                // Apply causal mask
                if (causal) {
                    uint global_q = q_start + q_idx;
                    uint global_k = k_start + k_idx;
                    if (global_k > global_q) {
                        score = -INFINITY;
                    }
                }
                
                scores_tile[q_idx * k_size + k_idx] = score;
            }
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
        
        // Online softmax with running max and sum
        for (uint q_idx = tid.x; q_idx < q_size; q_idx += tg_size) {
            // Find max in this tile
            float tile_max = scores_tile[q_idx * k_size];
            for (uint k_idx = 1; k_idx < k_size; k_idx++) {
                tile_max = max(tile_max, scores_tile[q_idx * k_size + k_idx]);
            }
            
            // Update global max
            float old_max = max_vals[q_idx];
            float new_max = max(tile_max, old_max);
            
            // Rescale factors
            float scale_old = exp(old_max - new_max);
            
            // Compute exp and new sum
            float new_sum = 0.0f;
            for (uint k_idx = 0; k_idx < k_size; k_idx++) {
                float exp_val = exp(scores_tile[q_idx * k_size + k_idx] - new_max);
                scores_tile[q_idx * k_size + k_idx] = exp_val;
                new_sum += exp_val;
            }
            
            // Rescale output accumulator
            for (uint d = 0; d < head_dim; d++) {
                out_tile[q_idx * head_dim + d] *= scale_old;
            }
            
            // Add contribution from this tile
            for (uint k_idx = 0; k_idx < k_size; k_idx++) {
                float attn_weight = scores_tile[q_idx * k_size + k_idx];
                for (uint d = 0; d < head_dim; d++) {
                    out_tile[q_idx * head_dim + d] += attn_weight * v_tile[k_idx * head_dim + d];
                }
            }
            
            // Update max and sum
            max_vals[q_idx] = new_max;
            sum_exp[q_idx] = sum_exp[q_idx] * scale_old + new_sum;
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    // Normalize and write output
    for (uint i = tid.x; i < q_size * head_dim; i += tg_size) {
        uint q_idx = i / head_dim;
        uint d_idx = i % head_dim;
        float inv_sum = 1.0f / sum_exp[q_idx];
        
        uint global_q = q_start + q_idx;
        output[base_offset + global_q * head_dim + d_idx] = out_tile[i] * inv_sum;
    }
}

/**
 * Fused RMSNorm kernel
 */
kernel void rms_norm(
    device const float* input [[buffer(0)]],
    device const float* weight [[buffer(1)]],
    device float* output [[buffer(2)]],
    constant uint& n [[buffer(3)]],
    constant float& eps [[buffer(4)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= n) return;
    
    // Compute RMS
    float sum_sq = 0.0f;
    for (uint i = 0; i < n; i++) {
        float val = input[gid * n + i];
        sum_sq += val * val;
    }
    float rms = sqrt(sum_sq / float(n) + eps);
    
    // Normalize and scale
    for (uint i = 0; i < n; i++) {
        output[gid * n + i] = (input[gid * n + i] / rms) * weight[i];
    }
}

/**
 * Fused SwiGLU activation (for LLaMA/Mistral FFN)
 * out = swish(W1 @ x) * (W2 @ x)
 */
kernel void swiglu(
    device const float* input [[buffer(0)]],
    device const float* W1 [[buffer(1)]],
    device const float* W2 [[buffer(2)]],
    device float* output [[buffer(3)]],
    constant uint& batch_seq [[buffer(4)]],
    constant uint& hidden_size [[buffer(5)]],
    constant uint& intermediate_size [[buffer(6)]],
    uint2 gid [[thread_position_in_grid]])
{
    uint batch_idx = gid.y;
    uint i = gid.x;
    
    if (batch_idx >= batch_seq || i >= intermediate_size) return;
    
    // Linear 1: W1 @ x
    float gate = 0.0f;
    for (uint j = 0; j < hidden_size; j++) {
        gate += W1[i * hidden_size + j] * input[batch_idx * hidden_size + j];
    }
    
    // Linear 2: W2 @ x
    float up = 0.0f;
    for (uint j = 0; j < hidden_size; j++) {
        up += W2[i * hidden_size + j] * input[batch_idx * hidden_size + j];
    }
    
    // SwiGLU: swish(gate) * up
    // swish(x) = x * sigmoid(x) = x / (1 + exp(-x))
    float swish_gate = gate / (1.0f + exp(-gate));
    output[batch_idx * intermediate_size + i] = swish_gate * up;
}

/**
 * RoPE (Rotary Position Embeddings) application
 */
kernel void apply_rope(
    device float* tensor [[buffer(0)]],           // [batch, heads, seq_len, head_dim]
    device const float* cos_cache [[buffer(1)]],  // [max_seq_len, head_dim]
    device const float* sin_cache [[buffer(2)]],  // [max_seq_len, head_dim]
    constant uint& batch_heads [[buffer(3)]],
    constant uint& seq_len [[buffer(4)]],
    constant uint& head_dim [[buffer(5)]],
    constant uint& position_offset [[buffer(6)]],
    uint3 gid [[thread_position_in_grid]])
{
    uint bh = gid.x;
    uint pos = gid.y;
    
    if (bh >= batch_heads || pos >= seq_len) return;
    
    uint actual_pos = pos + position_offset;
    uint base_offset = (bh * seq_len + pos) * head_dim;
    
    // Apply rotation to pairs
    for (uint i = 0; i < head_dim / 2; i++) {
        uint cos_idx = actual_pos * head_dim + i;
        float cos_val = cos_cache[cos_idx];
        float sin_val = sin_cache[cos_idx];
        
        float x0 = tensor[base_offset + i];
        float x1 = tensor[base_offset + i + head_dim / 2];
        
        tensor[base_offset + i] = x0 * cos_val - x1 * sin_val;
        tensor[base_offset + i + head_dim / 2] = x0 * sin_val + x1 * cos_val;
    }
}
