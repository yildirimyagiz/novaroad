/**
 * @file gpu_nn_kernels.cu
 * @brief CUDA GPU kernels for Neural Network operations
 * 
 * Includes:
 * - Pooling (MaxPool, AvgPool)
 * - Normalization (LayerNorm, BatchNorm, RMSNorm)
 * - Loss functions (CrossEntropy, MSE, etc.)
 * - Optimizers (Adam, AdamW, SGD)
 */

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>
#include <stdio.h>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define BLOCK_SIZE 256
#define TILE_SIZE 16
#define WARP_SIZE 32

// ═══════════════════════════════════════════════════════════════════════════
// Pooling Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * MaxPool2D kernel
 * Input: (batch, channels, height, width)
 * Output: (batch, channels, out_height, out_width)
 */
__global__ void cuda_maxpool2d_kernel(
    const float* input,
    float* output,
    int batch, int channels,
    int in_h, int in_w,
    int out_h, int out_w,
    int kernel_size, int stride, int padding
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = batch * channels * out_h * out_w;
    
    if (idx < total) {
        int ow = idx % out_w;
        int oh = (idx / out_w) % out_h;
        int c = (idx / (out_w * out_h)) % channels;
        int b = idx / (out_w * out_h * channels);
        
        float max_val = -INFINITY;
        
        for (int kh = 0; kh < kernel_size; kh++) {
            for (int kw = 0; kw < kernel_size; kw++) {
                int ih = oh * stride + kh - padding;
                int iw = ow * stride + kw - padding;
                
                if (ih >= 0 && ih < in_h && iw >= 0 && iw < in_w) {
                    int in_idx = b * (channels * in_h * in_w) +
                                c * (in_h * in_w) +
                                ih * in_w + iw;
                    max_val = fmaxf(max_val, input[in_idx]);
                }
            }
        }
        
        output[idx] = max_val;
    }
}

/**
 * AvgPool2D kernel
 */
__global__ void cuda_avgpool2d_kernel(
    const float* input,
    float* output,
    int batch, int channels,
    int in_h, int in_w,
    int out_h, int out_w,
    int kernel_size, int stride, int padding
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = batch * channels * out_h * out_w;
    
    if (idx < total) {
        int ow = idx % out_w;
        int oh = (idx / out_w) % out_h;
        int c = (idx / (out_w * out_h)) % channels;
        int b = idx / (out_w * out_h * channels);
        
        float sum = 0.0f;
        int count = 0;
        
        for (int kh = 0; kh < kernel_size; kh++) {
            for (int kw = 0; kw < kernel_size; kw++) {
                int ih = oh * stride + kh - padding;
                int iw = ow * stride + kw - padding;
                
                if (ih >= 0 && ih < in_h && iw >= 0 && iw < in_w) {
                    int in_idx = b * (channels * in_h * in_w) +
                                c * (in_h * in_w) +
                                ih * in_w + iw;
                    sum += input[in_idx];
                    count++;
                }
            }
        }
        
        output[idx] = count > 0 ? sum / count : 0.0f;
    }
}

/**
 * Global Average Pooling kernel
 */
