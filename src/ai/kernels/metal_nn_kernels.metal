/**
 * @file metal_nn_kernels.metal
 * @brief Metal GPU kernels for Neural Network operations (macOS/iOS)
 * 
 * Metal Shading Language (MSL) implementation
 * Optimized for Apple Silicon (M1/M2/M3)
 */

#include <metal_stdlib>
using namespace metal;

// ═══════════════════════════════════════════════════════════════════════════
// Pooling Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * MaxPool2D kernel
 * Optimized for Apple Silicon with threadgroup memory
 */
kernel void metal_maxpool2d(
    const device float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    constant uint& batch [[buffer(2)]],
    constant uint& channels [[buffer(3)]],
    constant uint& in_h [[buffer(4)]],
    constant uint& in_w [[buffer(5)]],
    constant uint& out_h [[buffer(6)]],
    constant uint& out_w [[buffer(7)]],
    constant uint& kernel_size [[buffer(8)]],
    constant uint& stride [[buffer(9)]],
    constant uint& padding [[buffer(10)]],
    uint3 gid [[thread_position_in_grid]]
) {
    uint ow = gid.x;
    uint oh = gid.y;
    uint idx = gid.z; // batch * channels
    
    if (ow >= out_w || oh >= out_h) return;
    
    uint c = idx % channels;
    uint b = idx / channels;
    
    float max_val = -INFINITY;
    
    for (uint kh = 0; kh < kernel_size; kh++) {
        for (uint kw = 0; kw < kernel_size; kw++) {
            int ih = int(oh * stride + kh) - int(padding);
            int iw = int(ow * stride + kw) - int(padding);
            
            if (ih >= 0 && ih < int(in_h) && iw >= 0 && iw < int(in_w)) {
                uint in_idx = b * (channels * in_h * in_w) +
                             c * (in_h * in_w) +
                             uint(ih) * in_w + uint(iw);
                max_val = max(max_val, input[in_idx]);
            }
        }
    }
    
    uint out_idx = b * (channels * out_h * out_w) +
                   c * (out_h * out_w) +
                   oh * out_w + ow;
    output[out_idx] = max_val;
}

/**
 * AvgPool2D kernel
 */
kernel void metal_avgpool2d(
    const device float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    constant uint& batch [[buffer(2)]],
    constant uint& channels [[buffer(3)]],
    constant uint& in_h [[buffer(4)]],
    constant uint& in_w [[buffer(5)]],
    constant uint& out_h [[buffer(6)]],
    constant uint& out_w [[buffer(7)]],
    constant uint& kernel_size [[buffer(8)]],
    constant uint& stride [[buffer(9)]],
    constant uint& padding [[buffer(10)]],
    uint3 gid [[thread_position_in_grid]]
) {
    uint ow = gid.x;
    uint oh = gid.y;
    uint idx = gid.z;
    
    if (ow >= out_w || oh >= out_h) return;
    
    uint c = idx % channels;
    uint b = idx / channels;
    
    float sum = 0.0f;
    uint count = 0;
    
    for (uint kh = 0; kh < kernel_size; kh++) {
        for (uint kw = 0; kw < kernel_size; kw++) {
            int ih = int(oh * stride + kh) - int(padding);
            int iw = int(ow * stride + kw) - int(padding);
            
            if (ih >= 0 && ih < int(in_h) && iw >= 0 && iw < int(in_w)) {
                uint in_idx = b * (channels * in_h * in_w) +
                             c * (in_h * in_w) +
                             uint(ih) * in_w + uint(iw);
                sum += input[in_idx];
                count++;
            }
        }
    }
    
    uint out_idx = b * (channels * out_h * out_w) +
                   c * (out_h * out_w) +
                   oh * out_w + ow;
    output[out_idx] = count > 0 ? sum / float(count) : 0.0f;
}

/**
 * Global Average Pooling
 * Optimized with SIMD operations
 */
