#ifndef NOVA_SMART_TRAINING_H
#define NOVA_SMART_TRAINING_H

#include "nova_advanced_optimizations.h" // Full GPU support
#include "runtime/nova_context.h"
#include "nova_fp16_kernels.h"
#include "nova_memory.h"
#include "ml/nova_tensor.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// INTELLIGENT DATASET MANAGEMENT (PERFORMANCE + IQ BOOST)
// ============================================================================

typedef struct {
  NovaTensor *features; // GPU-ready tensor
  int label;
  float weight;                 // Sample weight for balanced training
  int difficulty;               // Curriculum learning difficulty
  NovaLifetimeClass lifetime; // Memory management
} SmartSample;

typedef struct {
  SmartSample *samples;
  int sample_count;
  int input_dim;
  int num_classes;

  // Smart statistics (GPU accelerated)
  NovaTensor *feature_means;
  NovaTensor *feature_stds;
  int *class_counts;
  NovaTensor *class_weights;

  // Performance optimizations
  int max_batch_size;
  NovaArena *arena;
  NovaMemoryArena *mem_arena; // Added specialized memory arena
} SmartDataset;

// ============================================================================
// ACTIVE DATA PREPARATION PIPELINE
// ============================================================================

static SmartDataset *smart_dataset_create(NovaArena *arena, int sample_count,
                                          int input_dim, int num_classes,
                                          int max_batch_size) {
  SmartDataset *ds = nova_arena_alloc(arena, sizeof(SmartDataset));

  ds->sample_count = sample_count;
  ds->input_dim = input_dim;
  ds->num_classes = num_classes;
  ds->max_batch_size = max_batch_size;
  ds->arena = arena;

  // Allocate samples (as tensors)
  ds->samples = nova_arena_alloc(arena, sample_count * sizeof(SmartSample));
  int64_t feat_shape[] = {input_dim};

  for (int i = 0; i < sample_count; i++) {
    // Create actual tensor for features
    ds->samples[i].features =
        nova_tensor_create(NULL, feat_shape, 1, NOVA_DTYPE_FP32);
    ds->samples[i].weight = 1.0f;  // Default weight
    ds->samples[i].difficulty = 0; // Default difficulty
  }

  // Allocate statistics as tensors
  ds->feature_means =
      nova_tensor_create(NULL, feat_shape, 1, NOVA_DTYPE_FP32);
  ds->feature_stds =
      nova_tensor_create(NULL, feat_shape, 1, NOVA_DTYPE_FP32);
  ds->class_counts = nova_arena_alloc(arena, num_classes * sizeof(int));

  int64_t weight_shape[] = {num_classes};
  ds->class_weights =
      nova_tensor_create(NULL, weight_shape, 1, NOVA_DTYPE_FP32);

  return ds;
}

// ============================================================================
// INTELLIGENT FEATURE NORMALIZATION
// ============================================================================

static void smart_dataset_compute_statistics(SmartDataset *ds) {
  float *f_means = (float *)ds->feature_means->data;
  float *f_stds = (float *)ds->feature_stds->data;
  float *c_weights = (float *)ds->class_weights->data;

  // Compute feature means
  memset(f_means, 0, ds->input_dim * sizeof(float));
  memset(ds->class_counts, 0, ds->num_classes * sizeof(int));

  for (int i = 0; i < ds->sample_count; i++) {
    float *sample_f = (float *)ds->samples[i].features->data;
    // Accumulate feature sums
    for (int j = 0; j < ds->input_dim; j++) {
      f_means[j] += sample_f[j];
    }

    // Count class distribution
    ds->class_counts[ds->samples[i].label]++;
  }

  // Compute means
  for (int j = 0; j < ds->input_dim; j++) {
    f_means[j] /= ds->sample_count;
  }

  // Compute standard deviations
  memset(f_stds, 0, ds->input_dim * sizeof(float));

  for (int i = 0; i < ds->sample_count; i++) {
    float *sample_f = (float *)ds->samples[i].features->data;
    for (int j = 0; j < ds->input_dim; j++) {
      float diff = sample_f[j] - f_means[j];
      f_stds[j] += diff * diff;
    }
  }

  for (int j = 0; j < ds->input_dim; j++) {
    f_stds[j] = sqrtf(f_stds[j] / ds->sample_count + 1e-8f);
  }

  // Compute class weights for balanced loss
  int max_class_count = 0;
  for (int c = 0; c < ds->num_classes; c++) {
    if (ds->class_counts[c] > max_class_count)
      max_class_count = ds->class_counts[c];
  }

  for (int c = 0; c < ds->num_classes; c++) {
    c_weights[c] = (float)max_class_count / (ds->class_counts[c] + 1);
  }
}

