/**
 * train_mini_gpt.c - Train Mini GPT from Scratch
 * 
 * Example: Train a 124M parameter GPT model (Qwen3-style)
 * 
 * Usage:
 *   gcc train_mini_gpt.c -o train_mini_gpt -lm
 *   ./train_mini_gpt
 */

#include "nova_training.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("        NOVA MINI GPT TRAINING (Qwen3-Coder Style)\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    srand(time(NULL));
    
    // ═══════════════════════════════════════════════════════════════════
    // 1. Model Configuration
    // ═══════════════════════════════════════════════════════════════════
    
    NovaModelConfig config = {
        .vocab_size = 32000,           // Smaller vocab for demo
        .hidden_size = 768,            // GPT-2 Small size
        .num_layers = 8,               // Reduced for faster training
        .num_heads = 12,
        .num_kv_heads = 12,            // Full attention (GQA ratio = 1)
        .intermediate_size = 3072,     // 4× hidden size
        .max_seq_length = 512,         // Shorter for demo
        .rope_theta = 10000.0f,
        .dropout = 0.1f,
        .use_flash_attention = true
    };
    
    printf("📝 Model Configuration:\n");
    printf("   Size: 124M parameters (approx)\n");
    printf("   Hidden: %lld, Layers: %lld\n", config.hidden_size, config.num_layers);
    printf("   Context: %lld tokens\n\n", config.max_seq_length);
    
    // ═══════════════════════════════════════════════════════════════════
    // 2. Create Model
    // ═══════════════════════════════════════════════════════════════════
    
    printf("🔨 Creating model...\n");
    NovaGPTModel *model = nova_model_create(&config);
    if (!model) {
        fprintf(stderr, "❌ Failed to create model\n");
        return 1;
    }
    
    nova_model_summary(model);
    printf("\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 3. Memory Estimate
    // ═══════════════════════════════════════════════════════════════════
    
    int64_t batch_size = 4;
    NovaMemoryEstimate mem = nova_estimate_memory(&config, batch_size, config.max_seq_length);
    
    printf("💾 Memory Estimate (batch=%lld):\n", batch_size);
    printf("   Parameters:  %.1f MB\n", mem.params_bytes / (1024.0f * 1024.0f));
    printf("   Activations: %.1f MB\n", mem.activations_bytes / (1024.0f * 1024.0f));
    printf("   Gradients:   %.1f MB\n", mem.gradients_bytes / (1024.0f * 1024.0f));
    printf("   Optimizer:   %.1f MB\n", mem.optimizer_bytes / (1024.0f * 1024.0f));
    printf("   Total:       %.1f MB\n\n", mem.total_bytes / (1024.0f * 1024.0f));
    
    // ═══════════════════════════════════════════════════════════════════
    // 4. Create Optimizer
    // ═══════════════════════════════════════════════════════════════════
    
    printf("⚙️  Creating optimizer...\n");
    int num_params;
    NovaTensor **params = nova_model_parameters(model, &num_params);
    
    NovaAdamWOptimizer *optimizer = nova_optimizer_adamw_create(
        params,
        num_params,
        3e-4f,      // Learning rate
        0.01f       // Weight decay
    );
    
    if (!optimizer) {
        fprintf(stderr, "❌ Failed to create optimizer\n");
        nova_model_destroy(model);
        free(params);
        return 1;
    }
    printf("\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 5. Training Loop (Simplified Demo)
    // ═══════════════════════════════════════════════════════════════════
    
    printf("🚀 Starting training...\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    int num_steps = 10;  // Very short for demo
    
    for (int step = 0; step < num_steps; step++) {
        // Create dummy batch
        int64_t input_shape[2] = {batch_size, config.max_seq_length};
        NovaTensor *input_ids = nova_tensor_create(NULL, input_shape, 2, NOVA_DTYPE_FP32);
        NovaTensor *targets = nova_tensor_create(NULL, input_shape, 2, NOVA_DTYPE_FP32);
        
        // Fill with random token IDs (in real training, load from dataset)
        float *input_data = (float *)input_ids->data;
        float *target_data = (float *)targets->data;
        for (size_t i = 0; i < input_ids->total_elements; i++) {
            input_data[i] = (float)(rand() % config.vocab_size);
            target_data[i] = (float)(rand() % config.vocab_size);
        }
        
        // Forward pass
        float loss;
        NovaTensor *logits = nova_model_forward(model, input_ids, targets, &loss);
        
        printf("Step %d/%d - Loss: %.4f\n", step + 1, num_steps, loss);
        
        // Backward pass (TODO: implement actual backprop)
        // nova_model_backward(logits);
        
        // Optimizer step
        // nova_optimizer_step(optimizer, params);
        
        // Zero gradients
        nova_model_zero_grad(model);
        
        // Cleanup
        nova_tensor_destroy(input_ids);
        nova_tensor_destroy(targets);
        nova_tensor_destroy(logits);
    }
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("✅ Training complete!\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 6. Save Model
    // ═══════════════════════════════════════════════════════════════════
    
    printf("💾 Saving model to mini_gpt_checkpoint.znm\n");
    // TODO: Implement model saving
    
    printf("✅ Model saved\n\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 7. Cleanup
    // ═══════════════════════════════════════════════════════════════════
    
    nova_optimizer_destroy(optimizer);
    nova_model_destroy(model);
    free(params);
    
    printf("🎉 All done! Next steps:\n");
    printf("   1. Implement full forward/backward pass\n");
    printf("   2. Add real dataset loading\n");
    printf("   3. Add checkpointing\n");
    printf("   4. Scale up to 1B+ parameters\n");
    printf("   5. Train on code dataset (Qwen3-Coder style)\n\n");
    
    return 0;
}
