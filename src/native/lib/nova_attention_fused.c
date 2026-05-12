// nova_attention_fused.c
// Fused attention kernel: QK^T + softmax + V in single pass
// Memory-efficient: avoids materializing full attention matrix

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) ||             \
    defined(_M_IX86)
#include <immintrin.h>
#define NOVA_USE_AVX2
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define NOVA_USE_NEON
#endif

// Naive attention (materializes full matrix)
void attention_naive(const float *Q, // (batch * num_heads, seq_len, head_dim)
                     const float *K, // (batch * num_heads, kv_len, head_dim)
                     const float *V, // (batch * num_heads, kv_len, head_dim)
                     float *O,       // (batch * num_heads, seq_len, head_dim)
                     int batch_heads, int seq_len, int kv_len, int head_dim) {
  float scale = 1.0f / sqrtf((float)head_dim);

  // Allocate attention matrix (memory bottleneck!)
  float *attn = (float *)malloc(seq_len * kv_len * sizeof(float));

  for (int bh = 0; bh < batch_heads; bh++) {
    const float *q = &Q[bh * seq_len * head_dim];
    const float *k = &K[bh * kv_len * head_dim];
    const float *v = &V[bh * kv_len * head_dim];
    float *o = &O[bh * seq_len * head_dim];

    // QK^T
    for (int i = 0; i < seq_len; i++) {
      for (int j = 0; j < kv_len; j++) {
        float sum = 0.0f;
        for (int d = 0; d < head_dim; d++) {
          sum += q[i * head_dim + d] * k[j * head_dim + d];
        }
        attn[i * kv_len + j] = sum * scale;
      }
    }

    // Softmax (row-wise)
    for (int i = 0; i < seq_len; i++) {
      float *row = &attn[i * kv_len];

      // Max for numerical stability
      float max_val = row[0];
      for (int j = 1; j < kv_len; j++) {
        if (row[j] > max_val)
          max_val = row[j];
      }

      // Exp and sum
      float sum_exp = 0.0f;
      for (int j = 0; j < kv_len; j++) {
        row[j] = expf(row[j] - max_val);
        sum_exp += row[j];
      }

      // Normalize
      for (int j = 0; j < kv_len; j++) {
        row[j] /= sum_exp;
      }
    }

    // Attention @ V
    for (int i = 0; i < seq_len; i++) {
      for (int d = 0; d < head_dim; d++) {
        float sum = 0.0f;
        for (int j = 0; j < kv_len; j++) {
          sum += attn[i * kv_len + j] * v[j * head_dim + d];
        }
        o[i * head_dim + d] = sum;
      }
    }
  }

  free(attn);
}

// Fused attention (no intermediate matrix)
// Processes one query vector at a time
void attention_fused(const float *Q, const float *K, const float *V, float *O,
                     int batch_heads, int seq_len, int kv_len, int head_dim) {
  float scale = 1.0f / sqrtf((float)head_dim);

  // Temporary buffers (much smaller than full attention matrix)
  float *scores = (float *)malloc(kv_len * sizeof(float));
  float *attn_weights = (float *)malloc(kv_len * sizeof(float));

  for (int bh = 0; bh < batch_heads; bh++) {
    const float *q = &Q[bh * seq_len * head_dim];
    const float *k = &K[bh * kv_len * head_dim];
    const float *v = &V[bh * kv_len * head_dim];
    float *o = &O[bh * seq_len * head_dim];

    // Process each query vector
    for (int i = 0; i < seq_len; i++) {
      const float *q_vec = &q[i * head_dim];
      float *o_vec = &o[i * head_dim];

      // Compute scores: q @ K^T
      for (int j = 0; j < kv_len; j++) {
        float sum = 0.0f;
        for (int d = 0; d < head_dim; d++) {
          sum += q_vec[d] * k[j * head_dim + d];
        }
        scores[j] = sum * scale;
      }

      // Softmax
      float max_score = scores[0];
      for (int j = 1; j < kv_len; j++) {
        if (scores[j] > max_score)
          max_score = scores[j];
      }

      float sum_exp = 0.0f;
      for (int j = 0; j < kv_len; j++) {
        attn_weights[j] = expf(scores[j] - max_score);
        sum_exp += attn_weights[j];
      }

      for (int j = 0; j < kv_len; j++) {
        attn_weights[j] /= sum_exp;
      }

      // Compute output: attn @ V
      for (int d = 0; d < head_dim; d++) {
        float sum = 0.0f;
        for (int j = 0; j < kv_len; j++) {
          sum += attn_weights[j] * v[j * head_dim + d];
        }
        o_vec[d] = sum;
      }
    }
  }

  free(scores);
  free(attn_weights);
}