// ============================================================================
// SMART BATCH BUILDER (PERFORMANCE OPTIMIZATION)
// ============================================================================

typedef struct {
  NovaTensor *input;
  NovaTensor *target;
  NovaTensor *weights; // Sample weights for balanced training
  int *sample_indices;   // Track which samples were used
  int batch_size;
} SmartBatch;

static SmartBatch smart_batch_create(NovaArena *arena, int batch_size,
                                     int input_dim, int num_classes,
                                     NovaContext *ctx) {
  SmartBatch batch;
  batch.batch_size = batch_size;

  int64_t in_shape[] = {batch_size, input_dim};
  int64_t tgt_shape[] = {batch_size, num_classes};
  int64_t w_shape[] = {batch_size, 1};

  batch.input = nova_tensor_create(ctx, in_shape, 2, NOVA_DTYPE_FP32);
  batch.target = nova_tensor_create(ctx, tgt_shape, 2, NOVA_DTYPE_FP32);
  batch.weights = nova_tensor_create(ctx, w_shape, 2, NOVA_DTYPE_FP32);
  batch.sample_indices = nova_arena_alloc(arena, batch_size * sizeof(int));

  return batch;
}

// ============================================================================
// FULL GPU-ACCELERATED SMART TRAINING
// ============================================================================

typedef struct {
  void *model;
  int model_type; // 0=MLP, 1=CNN, 2=Transformer

  // Training configuration
  int epochs;
  int batch_size;
  float learning_rate;
  float weight_decay;

  // Advanced GPU features
  int curriculum_stages;
  int current_curriculum;
  float curriculum_threshold;

  // GPU optimizations
  NovaKernelFusion *kernel_fusion;     // 2-3x speedup
  NovaFlashAttention *flash_attention; // O(N) memory
  NovaLossScaler *loss_scaler;         // FP16 stability
  NovaKVCache *kv_cache;               // For transformers

  // Performance tracking
  float *epoch_losses;
  float *epoch_accuracies;
  double *epoch_times; // GPU timing
  int current_epoch;

  // Memory management
  NovaArena *arena;
  NovaContext *ctx;
  NovaComputeContext *compute_ctx; // Added specialized compute context
  NovaMemoryArena *mem_arena;      // Added specialized memory arena
  NovaTensorPool *tensor_pool;
} SmartTrainer;

static SmartTrainer *smart_trainer_create(void *model, int model_type,
                                          int epochs, int batch_size,
                                          NovaArena *arena,
                                          NovaContext *ctx) {
  SmartTrainer *trainer = nova_arena_alloc(arena, sizeof(SmartTrainer));

  trainer->model = model;
  trainer->model_type = model_type;
  trainer->epochs = epochs;
  trainer->batch_size = batch_size;
  trainer->learning_rate = 0.001f;
  trainer->weight_decay = 0.0001f;

  trainer->curriculum_stages = 5;
  trainer->current_curriculum = 0;
  trainer->curriculum_threshold = 0.85f;

  // Initialize specialized contexts
  trainer->compute_ctx = nova_compute_init(NOVA_DEVICE_METAL_GPU);
  trainer->mem_arena =
      nova_memory_arena_create(NOVA_DEVICE_METAL_GPU, 256 * 1024 * 1024);

  // Initialize advanced GPU optimizations
  trainer->kernel_fusion = nova_kernel_fusion_create(trainer->compute_ctx);
  trainer->flash_attention =
      nova_flash_attention_create(trainer->compute_ctx, 128);
  trainer->loss_scaler =
      nova_loss_scaler_create(trainer->compute_ctx, 65536.0f);

  if (model_type == 2) { // Transformer
    trainer->kv_cache = nova_kv_cache_create(
        trainer->compute_ctx, trainer->mem_arena, 12, 2048, 768);
  }

  // Performance tracking
  trainer->epoch_losses = nova_arena_alloc(arena, epochs * sizeof(float));
  trainer->epoch_accuracies = nova_arena_alloc(arena, epochs * sizeof(float));
  trainer->epoch_times = nova_arena_alloc(arena, epochs * sizeof(double));
  trainer->current_epoch = 0;

  trainer->arena = arena;
  trainer->ctx = ctx;

  return trainer;
}

#endif // NOVA_SMART_TRAINING_H