__global__ void cuda_global_avgpool_kernel(
    const float* input,
    float* output,
    int batch, int channels,
    int height, int width
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = batch * channels;
    
    if (idx < total) {
        int c = idx % channels;
        int b = idx / channels;
        
        float sum = 0.0f;
        int spatial_size = height * width;
        
        for (int i = 0; i < spatial_size; i++) {
            int in_idx = b * (channels * spatial_size) + c * spatial_size + i;
            sum += input[in_idx];
        }
        
        output[idx] = sum / spatial_size;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Normalization Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * LayerNorm kernel (fused)
 */
__global__ void cuda_layernorm_kernel(
    const float* input,
    float* output,
    int batch_size,
    int hidden_dim,
    float eps
) {
    int b = blockIdx.x;
    
    if (b < batch_size) {
        const float* x = input + b * hidden_dim;
        float* y = output + b * hidden_dim;
        
        // Compute mean
        float sum = 0.0f;
        for (int i = threadIdx.x; i < hidden_dim; i += blockDim.x) {
            sum += x[i];
        }
        
        __shared__ float shared_sum;
        sum = blockReduceSum(sum);
        if (threadIdx.x == 0) {
            shared_sum = sum / hidden_dim;
        }
        __syncthreads();
        float mean = shared_sum;
        
        // Compute variance
        float var_sum = 0.0f;
        for (int i = threadIdx.x; i < hidden_dim; i += blockDim.x) {
            float diff = x[i] - mean;
            var_sum += diff * diff;
        }
        
        var_sum = blockReduceSum(var_sum);
        if (threadIdx.x == 0) {
            shared_sum = var_sum / hidden_dim;
        }
        __syncthreads();
        float variance = shared_sum;
        
        // Normalize
        float inv_std = rsqrtf(variance + eps);
        for (int i = threadIdx.x; i < hidden_dim; i += blockDim.x) {
            y[i] = (x[i] - mean) * inv_std;
        }
    }
}

/**
 * RMSNorm kernel (used in LLaMA)
 */
__global__ void cuda_rmsnorm_kernel(
    const float* input,
    float* output,
    int batch_size,
    int hidden_dim,
    float eps
) {
    int b = blockIdx.x;
    
    if (b < batch_size) {
        const float* x = input + b * hidden_dim;
        float* y = output + b * hidden_dim;
        
        // Compute RMS
        float sum_sq = 0.0f;
        for (int i = threadIdx.x; i < hidden_dim; i += blockDim.x) {
            sum_sq += x[i] * x[i];
        }
        
        __shared__ float shared_rms;
        sum_sq = blockReduceSum(sum_sq);
        if (threadIdx.x == 0) {
            shared_rms = rsqrtf(sum_sq / hidden_dim + eps);
        }
        __syncthreads();
        float rms = shared_rms;
        
        // Normalize
        for (int i = threadIdx.x; i < hidden_dim; i += blockDim.x) {
            y[i] = x[i] * rms;
        }
    }
}

/**
 * BatchNorm kernel
 */
__global__ void cuda_batchnorm_kernel(
    const float* input,
    float* output,
    const float* mean,
    const float* variance,
    const float* gamma,
    const float* beta,
    int batch_size,
    int num_channels,
    int spatial_size,
    float eps
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = batch_size * num_channels * spatial_size;
    
    if (idx < total) {
        int s = idx % spatial_size;
        int c = (idx / spatial_size) % num_channels;
        int b = idx / (spatial_size * num_channels);
        
        float inv_std = rsqrtf(variance[c] + eps);
        float normalized = (input[idx] - mean[c]) * inv_std;
        
        float scale = gamma ? gamma[c] : 1.0f;
        float shift = beta ? beta[c] : 0.0f;
        
        output[idx] = scale * normalized + shift;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Loss Function Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * CrossEntropy loss kernel with softmax
 */
__global__ void cuda_cross_entropy_kernel(
    const float* logits,
    const float* targets,
    float* loss,
    int batch_size,
    int num_classes
) {
    extern __shared__ float shared_mem[];
    
    int b = blockIdx.x;
    
    if (b < batch_size) {
        const float* logit_row = logits + b * num_classes;
        
        // Find max (for numerical stability)
        float max_logit = -INFINITY;
        for (int i = threadIdx.x; i < num_classes; i += blockDim.x) {
            max_logit = fmaxf(max_logit, logit_row[i]);
        }
        max_logit = blockReduceMax(max_logit);
        
        // Compute exp sum
        float sum_exp = 0.0f;
        for (int i = threadIdx.x; i < num_classes; i += blockDim.x) {
            sum_exp += expf(logit_row[i] - max_logit);
        }
        sum_exp = blockReduceSum(sum_exp);
        
        // Compute loss for this sample
        if (threadIdx.x == 0) {
            int target_class = (int)targets[b];
            float log_sum_exp = max_logit + logf(sum_exp);
            float log_prob = logit_row[target_class] - log_sum_exp;
            atomicAdd(loss, -log_prob);
        }
    }
}

/**
 * MSE loss kernel
 */
__global__ void cuda_mse_kernel(
    const float* predictions,
    const float* targets,
    float* loss,
    int n
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < n) {
        float diff = predictions[idx] - targets[idx];
        atomicAdd(loss, diff * diff);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Optimizer Kernels
// ═══════════════════════════════════════════════════════════════════════════

/**
 * AdamW optimizer step kernel
 */
__global__ void cuda_adamw_step_kernel(
    float* params,
    const float* grads,
    float* m,          // First moment
    float* v,          // Second moment
    int n,
    float lr,
    float beta1,
    float beta2,
    float eps,
    float weight_decay,
    int step,
    float bias_correction1,
    float bias_correction2
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < n) {
        float grad = grads[idx];
        
        // Update biased first moment
        m[idx] = beta1 * m[idx] + (1.0f - beta1) * grad;
        
        // Update biased second moment
        v[idx] = beta2 * v[idx] + (1.0f - beta2) * grad * grad;
        
        // Bias-corrected moments
        float m_hat = m[idx] / bias_correction1;
        float v_hat = v[idx] / bias_correction2;
        
        // Update parameters with decoupled weight decay
        params[idx] -= lr * (m_hat / (sqrtf(v_hat) + eps) + weight_decay * params[idx]);
    }
}

/**
 * SGD with momentum kernel
 */
__global__ void cuda_sgd_momentum_kernel(
    float* params,
    const float* grads,
    float* velocity,
    int n,
    float lr,
    float momentum,
    float weight_decay,
    bool nesterov
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < n) {
        float grad = grads[idx];
        
        // Add weight decay
        if (weight_decay > 0) {
            grad += weight_decay * params[idx];
        }
        
        // Update velocity
        velocity[idx] = momentum * velocity[idx] + grad;
        
        // Update parameters
        if (nesterov) {
            params[idx] -= lr * (grad + momentum * velocity[idx]);
        } else {
            params[idx] -= lr * velocity[idx];
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════════════════════

__device__ float blockReduceSum(float val) {
    __shared__ float shared[WARP_SIZE];
    
    int lane = threadIdx.x % WARP_SIZE;
    int wid = threadIdx.x / WARP_SIZE;
    
    // Warp reduction
    for (int offset = WARP_SIZE / 2; offset > 0; offset /= 2) {
        val += __shfl_down_sync(0xffffffff, val, offset);
    }
    
    // Store warp results
    if (lane == 0) {
        shared[wid] = val;
    }
    __syncthreads();
    
    // Final reduction
    if (threadIdx.x < blockDim.x / WARP_SIZE) {
        val = shared[threadIdx.x];
    } else {
        val = 0.0f;
    }
    
    if (wid == 0) {
        for (int offset = blockDim.x / WARP_SIZE / 2; offset > 0; offset /= 2) {
            val += __shfl_down_sync(0xffffffff, val, offset);
        }
    }
    
    return val;
}

__device__ float blockReduceMax(float val) {
    __shared__ float shared[WARP_SIZE];
    
    int lane = threadIdx.x % WARP_SIZE;
    int wid = threadIdx.x / WARP_SIZE;
    
    // Warp reduction
    for (int offset = WARP_SIZE / 2; offset > 0; offset /= 2) {
        val = fmaxf(val, __shfl_down_sync(0xffffffff, val, offset));
    }
    
    if (lane == 0) {
        shared[wid] = val;
    }
    __syncthreads();
    
    if (threadIdx.x < blockDim.x / WARP_SIZE) {
        val = shared[threadIdx.x];
    } else {
        val = -INFINITY;
    }
    
    if (wid == 0) {
        for (int offset = blockDim.x / WARP_SIZE / 2; offset > 0; offset /= 2) {
            val = fmaxf(val, __shfl_down_sync(0xffffffff, val, offset));
        }
    }
    
    return val;
}

// ═══════════════════════════════════════════════════════════════════════════
// C Interface
// ═══════════════════════════════════════════════════════════════════════════

extern "C" {

void nova_cuda_maxpool2d(
    const float* input, float* output,
    int batch, int channels, int in_h, int in_w,
    int out_h, int out_w, int kernel_size, int stride, int padding
) {
    int total = batch * channels * out_h * out_w;
    int blocks = (total + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    cuda_maxpool2d_kernel<<<blocks, BLOCK_SIZE>>>(
        input, output, batch, channels, in_h, in_w, out_h, out_w,
        kernel_size, stride, padding
    );
    CUDA_CHECK(cudaDeviceSynchronize());
}

void nova_cuda_avgpool2d(
    const float* input, float* output,
    int batch, int channels, int in_h, int in_w,
    int out_h, int out_w, int kernel_size, int stride, int padding
) {
    int total = batch * channels * out_h * out_w;
    int blocks = (total + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    cuda_avgpool2d_kernel<<<blocks, BLOCK_SIZE>>>(
        input, output, batch, channels, in_h, in_w, out_h, out_w,
        kernel_size, stride, padding
    );
    CUDA_CHECK(cudaDeviceSynchronize());
}

void nova_cuda_layernorm(
    const float* input, float* output,
    int batch_size, int hidden_dim, float eps
) {
    cuda_layernorm_kernel<<<batch_size, BLOCK_SIZE>>>(
        input, output, batch_size, hidden_dim, eps
    );
    CUDA_CHECK(cudaDeviceSynchronize());
}

void nova_cuda_rmsnorm(
    const float* input, float* output,
    int batch_size, int hidden_dim, float eps
) {
    cuda_rmsnorm_kernel<<<batch_size, BLOCK_SIZE>>>(
        input, output, batch_size, hidden_dim, eps
    );
    CUDA_CHECK(cudaDeviceSynchronize());
}

void nova_cuda_adamw_step(
    float* params, const float* grads, float* m, float* v,
    int n, float lr, float beta1, float beta2, float eps,
    float weight_decay, int step
) {
    float bias_correction1 = 1.0f - powf(beta1, step);
    float bias_correction2 = 1.0f - powf(beta2, step);
    
    int blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    cuda_adamw_step_kernel<<<blocks, BLOCK_SIZE>>>(
        params, grads, m, v, n, lr, beta1, beta2, eps, weight_decay,
        step, bias_correction1, bias_correction2
    );
    CUDA_CHECK(cudaDeviceSynchronize());
}

} // extern "C"
