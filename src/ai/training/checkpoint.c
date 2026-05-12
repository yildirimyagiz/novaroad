/**
 * checkpoint.c - Model Checkpoint Save/Load
 * 
 * Save and load model weights, optimizer state, training state
 */

#include "../../include/ml/nova_training.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CHECKPOINT_MAGIC 0x4E4F5641  // "NOVA"
#define CHECKPOINT_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    int64_t num_params;
    int epoch;
    int64_t step;
    float best_loss;
} CheckpointHeader;

/**
 * Save model checkpoint
 */
int nova_checkpoint_save(
    const char *path,
    NovaGPTModel *model,
    NovaAdamWOptimizer *optimizer,
    int epoch,
    int64_t step,
    float loss
) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "❌ Failed to open %s for writing\n", path);
        return -1;
    }
    
    // Write header
    CheckpointHeader header = {
        .magic = CHECKPOINT_MAGIC,
        .version = CHECKPOINT_VERSION,
        .num_params = 0,
        .epoch = epoch,
        .step = step,
        .best_loss = loss
    };
    
    int num_params;
    NovaTensor **params = nova_model_parameters(model, &num_params);
    header.num_params = num_params;
    
    fwrite(&header, sizeof(CheckpointHeader), 1, f);
    
    // Write model config
    fwrite(&model->config, sizeof(NovaModelConfig), 1, f);
    
    // Write each parameter
    for (int i = 0; i < num_params; i++) {
        NovaTensor *param = params[i];
        
        // Write tensor metadata
        fwrite(&param->ndim, sizeof(int), 1, f);
        fwrite(param->shape, sizeof(int64_t), param->ndim, f);
        fwrite(&param->total_elements, sizeof(size_t), 1, f);
        
        // Write tensor data
        fwrite(param->data, sizeof(float), param->total_elements, f);
    }
    
    // Write optimizer state (if provided)
    if (optimizer) {
        fwrite(&optimizer->step, sizeof(int), 1, f);
        fwrite(&optimizer->learning_rate, sizeof(float), 1, f);
        
        // Write momentum and variance
        for (int i = 0; i < num_params; i++) {
            fwrite(optimizer->momentum[i]->data, sizeof(float),
                   optimizer->momentum[i]->total_elements, f);
            fwrite(optimizer->variance[i]->data, sizeof(float),
                   optimizer->variance[i]->total_elements, f);
        }
    }
    
    fclose(f);
    free(params);
    
    printf("✅ Checkpoint saved: %s\n", path);
    printf("   Epoch: %d, Step: %lld, Loss: %.4f\n", epoch, step, loss);
    
    return 0;
}

/**
 * Load model checkpoint
 */
int nova_checkpoint_load(
    const char *path,
    NovaGPTModel **model_out,
    NovaAdamWOptimizer **optimizer_out,
    int *epoch_out,
    int64_t *step_out,
    float *loss_out
) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "❌ Failed to open %s for reading\n", path);
        return -1;
    }
    
    // Read header
    CheckpointHeader header;
    fread(&header, sizeof(CheckpointHeader), 1, f);
    
    if (header.magic != CHECKPOINT_MAGIC) {
        fprintf(stderr, "❌ Invalid checkpoint magic\n");
        fclose(f);
        return -1;
    }
    
    // Read model config
    NovaModelConfig config;
    fread(&config, sizeof(NovaModelConfig), 1, f);
    
    // Create model
    NovaGPTModel *model = nova_model_create(&config);
    if (!model) {
        fclose(f);
        return -1;
    }
    
    int num_params;
    NovaTensor **params = nova_model_parameters(model, &num_params);
    
    if (num_params != header.num_params) {
        fprintf(stderr, "❌ Parameter count mismatch\n");
        nova_model_destroy(model);
        free(params);
        fclose(f);
        return -1;
    }
    
    // Read each parameter
    for (int i = 0; i < num_params; i++) {
        int ndim;
        fread(&ndim, sizeof(int), 1, f);
        
        int64_t shape[8];
        fread(shape, sizeof(int64_t), ndim, f);
        
        size_t total_elements;
        fread(&total_elements, sizeof(size_t), 1, f);
        
        // Read data
        fread(params[i]->data, sizeof(float), total_elements, f);
    }
    
    // Read optimizer state (optional)
    if (optimizer_out && !feof(f)) {
        NovaAdamWOptimizer *opt = nova_optimizer_adamw_create(
            params, num_params, 3e-4f, 0.01f
        );
        
        fread(&opt->step, sizeof(int), 1, f);
        fread(&opt->learning_rate, sizeof(float), 1, f);
        
        for (int i = 0; i < num_params; i++) {
            fread(opt->momentum[i]->data, sizeof(float),
                  opt->momentum[i]->total_elements, f);
            fread(opt->variance[i]->data, sizeof(float),
                  opt->variance[i]->total_elements, f);
        }
        
        *optimizer_out = opt;
    }
    
    fclose(f);
    free(params);
    
    *model_out = model;
    if (epoch_out) *epoch_out = header.epoch;
    if (step_out) *step_out = header.step;
    if (loss_out) *loss_out = header.best_loss;
    
    printf("✅ Checkpoint loaded: %s\n", path);
    printf("   Epoch: %d, Step: %lld, Loss: %.4f\n",
           header.epoch, header.step, header.best_loss);
    
    return 0;
}
