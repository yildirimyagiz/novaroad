// ============================================================================
// NOVA TRAINING UPGRADE - STANDALONE ADVANCED SYSTEM
// ============================================================================
// Self-contained: tensor pools, streaming, stability, mixed precision
// No external dependencies - pure C implementation

#ifndef NOVA_TRAINING_UPGRADE_H
#define NOVA_TRAINING_UPGRADE_H

#include <stdbool.h> // Include stdbool for bool type
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "runtime/nova_context.h"
#include "nova_memory.h"
#include "ml/nova_tensor.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// SELF-CONTAINED TENSOR SYSTEM
// ============================================================================

typedef NovaTensor Tensor;

static inline float tensor_to_scalar(Tensor *t) {
  if (!t || !t->data)
    return 0.0f;
  return ((float *)t->data)[0];
}

// ============================================================================
// REUSABLE TENSOR POOL (ZERO ALLOCATION)
// ============================================================================

typedef enum {
  ZT_DTYPE_F32 = 0,
  ZT_DTYPE_F16 = 1,
  ZT_DTYPE_BF16 = 2
} ZTDataType;

typedef struct {
  int rows;
  int cols;
  int channels;
  int ndim;
  ZTDataType dtype;
  int device;
} TensorSpec;

typedef struct {
  Tensor **items;
  TensorSpec *specs;
  int count;
  int capacity;
  NovaArena *arena;
  NovaContext *ctx;
} TensorPool;

TensorPool *tensor_pool_create(NovaArena *arena, NovaContext *ctx,
                               int capacity);
void tensor_pool_free(TensorPool *pool);
Tensor *tensor_pool_acquire(TensorPool *pool, TensorSpec spec);
void tensor_pool_release(TensorPool *pool, Tensor *t);

// ============================================================================
// STREAMED DATA PIPELINE (DOUBLE BUFFER + PREFETCH)
// ============================================================================

typedef struct {
  Tensor *x;
  Tensor *y;
  int batch_size;
  int step_id;
} TrainBatch;

typedef struct {
  void *user_dataset;
  int input_dim;
  int num_classes;
  int batch_size;
  int shuffle;
  uint64_t rng_state;
  TensorPool *pool;

  TrainBatch buffers[2];
  int write_idx;
  int read_idx;
  int initialized;
  NovaContext *ctx;
} BatchStreamer;

typedef void (*BatchFillFn)(void *user_dataset, int batch_size, Tensor *x,
                            Tensor *y, uint64_t *rng_state);

BatchStreamer *batch_streamer_create(NovaArena *arena, void *user_dataset,
                                     int input_dim, int num_classes,
                                     int batch_size, TensorPool *pool,
                                     NovaContext *ctx);

void batch_streamer_free(BatchStreamer *s);
void batch_streamer_prefetch(BatchStreamer *s, BatchFillFn fill_fn);
TrainBatch *batch_streamer_next(BatchStreamer *s, BatchFillFn fill_fn);

// ============================================================================
// STABILITY GUARDS
// ============================================================================

typedef struct {
  float grad_clip_norm;
  float loss_scale;
  float loss_scale_up;
  float loss_scale_down;
  int loss_scale_window;
  int window_counter;
  int found_inf_recently;
} StabilityConfig;

typedef struct {
  int ok;
  int found_nan;
  int found_inf;
  float grad_norm;
} StabilityReport;

StabilityReport stability_check_and_clip(void *model, float clip_norm,
                                         NovaContext *ctx);
int stability_has_nan_or_inf_tensor(Tensor *t, NovaContext *ctx);

// ============================================================================
// MIXED PRECISION SIMULATION
// ============================================================================

typedef struct {
  int enabled;
  ZTDataType compute_dtype;
  float loss_scale;
} MixedPrecision;

void mp_init(MixedPrecision *mp, int enabled, ZTDataType dtype,
             float loss_scale);

// ============================================================================
// ADVANCED TRAINING LOOP
// ============================================================================

typedef struct {
  int epochs;
  int steps_per_epoch;
  int log_every;
  float lr;
  float weight_decay;
  int use_mixed_precision;
  ZTDataType mp_dtype;
  StabilityConfig stability;
} TrainConfig;

typedef struct {
  float last_loss;
  float ema_loss;
  int step;
  int epoch;
  int aborted;
} TrainStats;

typedef Tensor *(*ModelForwardFn)(void *model, Tensor *x, NovaContext *ctx);
typedef void (*ModelZeroGradFn)(void *model, NovaContext *ctx);

typedef struct {
  ModelForwardFn forward;
  ModelZeroGradFn zero_grad;
} ModelVTable;

TrainStats train_loop_v2(void *model, ModelVTable vtable, int model_type,
                         BatchStreamer *streamer, BatchFillFn fill_fn,
                         TrainConfig cfg, NovaContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // NOVA_TRAINING_UPGRADE_H
