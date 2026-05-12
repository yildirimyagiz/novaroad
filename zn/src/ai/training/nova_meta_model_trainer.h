#ifndef NOVA_META_MODEL_TRAINER_H
#define NOVA_META_MODEL_TRAINER_H

#include "ai/models/transformer/nova_transformer.h"
#include "nova_context.h"
#include "nova_memory.h"

// ============================================================================
// META-MODEL TRAINER: Models that Train Models
// ============================================================================
// A Transformer-based system that learns how to train other models effectively
// Meta-learning for optimal training strategies, hyperparameter selection, and
// curriculum learning

// ============================================================================
// TRAINING STATE REPRESENTATION
// ============================================================================

typedef struct {
  float loss_history[100];      // Last 100 loss values
  float gradient_norms[100];    // Gradient magnitudes
  float learning_rates[100];    // LR schedule
  int epochs_completed;         // Training progress
  float validation_metrics[10]; // Accuracy, F1, etc.
  int model_type;               // CNN, Transformer, MLP
  int dataset_complexity;       // Dataset difficulty score
} TrainingState;

typedef struct {
  TrainingState current_state;
  TrainingState target_state; // Optimal training trajectory
  float reward;               // How good the training strategy is
} MetaTrainingExample;

// ============================================================================
// META-DECISIONS (What the Meta-Model Learns)
// ============================================================================

typedef struct {
  float suggested_lr;          // Learning rate recommendation
  float lr_decay_factor;       // LR scheduling
  int batch_size;              // Optimal batch size
  int gradient_accumulation;   // Gradient accumulation steps
  float weight_decay;          // Regularization strength
  int patience;                // Early stopping patience
  float curriculum_difficulty; // Training curriculum progression
  int architecture_choice;     // Which model type to use
} MetaDecisions;

// ============================================================================
// META-MODEL ARCHITECTURE
// ============================================================================

typedef struct {
  Transformer *policy_network;     // Decides training strategies
  Transformer *value_network;      // Predicts training success
  Transformer *curriculum_network; // Manages learning curriculum

  // Experience replay for meta-learning
  MetaTrainingExample *replay_buffer;
  int replay_size;
  int replay_capacity;

  // Training statistics
  int total_meta_steps;
  float average_meta_reward;
  int successful_trainings;
} MetaModelTrainer;

// ============================================================================
// META-TRAINING DATASET
// ============================================================================

typedef struct {
  char model_name[64];       // "CNN_Emoji", "Transformer_SHAKESPEARE", etc.
  int model_type;            // CNN, TRANSFORMER, MLP
  int dataset_size;          // Training set size
  int input_dim;             // Input dimensionality
  int output_dim;            // Output dimensionality
  float optimal_lr;          // Known optimal learning rate
  int optimal_batch_size;    // Known optimal batch size
  float expected_final_loss; // Expected converged loss
} ModelTrainingTask;

typedef struct {
  ModelTrainingTask *tasks;
  int num_tasks;
  NovaArena *arena;
} MetaDataset;

// ============================================================================
// META-TRAINING API
// ============================================================================

// Lifecycle
MetaModelTrainer *meta_model_trainer_create(NovaArena *arena);
void meta_model_trainer_free(MetaModelTrainer *meta_trainer);

// Meta-learning training
int meta_train_on_task(MetaModelTrainer *meta_trainer, ModelTrainingTask *task,
                       NovaContext *ctx);

// Get optimal training strategy for a new model
MetaDecisions meta_get_training_strategy(MetaModelTrainer *meta_trainer,
                                         const char *model_description,
                                         int dataset_size, int input_dim,
                                         int output_dim);

// Execute meta-guided training
int meta_guided_training(MetaModelTrainer *meta_trainer,
                         void *model_to_train, // Generic model pointer
                         void *dataset,        // Generic dataset pointer
                         NovaContext *ctx);

// ============================================================================
// CURRICULUM LEARNING (Progressive Difficulty)
// ============================================================================

typedef struct {
  float *difficulties; // Difficulty levels for curriculum
  int *sample_counts;  // Samples per difficulty level
  int num_levels;
  int current_level;
  float progression_threshold; // When to advance difficulty
} CurriculumSchedule;

CurriculumSchedule *curriculum_create(int num_levels, NovaArena *arena);
void curriculum_update(CurriculumSchedule *curriculum,
                       float current_performance);
int curriculum_get_current_level(CurriculumSchedule *curriculum);

// ============================================================================
// HYPERPARAMETER OPTIMIZATION (Bayesian Optimization)
// ============================================================================

typedef struct {
  float *param_values; // Parameter combinations tried
  float *performances; // Performance for each combination
  int num_trials;
  int max_trials;
  NovaArena *arena;
} HyperparameterSearch;

HyperparameterSearch *hyperparameter_search_create(int max_trials,
                                                   NovaArena *arena);
void hyperparameter_search_add_trial(HyperparameterSearch *search,
                                     float *params, float performance);
float *hyperparameter_search_suggest_next(HyperparameterSearch *search);

// ============================================================================
// AUTO MODEL SELECTION
// ============================================================================

typedef struct {
  char *model_types[10]; // Available model types
  float *model_scores;   // Performance prediction for each type
  int num_models;
  NovaArena *arena;
} ModelSelector;

ModelSelector *model_selector_create(NovaArena *arena);
int model_selector_choose_best(ModelSelector *selector, int input_dim,
                               int output_dim, int dataset_size);

// ============================================================================
// META-TRAINING DEMO
// ============================================================================

void meta_model_training_demo();

#endif // NOVA_META_MODEL_TRAINER_H
