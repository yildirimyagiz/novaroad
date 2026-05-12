// ═══════════════════════════════════════════════════════════════════════════
// nova_mirror_ml.c - ML/AI Pipeline Integration for NovaMirror
// ═══════════════════════════════════════════════════════════════════════════
//
// Makes delta inference a FIRST-CLASS citizen in the ML pipeline.
// Every layer learns its own optimal threshold adaptively.

#include "nova_mirror_ml.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
static uint64_t ml_now_ns(void) {
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return mach_absolute_time() * tb.numer / tb.denom;
}
#else
static uint64_t ml_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

// ═══════════════════════════════════════════════════════════════════════════
// SIMILARITY COMPUTATION (Fast vectorized scan)
// ═══════════════════════════════════════════════════════════════════════════

static float compute_similarity_fp32(const float *a, const float *b,
                                     uint64_t count, float threshold) {
  if (!a || !b || count == 0)
    return 0.0f;

  uint64_t same = 0;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  // NEON accelerated: process 4 floats at a time
  uint64_t vec_count = count & ~3ULL;
  float32x4_t vthresh = vdupq_n_f32(threshold);
  float32x4_t vnthresh = vnegq_f32(vthresh);

  for (uint64_t i = 0; i < vec_count; i += 4) {
    float32x4_t va = vld1q_f32(a + i);
    float32x4_t vb = vld1q_f32(b + i);
    float32x4_t vdiff = vsubq_f32(va, vb);

    // |diff| <= threshold
    uint32x4_t le_pos = vcleq_f32(vdiff, vthresh);
    uint32x4_t ge_neg = vcgeq_f32(vdiff, vnthresh);
    uint32x4_t within = vandq_u32(le_pos, ge_neg);

    // Count matches
    same += (vgetq_lane_u32(within, 0) ? 1 : 0);
    same += (vgetq_lane_u32(within, 1) ? 1 : 0);
    same += (vgetq_lane_u32(within, 2) ? 1 : 0);
    same += (vgetq_lane_u32(within, 3) ? 1 : 0);
  }

  // Scalar remainder
  for (uint64_t i = vec_count; i < count; i++) {
    if (fabsf(a[i] - b[i]) <= threshold)
      same++;
  }
#else
  for (uint64_t i = 0; i < count; i++) {
    if (fabsf(a[i] - b[i]) <= threshold)
      same++;
  }
#endif

  return (float)same / (float)count;
}

// ═══════════════════════════════════════════════════════════════════════════
// DEFAULT THRESHOLDS PER LAYER TYPE
// ═══════════════════════════════════════════════════════════════════════════

static float default_delta_threshold(ZMirrorLayerType type) {
  switch (type) {
  case ZMIRROR_LAYER_LINEAR:
    return 1e-4f; // MatMul scales linearly
  case ZMIRROR_LAYER_ATTENTION:
    return 1e-5f; // Softmax amplifies — tight threshold
  case ZMIRROR_LAYER_NORM:
    return 1e-3f; // Normalization absorbs small changes
  case ZMIRROR_LAYER_ACTIVATION:
    return 1e-4f; // ReLU can zero out regions
  case ZMIRROR_LAYER_CONV2D:
    return 1e-4f; // Local receptive field
  case ZMIRROR_LAYER_EMBEDDING:
    return 0.0f; // Binary: same token or different
  case ZMIRROR_LAYER_RESIDUAL:
    return 1e-3f; // Skip connection preserves delta
  default:
    return 1e-4f;
  }
}

static float default_reuse_similarity(ZMirrorLayerType type) {
  switch (type) {
  case ZMIRROR_LAYER_ATTENTION:
    return 0.9999f; // Very cautious for attention
  case ZMIRROR_LAYER_NORM:
    return 0.998f; // Norm can tolerate more
  case ZMIRROR_LAYER_RESIDUAL:
    return 0.998f; // Residual preserves well
  default:
    return 0.999f;
  }
}

static float default_delta_similarity(ZMirrorLayerType type) {
  switch (type) {
  case ZMIRROR_LAYER_ATTENTION:
    return 0.95f; // Attention needs higher similarity for delta
  case ZMIRROR_LAYER_NORM:
    return 0.85f; // Norm absorbs → more aggressive delta
  case ZMIRROR_LAYER_ACTIVATION:
    return 0.80f; // ReLU zeros out → aggressive
  default:
    return 0.90f;
  }
}