// Flash Attention style (tiled + fused)
void attention_flash(const float *Q, const float *K, const float *V, float *O,
                     int batch_heads, int seq_len, int kv_len, int head_dim) {
  // Tile size for cache efficiency
  const int TILE_SIZE = 64;
  float scale = 1.0f / sqrtf((float)head_dim);

  // Temporary buffers
  float *scores = (float *)aligned_alloc(32, TILE_SIZE * sizeof(float));
  float *attn_weights = (float *)aligned_alloc(32, TILE_SIZE * sizeof(float));

  for (int bh = 0; bh < batch_heads; bh++) {
    const float *q = &Q[bh * seq_len * head_dim];
    const float *k = &K[bh * kv_len * head_dim];
    const float *v = &V[bh * kv_len * head_dim];
    float *o = &O[bh * seq_len * head_dim];

    // Process queries
    for (int i = 0; i < seq_len; i++) {
      const float *q_vec = &q[i * head_dim];
      float *o_vec = &o[i * head_dim];

      // Zero output accumulator
      memset(o_vec, 0, head_dim * sizeof(float));

      float max_score = -INFINITY;
      float sum_exp = 0.0f;

      // Process KV in tiles
      for (int kv_start = 0; kv_start < kv_len; kv_start += TILE_SIZE) {
        int tile_size =
            (kv_start + TILE_SIZE <= kv_len) ? TILE_SIZE : (kv_len - kv_start);

        // Compute scores for this tile
        for (int j = 0; j < tile_size; j++) {
          int kv_idx = kv_start + j;
          float sum = 0.0f;

// Vectorized dot product
#if defined(NOVA_USE_AVX2)
          __m256 acc = _mm256_setzero_ps();
          for (int d = 0; d < head_dim; d += 8) {
            __m256 q_vec_simd = _mm256_loadu_ps(&q_vec[d]);
            __m256 k_vec_simd = _mm256_loadu_ps(&k[kv_idx * head_dim + d]);
            acc = _mm256_fmadd_ps(q_vec_simd, k_vec_simd, acc);
          }

          // Horizontal sum
          __m128 sum128 = _mm_add_ps(_mm256_extractf128_ps(acc, 0),
                                     _mm256_extractf128_ps(acc, 1));
          sum128 = _mm_hadd_ps(sum128, sum128);
          sum128 = _mm_hadd_ps(sum128, sum128);
          sum = _mm_cvtss_f32(sum128);
#elif defined(NOVA_USE_NEON)
          float32x4_t acc = vdupq_n_f32(0.0f);
          for (int d = 0; d < head_dim; d += 4) {
            float32x4_t q_vec_simd = vld1q_f32(&q_vec[d]);
            float32x4_t k_vec_simd = vld1q_f32(&k[kv_idx * head_dim + d]);
            acc = vmlaq_f32(acc, q_vec_simd, k_vec_simd);
          }
          sum = vgetq_lane_f32(acc, 0) + vgetq_lane_f32(acc, 1) +
                vgetq_lane_f32(acc, 2) + vgetq_lane_f32(acc, 3);
#else
          for (int d = 0; d < head_dim; d++) {
            sum += q_vec[d] * k[kv_idx * head_dim + d];
          }
#endif

          scores[j] = sum * scale;
          if (scores[j] > max_score)
            max_score = scores[j];
        }

        // Compute attention weights (softmax numerator)
        for (int j = 0; j < tile_size; j++) {
          attn_weights[j] = expf(scores[j] - max_score);
          sum_exp += attn_weights[j];
        }

        // Accumulate weighted values
        for (int j = 0; j < tile_size; j++) {
          int kv_idx = kv_start + j;
          float weight = attn_weights[j];

          for (int d = 0; d < head_dim; d++) {
            o_vec[d] += weight * v[kv_idx * head_dim + d];
          }
        }
      }

      // Normalize output
      for (int d = 0; d < head_dim; d++) {
        o_vec[d] /= sum_exp;
      }
    }
  }

  free(scores);
  free(attn_weights);
}