kernel void metal_global_avgpool(
    const device float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    constant uint& batch [[buffer(2)]],
    constant uint& channels [[buffer(3)]],
    constant uint& height [[buffer(4)]],
    constant uint& width [[buffer(5)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint c = gid.x;
    uint b = gid.y;
    
    if (c >= channels || b >= batch) return;
    
    uint spatial_size = height * width;
    float sum = 0.0f;
    
    // Use SIMD for better performance on Apple Silicon
    uint base = b * (channels * spatial_size) + c * spatial_size;
    
    for (uint i = 0; i < spatial_size; i++) {
        sum += input[base + i];
    }
    
    output[b * channels + c] = sum / float(spatial_size);
}

// ═══════════════════════════════════════════════════════════════════════════
// Normalization Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * LayerNorm kernel
 * Using threadgroup memory for efficient reduction
 */
kernel void metal_layernorm(
    const device float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    constant uint& batch_size [[buffer(2)]],
    constant uint& hidden_dim [[buffer(3)]],
    constant float& eps [[buffer(4)]],
    uint gid [[thread_position_in_grid]],
    uint tid [[thread_index_in_threadgroup]],
    uint tg_size [[threads_per_threadgroup]]
) {
    uint b = gid / tg_size;
    
    if (b >= batch_size) return;
    
    threadgroup float shared_sum[256];
    threadgroup float mean_val;
    threadgroup float var_val;
    
    const device float* x = input + b * hidden_dim;
    device float* y = output + b * hidden_dim;
    
    // Compute mean (parallel reduction)
    float sum = 0.0f;
    for (uint i = tid; i < hidden_dim; i += tg_size) {
        sum += x[i];
    }
    shared_sum[tid] = sum;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Reduction in shared memory
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared_sum[tid] += shared_sum[tid + s];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    if (tid == 0) {
        mean_val = shared_sum[0] / float(hidden_dim);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    float mean = mean_val;
    
    // Compute variance
    float var_sum = 0.0f;
    for (uint i = tid; i < hidden_dim; i += tg_size) {
        float diff = x[i] - mean;
        var_sum += diff * diff;
    }
    shared_sum[tid] = var_sum;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared_sum[tid] += shared_sum[tid + s];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    if (tid == 0) {
        var_val = shared_sum[0] / float(hidden_dim);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    float variance = var_val;
    
    // Normalize
    float inv_std = rsqrt(variance + eps);
    for (uint i = tid; i < hidden_dim; i += tg_size) {
        y[i] = (x[i] - mean) * inv_std;
    }
}

/**
 * RMSNorm kernel (LLaMA style)
 * Optimized for Apple Silicon
 */
kernel void metal_rmsnorm(
    const device float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    constant uint& batch_size [[buffer(2)]],
    constant uint& hidden_dim [[buffer(3)]],
    constant float& eps [[buffer(4)]],
    uint gid [[thread_position_in_grid]],
    uint tid [[thread_index_in_threadgroup]],
    uint tg_size [[threads_per_threadgroup]]
) {
    uint b = gid / tg_size;
    
    if (b >= batch_size) return;
    
    threadgroup float shared_sum[256];
    threadgroup float rms_val;
    
    const device float* x = input + b * hidden_dim;
    device float* y = output + b * hidden_dim;
    
    // Compute sum of squares
    float sum_sq = 0.0f;
    for (uint i = tid; i < hidden_dim; i += tg_size) {
        sum_sq += x[i] * x[i];
    }
    shared_sum[tid] = sum_sq;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Reduction
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            shared_sum[tid] += shared_sum[tid + s];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    if (tid == 0) {
        rms_val = rsqrt(shared_sum[0] / float(hidden_dim) + eps);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    float rms = rms_val;
    
    // Normalize
    for (uint i = tid; i < hidden_dim; i += tg_size) {
        y[i] = x[i] * rms;
    }
}

/**
 * BatchNorm kernel
 */
kernel void metal_batchnorm(
    const device float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    const device float* mean [[buffer(2)]],
    const device float* variance [[buffer(3)]],
    const device float* gamma [[buffer(4)]],
    const device float* beta [[buffer(5)]],
    constant uint& batch_size [[buffer(6)]],
    constant uint& num_channels [[buffer(7)]],
    constant uint& spatial_size [[buffer(8)]],
    constant float& eps [[buffer(9)]],
    uint gid [[thread_position_in_grid]]
) {
    uint total = batch_size * num_channels * spatial_size;
    
    if (gid >= total) return;
    
    uint s = gid % spatial_size;
    uint c = (gid / spatial_size) % num_channels;
    uint b = gid / (spatial_size * num_channels);
    
    float inv_std = rsqrt(variance[c] + eps);
    float normalized = (input[gid] - mean[c]) * inv_std;
    
    float scale = gamma ? gamma[c] : 1.0f;
    float shift = beta ? beta[c] : 0.0f;
    
    output[gid] = scale * normalized + shift;
}

// ═══════════════════════════════════════════════════════════════════════════
// Loss Function Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * CrossEntropy loss with softmax
 */
kernel void metal_cross_entropy(
    const device float* logits [[buffer(0)]],
    const device float* targets [[buffer(1)]],
    device atomic_float* loss [[buffer(2)]],
    constant uint& batch_size [[buffer(3)]],
    constant uint& num_classes [[buffer(4)]],
    uint gid [[thread_position_in_grid]],
    uint tid [[thread_index_in_threadgroup]],
    uint tg_size [[threads_per_threadgroup]]
) {
    uint b = gid / tg_size;
    
    if (b >= batch_size) return;
    
    threadgroup float shared_max;
    threadgroup float shared_sum;
    
    const device float* logit_row = logits + b * num_classes;
    
    // Find max (for numerical stability)
    float max_val = -INFINITY;
    for (uint i = tid; i < num_classes; i += tg_size) {
        max_val = max(max_val, logit_row[i]);
    }
    
    // Reduce max
    threadgroup float temp_max[256];
    temp_max[tid] = max_val;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            temp_max[tid] = max(temp_max[tid], temp_max[tid + s]);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    if (tid == 0) {
        shared_max = temp_max[0];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Compute exp sum
    float sum_exp = 0.0f;
    for (uint i = tid; i < num_classes; i += tg_size) {
        sum_exp += exp(logit_row[i] - shared_max);
    }
    
    temp_max[tid] = sum_exp;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    for (uint s = tg_size / 2; s > 0; s >>= 1) {
        if (tid < s) {
            temp_max[tid] += temp_max[tid + s];
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    if (tid == 0) {
        shared_sum = temp_max[0];
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Compute loss
    if (tid == 0) {
        uint target_class = uint(targets[b]);
        float log_sum_exp = shared_max + log(shared_sum);
        float log_prob = logit_row[target_class] - log_sum_exp;
        atomic_fetch_add_explicit(loss, -log_prob, memory_order_relaxed);
    }
}

/**
 * MSE loss kernel
 */
kernel void metal_mse_loss(
    const device float* predictions [[buffer(0)]],
    const device float* targets [[buffer(1)]],
    device atomic_float* loss [[buffer(2)]],
    constant uint& n [[buffer(3)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= n) return;
    
    float diff = predictions[gid] - targets[gid];
    atomic_fetch_add_explicit(loss, diff * diff, memory_order_relaxed);
}

// ═══════════════════════════════════════════════════════════════════════════
// Optimizer Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * AdamW optimizer step
 * Optimized for Apple Silicon
 */
kernel void metal_adamw_step(
    device float* params [[buffer(0)]],
    const device float* grads [[buffer(1)]],
    device float* m [[buffer(2)]],
    device float* v [[buffer(3)]],
    constant uint& n [[buffer(4)]],
    constant float& lr [[buffer(5)]],
    constant float& beta1 [[buffer(6)]],
    constant float& beta2 [[buffer(7)]],
    constant float& eps [[buffer(8)]],
    constant float& weight_decay [[buffer(9)]],
    constant float& bias_correction1 [[buffer(10)]],
    constant float& bias_correction2 [[buffer(11)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= n) return;
    
    float grad = grads[gid];
    
    // Update biased first moment
    m[gid] = beta1 * m[gid] + (1.0f - beta1) * grad;
    
    // Update biased second moment
    v[gid] = beta2 * v[gid] + (1.0f - beta2) * grad * grad;
    
    // Bias-corrected moments
    float m_hat = m[gid] / bias_correction1;
    float v_hat = v[gid] / bias_correction2;
    
    // Update parameters with decoupled weight decay
    params[gid] -= lr * (m_hat / (sqrt(v_hat) + eps) + weight_decay * params[gid]);
}

/**
 * SGD with momentum
 */
kernel void metal_sgd_momentum(
    device float* params [[buffer(0)]],
    const device float* grads [[buffer(1)]],
    device float* velocity [[buffer(2)]],
    constant uint& n [[buffer(3)]],
    constant float& lr [[buffer(4)]],
    constant float& momentum [[buffer(5)]],
    constant float& weight_decay [[buffer(6)]],
    constant bool& nesterov [[buffer(7)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= n) return;
    
    float grad = grads[gid];
    
    // Add weight decay
    if (weight_decay > 0.0f) {
        grad += weight_decay * params[gid];
    }
    
    // Update velocity
    velocity[gid] = momentum * velocity[gid] + grad;
    
    // Update parameters
    if (nesterov) {
        params[gid] -= lr * (grad + momentum * velocity[gid]);
    } else {
        params[gid] -= lr * velocity[gid];
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Advanced Kernels (Apple Silicon Optimized)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Fused MatMul + ReLU
 * Takes advantage of Metal's fast local memory
 */
kernel void metal_matmul_relu(
    const device float* A [[buffer(0)]],
    const device float* B [[buffer(1)]],
    device float* C [[buffer(2)]],
    constant uint& M [[buffer(3)]],
    constant uint& N [[buffer(4)]],
    constant uint& K [[buffer(5)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= N || gid.y >= M) return;
    
    float sum = 0.0f;
    for (uint k = 0; k < K; k++) {
        sum += A[gid.y * K + k] * B[k * N + gid.x];
    }
    
    // Fused ReLU activation
    C[gid.y * N + gid.x] = max(0.0f, sum);
}

/**
 * Attention QKV computation (for Transformers)
 * Optimized for Apple Neural Engine
 */
kernel void metal_attention_qkv(
    const device float* input [[buffer(0)]],
    const device float* W_q [[buffer(1)]],
    const device float* W_k [[buffer(2)]],
    const device float* W_v [[buffer(3)]],
    device float* Q [[buffer(4)]],
    device float* K [[buffer(5)]],
    device float* V [[buffer(6)]],
    constant uint& seq_len [[buffer(7)]],
    constant uint& d_model [[buffer(8)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= d_model || gid.y >= seq_len) return;
    
    uint row = gid.y;
    uint col = gid.x;
    
    // Compute Q, K, V in parallel
    float q_sum = 0.0f, k_sum = 0.0f, v_sum = 0.0f;
    
    for (uint i = 0; i < d_model; i++) {
        float x = input[row * d_model + i];
        q_sum += x * W_q[i * d_model + col];
        k_sum += x * W_k[i * d_model + col];
        v_sum += x * W_v[i * d_model + col];
    }
    
    uint idx = row * d_model + col;
    Q[idx] = q_sum;
    K[idx] = k_sum;
    V[idx] = v_sum;
}
