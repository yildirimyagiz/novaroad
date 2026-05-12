#ifndef NOVA_CNN_H
#define NOVA_CNN_H

#include "nova_advanced_optimizations.h"
#include "nova_fp16_kernels.h"
#include "nova_memory_arena.h"
#include "nova_nn.h"

// ============================================================================
// CONVOLUTIONAL NEURAL NETWORK (CNN) FOR VISION TASKS
// ============================================================================
// Designed for image classification tasks like emoji gesture recognition

// ============================================================================
// POOLING LAYER
// ============================================================================

typedef enum {
  NOVA_POOL_MAX,
  NOVA_POOL_AVERAGE,
  NOVA_POOL_ADAPTIVE_MAX,
  NOVA_POOL_ADAPTIVE_AVERAGE
} NovaPoolType;

typedef struct {
  NovaPoolType pool_type;
  int kernel_size;
  int stride;
} PoolLayer;

// ============================================================================
// COMPLETE CNN MODEL FOR EMOJI CLASSIFICATION
// ============================================================================

typedef struct {
  // Convolutional layers
  ConvLayer *conv1; // Input: 64x64x3 -> 32x32x16
  PoolLayer *pool1;

  ConvLayer *conv2; // 32x32x16 -> 16x16x32
  PoolLayer *pool2;

  ConvLayer *conv3; // 16x16x32 -> 8x8x64
  PoolLayer *pool3;

  // Fully connected layers
  LinearLayer *fc1; // 8*8*64 -> 128
  LinearLayer *fc2; // 128 -> 64
  LinearLayer *fc3; // 64 -> num_classes (12 emojis)

  int input_height;
  int input_width;
  int input_channels;
  int num_classes;

  // Training components
  NovaComputeContext *compute_ctx;
  NovaMemoryArena *arena;
} EmojiCNN;

// ============================================================================
// CNN LIFECYCLE
// ============================================================================

EmojiCNN *emoji_cnn_create(int input_height, int input_width,
                           int input_channels, int num_classes);
void emoji_cnn_free(EmojiCNN *cnn);

// Forward pass
Tensor *emoji_cnn_forward(EmojiCNN *cnn, Tensor *input);

// Training
void emoji_cnn_train_batch(EmojiCNN *cnn, Tensor *batch_images,
                           Tensor *batch_labels, NovaLossScaler *scaler,
                           NovaTensorPool *tensor_pool);

// Inference
int emoji_cnn_predict(EmojiCNN *cnn, Tensor *image);

// ============================================================================
// DATA LOADING FOR EMOJINATOR DATASET
// ============================================================================

typedef struct {
  char dataset_path[256];
  int num_classes;  // 12 emojis
  int image_height; // 64
  int image_width;  // 64
  int batch_size;

  // Dataset statistics
  int *class_counts; // Samples per class
  int total_samples;

  NovaMemoryArena *arena;
} EmojiDataset;

EmojiDataset *emoji_dataset_load(const char *dataset_path, int batch_size);
void emoji_dataset_free(EmojiDataset *dataset);

// Get batch for training
void emoji_dataset_get_batch(EmojiDataset *dataset, int batch_idx,
                             Tensor *batch_images, Tensor *batch_labels);

// ============================================================================
// REAL-TIME EMOJI CLASSIFICATION (OpenCV Integration)
// ============================================================================

typedef struct {
  EmojiCNN *cnn;
  // OpenCV integration would go here in full implementation
  int camera_width;
  int camera_height;
  int gesture_region[4]; // [x, y, w, h] for hand detection
} EmojiClassifier;

EmojiClassifier *emoji_classifier_create(const char *model_path);
void emoji_classifier_free(EmojiClassifier *classifier);

// Process camera frame and return emoji class
int emoji_classifier_process_frame(EmojiClassifier *classifier,
                                   unsigned char *frame_data, int width,
                                   int height);

// Get emoji name from class ID
const char *emoji_classifier_get_emoji_name(int class_id);

// ============================================================================
// TRAINING UTILITIES
// ============================================================================

typedef struct {
  EmojiCNN *model;
  EmojiDataset *dataset;
  AdamW *optimizer;
  NovaLossScaler *scaler;
  NovaTensorPool *tensor_pool;

  // Training config
  int epochs;
  int batch_size;
  float learning_rate;
  float validation_split;

  // Metrics
  float *train_losses;
  float *val_accuracies;
  int current_epoch;
  NovaMemoryArena *arena;
} EmojiTrainer;

EmojiTrainer *emoji_trainer_create(EmojiCNN *model, EmojiDataset *dataset,
                                   float learning_rate, int epochs,
                                   int batch_size);
void emoji_trainer_free(EmojiTrainer *trainer);

// Full training pipeline
int emoji_trainer_train(EmojiTrainer *trainer);

// Save/load trained model
int emoji_trainer_save_model(EmojiTrainer *trainer, const char *filepath);
EmojiCNN *emoji_trainer_load_model(const char *filepath,
                                   NovaMemoryArena *arena);

// ============================================================================
// PERFORMANCE METRICS
// ============================================================================

typedef struct {
  float accuracy;
  float precision;
  float recall;
  float f1_score;
  float *confusion_matrix; // [num_classes x num_classes]
} EmojiMetrics;

EmojiMetrics *emoji_evaluate_model(EmojiCNN *model, EmojiDataset *dataset);
void emoji_metrics_free(EmojiMetrics *metrics);
void emoji_metrics_print(EmojiMetrics *metrics);

#endif // NOVA_CNN_H