// Benchmark
double get_time() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  yield ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main() {
  printf("=== Nova Fused Attention Benchmark ===\n\n");

  // Test configuration (typical for LLM inference)
  int batch = 1;
  int num_heads = 12;
  int seq_len = 128;
  int kv_len = 512; // With KV-cache
  int head_dim = 64;

  int batch_heads = batch * num_heads;

  printf("Configuration:\n");
  printf("  Batch: %d, Heads: %d\n", batch, num_heads);
  printf("  Seq length: %d, KV length: %d\n", seq_len, kv_len);
  printf("  Head dim: %d\n\n", head_dim);

  // Allocate tensors
  size_t q_size = batch_heads * seq_len * head_dim;
  size_t kv_size = batch_heads * kv_len * head_dim;

  float *Q = (float *)aligned_alloc(32, q_size * sizeof(float));
  float *K = (float *)aligned_alloc(32, kv_size * sizeof(float));
  float *V = (float *)aligned_alloc(32, kv_size * sizeof(float));
  float *O = (float *)aligned_alloc(32, q_size * sizeof(float));

  // Initialize
  for (size_t i = 0; i < q_size; i++)
    Q[i] = (float)rand() / RAND_MAX;
  for (size_t i = 0; i < kv_size; i++)
    K[i] = (float)rand() / RAND_MAX;
  for (size_t i = 0; i < kv_size; i++)
    V[i] = (float)rand() / RAND_MAX;

  // Warmup
  attention_fused(Q, K, V, O, batch_heads, seq_len, kv_len, head_dim);

  // Benchmark naive
  double t0 = get_time();
  attention_naive(Q, K, V, O, batch_heads, seq_len, kv_len, head_dim);
  double t_naive = get_time() - t0;

  // Calculate memory usage
  size_t attn_matrix_bytes = seq_len * kv_len * sizeof(float);
  float attn_matrix_mb = attn_matrix_bytes / (1024.0f * 1024.0f);

  printf("Naive attention:\n");
  printf("  Time: %.4f s\n", t_naive);
  printf("  Memory for attention matrix: %.2f MB\n\n", attn_matrix_mb);

  // Benchmark fused
  t0 = get_time();
  attention_fused(Q, K, V, O, batch_heads, seq_len, kv_len, head_dim);
  double t_fused = get_time() - t0;

  printf("Fused attention:\n");
  printf("  Time: %.4f s\n", t_fused);
  printf("  Speedup: %.2fx\n", t_naive / t_fused);
  printf("  Memory: O(kv_len) instead of O(seq_len * kv_len)\n\n");

  // Benchmark flash attention
  t0 = get_time();
  attention_flash(Q, K, V, O, batch_heads, seq_len, kv_len, head_dim);
  double t_flash = get_time() - t0;

  printf("Flash attention (tiled + SIMD):\n");
  printf("  Time: %.4f s\n", t_flash);
  printf("  Speedup vs naive: %.2fx\n", t_naive / t_flash);
  printf("  Speedup vs fused: %.2fx\n\n", t_fused / t_flash);

  printf("Summary:\n");
  printf("  ✓ Fused attention avoids materializing full matrix\n");
  printf("  ✓ Memory-efficient: critical for long sequences\n");
  printf("  ✓ Cache-friendly tiling for better performance\n");
  printf("  ✓ SIMD vectorization for compute efficiency\n");

  free(Q);
  free(K);
  free(V);
  free(O);

  yield 0;
}
