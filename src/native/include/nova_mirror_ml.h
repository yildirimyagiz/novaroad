/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_mirror_ml.h - ML/AI Pipeline Integration for NovaMirror
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * This module makes NovaMirror a CORE part of the inference engine:
 *
 *   1. Per-Layer Activation Caching  → Remember previous outputs
 *   2. Input Delta Detection         → How much did input change?
 *   3. Adaptive Threshold Learning   → Auto-tune sensitivity per layer
 *   4. Layer Skip Decision           → Skip, Delta Update, or Recompute
 *   5. Pipeline-Level Statistics     → Track total compute savings
 *
 * Integration Pattern:
 *
 *   cache = zmirror_ml_create_cache(num_layers);
 *   zmirror_ml_begin_inference(cache, new_input, input_size);
 *
 *   for each layer:
 *     decision = zmirror_ml_layer_decision(cache, layer_id, input);
 *     if (decision == REUSE)  → use cached output
 *     if (decision == DELTA)  → apply delta kernel
 *     if (decision == FULL)   → full recompute
 *     zmirror_ml_layer_commit(cache, layer_id, output);
 *
 *   zmirror_ml_end_inference(cache);
 */

#ifndef NOVA_MIRROR_ML_H
#define NOVA_MIRROR_ML_H

#include "nova_mirror.h"
#include "nova_tensor.h"
#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// LAYER TYPES (affects sensitivity and decision strategy)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  ZMIRROR_LAYER_LINEAR,     // MatMul - scales delta linearly
  ZMIRROR_LAYER_ATTENTION,  // Softmax amplifies small changes
  ZMIRROR_LAYER_NORM,       // LayerNorm/BatchNorm - normalizes delta
  ZMIRROR_LAYER_ACTIVATION, // ReLU/GELU/SiLU - can zero out delta
  ZMIRROR_LAYER_CONV2D,     // Conv - local receptive field
  ZMIRROR_LAYER_EMBEDDING,  // Lookup - binary same/different
  ZMIRROR_LAYER_RESIDUAL,   // Skip connection - preserves delta
} ZMirrorLayerType;

// ═══════════════════════════════════════════════════════════════════════════
// DECISION: What to do for each layer
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  ZMIRROR_DECISION_FULL,  // Recompute everything (input changed too much)
  ZMIRROR_DECISION_DELTA, // Apply delta kernel (partial change)
  ZMIRROR_DECISION_REUSE, // Skip entirely (input didn't change enough)
} ZMirrorDecision;

// ═══════════════════════════════════════════════════════════════════════════
// PER-LAYER CACHE ENTRY
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  // Layer metadata
  int layer_id;
  ZMirrorLayerType type;
  char name[64];

  // Cached activation (previous output)
  float *cached_output;
  uint64_t output_size;    // in bytes
  int64_t output_shape[4]; // up to 4D tensor
  int output_ndim;

  // Cached input (for delta computation)
  float *cached_input;
  uint64_t input_size;

  // Adaptive threshold (learned from data)
  float delta_threshold; // Current threshold
  float threshold_min;   // Lower bound
  float threshold_max;   // Upper bound
  float sensitivity;     // How much this layer amplifies delta (learned)

  // Running statistics
  uint64_t total_calls;
  uint64_t reuse_count;     // Times we skipped
  uint64_t delta_count;     // Times we used delta
  uint64_t full_count;      // Times we recomputed
  double total_flops_saved; // Estimated FLOPS avoided
  double accuracy_loss_sum; // Accumulated error from delta/reuse

  // Decision thresholds (auto-tuned)
  float reuse_similarity; // Above this → REUSE (default 0.999)
  float delta_similarity; // Above this → DELTA (default 0.90)
  // Below delta_similarity → FULL recompute

  bool has_cached;
} ZMirrorLayerCache;

// ═══════════════════════════════════════════════════════════════════════════
// INFERENCE CACHE (holds all layers)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  ZMirrorLayerCache *layers;
  int num_layers;

  // Global input tracking
  float *prev_input;
  uint64_t input_size;
  float global_similarity; // Overall input similarity

  // Pipeline statistics
  uint64_t total_inferences;
  uint64_t total_layers_skipped;
  uint64_t total_layers_delta;
  uint64_t total_layers_full;
  double total_compute_time_ms;
  double total_saved_time_ms;

  // Adaptive learning
  float learning_rate; // For threshold adjustment
  bool auto_tune_enabled;
} ZMirrorInferenceCache;

// ═══════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════════════

// Create/destroy inference cache
ZMirrorInferenceCache *zmirror_ml_create_cache(int num_layers);
void zmirror_ml_destroy_cache(ZMirrorInferenceCache *cache);

// Configure layers
void zmirror_ml_configure_layer(ZMirrorInferenceCache *cache, int layer_id,
                                ZMirrorLayerType type, const char *name,
                                uint64_t input_size, uint64_t output_size);

// Inference lifecycle
void zmirror_ml_begin_inference(ZMirrorInferenceCache *cache,
                                const float *input, uint64_t input_size);

ZMirrorDecision zmirror_ml_layer_decision(ZMirrorInferenceCache *cache,
                                          int layer_id,
                                          const float *layer_input,
                                          uint64_t input_size);

void zmirror_ml_layer_commit(ZMirrorInferenceCache *cache, int layer_id,
                             const float *input, uint64_t input_size,
                             const float *output, uint64_t output_size);

void zmirror_ml_end_inference(ZMirrorInferenceCache *cache);

// Get cached output (when REUSE decision)
const float *zmirror_ml_get_cached_output(const ZMirrorInferenceCache *cache,
                                          int layer_id);

// Statistics
void zmirror_ml_print_stats(const ZMirrorInferenceCache *cache);
void zmirror_ml_print_layer_stats(const ZMirrorInferenceCache *cache,
                                  int layer_id);

// Adaptive tuning
void zmirror_ml_enable_autotune(ZMirrorInferenceCache *cache,
                                float learning_rate);
void zmirror_ml_feedback(ZMirrorInferenceCache *cache, int layer_id,
                         float actual_error); // Error from delta/reuse vs full

#endif // NOVA_MIRROR_ML_H