// Estimated FLOPS per element for each layer type (rough)
static double estimated_flops_per_element(ZMirrorLayerType type) {
  switch (type) {
  case ZMIRROR_LAYER_LINEAR:
    return 2.0; // Multiply-add
  case ZMIRROR_LAYER_ATTENTION:
    return 10.0; // QKV + softmax + output
  case ZMIRROR_LAYER_NORM:
    return 5.0; // Mean + variance + normalize
  case ZMIRROR_LAYER_ACTIVATION:
    return 1.0; // Simple comparison/function
  case ZMIRROR_LAYER_CONV2D:
    return 4.0; // kernel_size^2 * channels
  case ZMIRROR_LAYER_EMBEDDING:
    return 0.1; // Just a lookup
  case ZMIRROR_LAYER_RESIDUAL:
    return 1.0; // Addition
  default:
    return 2.0;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// CREATE / DESTROY
// ═══════════════════════════════════════════════════════════════════════════

ZMirrorInferenceCache *zmirror_ml_create_cache(int num_layers) {
  ZMirrorInferenceCache *cache = calloc(1, sizeof(ZMirrorInferenceCache));
  cache->num_layers = num_layers;
  cache->layers = calloc(num_layers, sizeof(ZMirrorLayerCache));
  cache->learning_rate = 0.01f;
  cache->auto_tune_enabled = true;

  for (int i = 0; i < num_layers; i++) {
    cache->layers[i].layer_id = i;
    cache->layers[i].type = ZMIRROR_LAYER_LINEAR;
    cache->layers[i].delta_threshold = 1e-4f;
    cache->layers[i].threshold_min = 1e-7f;
    cache->layers[i].threshold_max = 1e-1f;
    cache->layers[i].sensitivity = 1.0f;
    cache->layers[i].reuse_similarity = 0.999f;
    cache->layers[i].delta_similarity = 0.90f;
  }

  return cache;
}

void zmirror_ml_destroy_cache(ZMirrorInferenceCache *cache) {
  if (!cache)
    return;
  for (int i = 0; i < cache->num_layers; i++) {
    free(cache->layers[i].cached_output);
    free(cache->layers[i].cached_input);
  }
  free(cache->layers);
  free(cache->prev_input);
  free(cache);
}

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURE LAYER
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_ml_configure_layer(ZMirrorInferenceCache *cache, int layer_id,
                                ZMirrorLayerType type, const char *name,
                                uint64_t input_size, uint64_t output_size) {
  if (!cache || layer_id < 0 || layer_id >= cache->num_layers)
    return;

  ZMirrorLayerCache *l = &cache->layers[layer_id];
  l->type = type;
  strncpy(l->name, name, sizeof(l->name) - 1);

  l->input_size = input_size;
  l->output_size = output_size;

  // Allocate cache buffers
  free(l->cached_input);
  free(l->cached_output);
  l->cached_input = calloc(1, input_size);
  l->cached_output = calloc(1, output_size);

  // Set type-specific defaults
  l->delta_threshold = default_delta_threshold(type);
  l->reuse_similarity = default_reuse_similarity(type);
  l->delta_similarity = default_delta_similarity(type);
  l->has_cached = false;
}

// ═══════════════════════════════════════════════════════════════════════════
// INFERENCE LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_ml_begin_inference(ZMirrorInferenceCache *cache,
                                const float *input, uint64_t input_size) {
  if (!cache || !input)
    return;

  // Compute global input similarity
  if (cache->prev_input && cache->input_size == input_size) {
    uint64_t elem_count = input_size / sizeof(float);
    cache->global_similarity =
        compute_similarity_fp32(cache->prev_input, input, elem_count, 1e-5f);
  } else {
    cache->global_similarity = 0.0f;
    free(cache->prev_input);
    cache->prev_input = malloc(input_size);
    cache->input_size = input_size;
  }

  // Store input for next comparison
  memcpy(cache->prev_input, input, input_size);
  cache->total_inferences++;
}

// ═══════════════════════════════════════════════════════════════════════════
// LAYER DECISION ENGINE (Core Intelligence)
// ═══════════════════════════════════════════════════════════════════════════

ZMirrorDecision zmirror_ml_layer_decision(ZMirrorInferenceCache *cache,
                                          int layer_id,
                                          const float *layer_input,
                                          uint64_t input_size) {
  if (!cache || layer_id < 0 || layer_id >= cache->num_layers)
    return ZMIRROR_DECISION_FULL;

  ZMirrorLayerCache *l = &cache->layers[layer_id];
  l->total_calls++;

  // First call → must compute fully
  if (!l->has_cached) {
    l->full_count++;
    cache->total_layers_full++;
    return ZMIRROR_DECISION_FULL;
  }

  // Compute per-layer input similarity
  uint64_t elem_count = input_size / sizeof(float);
  float similarity = compute_similarity_fp32(l->cached_input, layer_input,
                                             elem_count, l->delta_threshold);

  // Decision tree:
  //
  //   similarity >= reuse_threshold → REUSE (skip entirely)
  //   similarity >= delta_threshold → DELTA (partial update)
  //   otherwise                     → FULL  (recompute)
  //
  // Layer sensitivity adjusts thresholds:
  //   High sensitivity (attention) → more conservative (higher thresholds)
  //   Low sensitivity (norm)       → more aggressive  (lower thresholds)

  ZMirrorDecision decision;

  if (similarity >= l->reuse_similarity) {
    decision = ZMIRROR_DECISION_REUSE;
    l->reuse_count++;
    cache->total_layers_skipped++;

    // Estimate FLOPS saved: all of them
    double flops = estimated_flops_per_element(l->type) * elem_count;
    l->total_flops_saved += flops;
  } else if (similarity >= l->delta_similarity) {
    decision = ZMIRROR_DECISION_DELTA;
    l->delta_count++;
    cache->total_layers_delta++;

    // Estimate FLOPS saved: proportional to similarity
    double flops = estimated_flops_per_element(l->type) * elem_count;
    l->total_flops_saved +=
        flops * similarity; // Only saved the unchanged portion
  } else {
    decision = ZMIRROR_DECISION_FULL;
    l->full_count++;
    cache->total_layers_full++;
  }

  return decision;
}

// ═══════════════════════════════════════════════════════════════════════════
// LAYER COMMIT (Save output for next comparison)
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_ml_layer_commit(ZMirrorInferenceCache *cache, int layer_id,
                             const float *input, uint64_t input_size,
                             const float *output, uint64_t output_size) {
  if (!cache || layer_id < 0 || layer_id >= cache->num_layers)
    return;

  ZMirrorLayerCache *l = &cache->layers[layer_id];

  // Update Input Reference
  if (l->input_size != input_size) {
    free(l->cached_input);
    l->cached_input = malloc(input_size);
    l->input_size = input_size;
  }
  memcpy(l->cached_input, input, input_size);

  // Update Output Reference
  if (l->output_size != output_size) {
    free(l->cached_output);
    l->cached_output = malloc(output_size);
    l->output_size = output_size;
  }
  memcpy(l->cached_output, output, output_size);
  l->has_cached = true;
}

void zmirror_ml_end_inference(ZMirrorInferenceCache *cache) {
  (void)cache;
  // Currently a no-op, reserved for future pipeline finalization
}

// ═══════════════════════════════════════════════════════════════════════════
// GET CACHED OUTPUT
// ═══════════════════════════════════════════════════════════════════════════

const float *zmirror_ml_get_cached_output(const ZMirrorInferenceCache *cache,
                                          int layer_id) {
  if (!cache || layer_id < 0 || layer_id >= cache->num_layers)
    return NULL;
  if (!cache->layers[layer_id].has_cached)
    return NULL;
  return cache->layers[layer_id].cached_output;
}

// ═══════════════════════════════════════════════════════════════════════════
// ADAPTIVE THRESHOLD TUNING
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_ml_enable_autotune(ZMirrorInferenceCache *cache, float lr) {
  if (!cache)
    return;
  cache->auto_tune_enabled = true;
  cache->learning_rate = lr;
}

void zmirror_ml_feedback(ZMirrorInferenceCache *cache, int layer_id,
                         float actual_error) {
  if (!cache || !cache->auto_tune_enabled)
    return;
  if (layer_id < 0 || layer_id >= cache->num_layers)
    return;

  ZMirrorLayerCache *l = &cache->layers[layer_id];
  float lr = cache->learning_rate;

  l->accuracy_loss_sum += actual_error;

  // If error is high → tighten thresholds (less aggressive)
  // If error is low  → loosen thresholds (more aggressive = more savings)
  if (actual_error > 0.01f) {
    // Too aggressive: raise similarity requirements
    l->reuse_similarity += lr * (1.0f - l->reuse_similarity);
    l->delta_similarity += lr * (1.0f - l->delta_similarity);
    l->delta_threshold *= (1.0f - lr);

    // Clamp
    if (l->delta_threshold < l->threshold_min)
      l->delta_threshold = l->threshold_min;
  } else if (actual_error < 0.001f) {
    // Can be more aggressive: lower similarity requirements
    l->reuse_similarity -= lr * l->reuse_similarity * 0.1f;
    l->delta_similarity -= lr * l->delta_similarity * 0.1f;
    l->delta_threshold *= (1.0f + lr);

    // Clamp
    if (l->delta_threshold > l->threshold_max)
      l->delta_threshold = l->threshold_max;
    if (l->reuse_similarity < 0.95f)
      l->reuse_similarity = 0.95f;
    if (l->delta_similarity < 0.5f)
      l->delta_similarity = 0.5f;
  }

  // Update learned sensitivity
  l->sensitivity = l->sensitivity * 0.95f +
                   (actual_error / (l->delta_threshold + 1e-10f)) * 0.05f;
}

// ═══════════════════════════════════════════════════════════════════════════
// STATISTICS
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_ml_print_layer_stats(const ZMirrorInferenceCache *cache,
                                  int layer_id) {
  if (!cache || layer_id < 0 || layer_id >= cache->num_layers)
    return;

  const ZMirrorLayerCache *l = &cache->layers[layer_id];
  const char *type_names[] = {"Linear", "Attention", "Norm",    "Activation",
                              "Conv2D", "Embedding", "Residual"};

  double total = l->total_calls > 0 ? (double)l->total_calls : 1.0;

  printf("  │ %2d │ %-12s │ %-8s │ %5.1f%% │ %5.1f%% │ %5.1f%% │ %8.0f │\n",
         l->layer_id, l->name[0] ? l->name : type_names[l->type],
         type_names[l->type], 100.0 * l->reuse_count / total,
         100.0 * l->delta_count / total, 100.0 * l->full_count / total,
         l->total_flops_saved / 1e6);
}

void zmirror_ml_print_stats(const ZMirrorInferenceCache *cache) {
  if (!cache)
    return;

  uint64_t total_decisions = cache->total_layers_skipped +
                             cache->total_layers_delta +
                             cache->total_layers_full;
  double td = total_decisions > 0 ? (double)total_decisions : 1.0;

  printf("\n  "
         "╔═══════════════════════════════════════════════════════════════════╗"
         "\n");
  printf("  ║  NovaMirror ML Pipeline Statistics                             "
         " ║\n");
  printf("  "
         "╠═══════════════════════════════════════════════════════════════════╣"
         "\n");
  printf(
      "  ║  Total Inferences: %-10llu                                     ║\n",
      cache->total_inferences);
  printf("  ║  Global Input Similarity: %5.1f%%                                "
         " ║\n",
         cache->global_similarity * 100);
  printf("  "
         "╠═══════════════════════════════════════════════════════════════════╣"
         "\n");
  printf("  ║  Layer Decisions:                                                "
         " ║\n");
  printf("  ║    REUSE (skip):     %6llu (%5.1f%%)                             "
         " ║\n",
         cache->total_layers_skipped, 100.0 * cache->total_layers_skipped / td);
  printf("  ║    DELTA (partial):  %6llu (%5.1f%%)                             "
         " ║\n",
         cache->total_layers_delta, 100.0 * cache->total_layers_delta / td);
  printf("  ║    FULL  (compute):  %6llu (%5.1f%%)                             "
         " ║\n",
         cache->total_layers_full, 100.0 * cache->total_layers_full / td);
  printf("  "
         "╠═══════════════════════════════════════════════════════════════════╣"
         "\n");
  printf("  ║  Per-Layer Breakdown:                                            "
         " ║\n");
  printf("  "
         "├──────┬──────────────┬──────────┬────────┬────────┬────────┬────────"
         "┤\n");
  printf("  │  ID  │ Name         │ Type     │ Reuse  │ Delta  │ Full   │ "
         "MFLOPS ║\n");
  printf("  "
         "├──────┼──────────────┼──────────┼────────┼────────┼────────┼────────"
         "┤\n");

  double total_flops_saved = 0;
  for (int i = 0; i < cache->num_layers; i++) {
    zmirror_ml_print_layer_stats(cache, i);
    total_flops_saved += cache->layers[i].total_flops_saved;
  }

  printf("  "
         "╠══════╧══════════════╧══════════╧════════╧════════╧════════╧════════"
         "╣\n");
  printf("  ║  Total FLOPS Saved: %.2f GFLOPS                                  "
         " ║\n",
         total_flops_saved / 1e9);
  printf("  ║  Compute Reduction: %5.1f%%                                      "
         "  ║\n",
         100.0 *
             (cache->total_layers_skipped + cache->total_layers_delta * 0.5) /
             td);
  printf("  "
         "╚═══════════════════════════════════════════════════════════════════╝"
         "\n\n");
}
