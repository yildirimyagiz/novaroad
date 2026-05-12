/**
 * nova_training.h - Neural Network Training Infrastructure
 * 
 * Complete training pipeline for GPT-style models
 */

#ifndef NOVA_TRAINING_H
#define NOVA_TRAINING_H

#include "nova_tensor.h"
#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// Model Definition
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int64_t vocab_size;
    int64_t hidden_size;
    int64_t num_layers;
    int64_t num_heads;
    int64_t num_kv_heads;         // For GQA (Grouped Query Attention)
    int64_t intermediate_size;
    int64_t max_seq_length;
    float rope_theta;
    float dropout;
    bool use_flash_attention;
} NovaModelConfig;

typedef struct {
    NovaModelConfig config;
    
    // Embeddings
    NovaTensor *token_embeddings;  // [vocab_size, hidden_size]
    
    // Transformer layers
    struct {
        // Attention
        NovaTensor *attn_norm;     // [hidden_size]
        NovaTensor *q_proj;        // [hidden_size, hidden_size]
        NovaTensor *k_proj;        // [hidden_size, kv_hidden]
        NovaTensor *v_proj;        // [hidden_size, kv_hidden]
        NovaTensor *o_proj;        // [hidden_size, hidden_size]
        
        // FFN
        NovaTensor *ffn_norm;      // [hidden_size]
        NovaTensor *gate_proj;     // [hidden_size, intermediate_size]
        NovaTensor *up_proj;       // [hidden_size, intermediate_size]
        NovaTensor *down_proj;     // [intermediate_size, hidden_size]
    } *layers;
    
    // Output
    NovaTensor *final_norm;        // [hidden_size]
    NovaTensor *lm_head;          // [hidden_size, vocab_size]
    
    int num_layers;
} NovaGPTModel;

/**
 * Create model with given configuration
 */
NovaGPTModel *nova_model_create(const NovaModelConfig *config);

/**
 * Free model and all parameters
 */
void nova_model_destroy(NovaGPTModel *model);

/**
 * Get all trainable parameters
 */
NovaTensor **nova_model_parameters(const NovaGPTModel *model, int *num_params);

/**
 * Count total parameters
 */
int64_t nova_model_num_parameters(const NovaGPTModel *model);

// ═══════════════════════════════════════════════════════════════════════════
// Forward Pass
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Forward pass through model
 * 
 * @param model Model to run
 * @param input_ids Token IDs [batch_size, seq_len]
 * @param targets Target token IDs for loss computation (optional)
 * @param loss Output loss (if targets provided)
 * @return Logits [batch_size, seq_len, vocab_size]
 */
NovaTensor *nova_model_forward(
    NovaGPTModel *model,
    const NovaTensor *input_ids,
    const NovaTensor *targets,
    float *loss
);

// ═══════════════════════════════════════════════════════════════════════════
// Backward Pass (Autograd)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Compute gradients via backpropagation
 */
void nova_model_backward(NovaTensor *loss);

/**
 * Zero all gradients
 */
void nova_model_zero_grad(NovaGPTModel *model);

// ═══════════════════════════════════════════════════════════════════════════
// Optimizer
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    float learning_rate;
    float beta1;               // Adam momentum
    float beta2;               // Adam RMSprop
    float epsilon;
    float weight_decay;
    int step;
    
    // Optimizer state (one per parameter)
    NovaTensor **momentum;     // First moment
    NovaTensor **variance;     // Second moment
    int num_params;
} NovaAdamWOptimizer;

/**
 * Create AdamW optimizer
 */
NovaAdamWOptimizer *nova_optimizer_adamw_create(
    NovaTensor **parameters,
    int num_params,
    float learning_rate,
    float weight_decay
);

/**
 * Optimizer step (update parameters)
 */
void nova_optimizer_step(NovaAdamWOptimizer *opt, NovaTensor **parameters);

/**
 * Free optimizer
 */
void nova_optimizer_destroy(NovaAdamWOptimizer *opt);

// ═══════════════════════════════════════════════════════════════════════════
// Data Loading
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int64_t *data;             // Token IDs
    int64_t num_tokens;
    int64_t seq_length;
    int64_t batch_size;
    int64_t current_pos;
} NovaDataLoader;

/**
 * Create data loader from token file
 */
NovaDataLoader *nova_dataloader_create(
    const char *token_file,
    int64_t seq_length,
    int64_t batch_size
);

/**
 * Get next batch
 * Returns input_ids and targets
 */
bool nova_dataloader_next_batch(
    NovaDataLoader *loader,
    NovaTensor **input_ids,
    NovaTensor **targets
);

/**
 * Reset loader to beginning
 */
void nova_dataloader_reset(NovaDataLoader *loader);

/**
 * Free data loader
 */
void nova_dataloader_destroy(NovaDataLoader *loader);

// ═══════════════════════════════════════════════════════════════════════════
// Training Loop
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int num_epochs;
    int64_t max_steps;
    int eval_interval;
    int save_interval;
    float gradient_clip_norm;
    bool use_amp;              // Automatic Mixed Precision
    const char *checkpoint_dir;
} NovaTrainingConfig;

typedef struct {
    NovaGPTModel *model;
    NovaAdamWOptimizer *optimizer;
    NovaDataLoader *train_loader;
    NovaDataLoader *val_loader;
    NovaTrainingConfig config;
    
    // Training state
    int current_epoch;
    int64_t current_step;
    float *train_losses;
    float *val_losses;
    int num_losses;
} NovaTrainer;

/**
 * Create trainer
 */
NovaTrainer *nova_trainer_create(
    NovaGPTModel *model,
    NovaDataLoader *train_loader,
    NovaDataLoader *val_loader,
    const NovaTrainingConfig *config
);

/**
 * Run training
 */
void nova_trainer_train(NovaTrainer *trainer);

/**
 * Evaluate model
 */
float nova_trainer_evaluate(NovaTrainer *trainer);

/**
 * Save checkpoint
 */
int nova_trainer_save_checkpoint(
    NovaTrainer *trainer,
    const char *path
);

/**
 * Load checkpoint
 */
int nova_trainer_load_checkpoint(
    NovaTrainer *trainer,
    const char *path
);

/**
 * Free trainer
 */
void nova_trainer_destroy(NovaTrainer *trainer);

// ═══════════════════════════════════════════════════════════════════════════
// Utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Print model summary
 */
void nova_model_summary(const NovaGPTModel *model);

/**
 * Estimate memory usage
 */
typedef struct {
    size_t params_bytes;
    size_t activations_bytes;
    size_t gradients_bytes;
    size_t optimizer_bytes;
    size_t total_bytes;
} NovaMemoryEstimate;

NovaMemoryEstimate nova_estimate_memory(
    const NovaModelConfig *config,
    int64_t batch_size,
    int64_t seq_length
);

#endif // NOVA_TRAINING_H
