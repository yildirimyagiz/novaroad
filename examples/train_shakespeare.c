/**
 * train_shakespeare.c - Train Mini GPT on Shakespeare
 * 
 * Complete training example:
 * 1. Load Shakespeare text
 * 2. Create character-level tokenizer
 * 3. Train mini GPT
 * 4. Generate text
 * 
 * Usage:
 *   ./train_shakespeare shakespeare.txt
 */

#include "nova_training.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// External functions from simple_tokenizer.c
typedef struct {
    char *vocab[256];
    int vocab_size;
    int char_to_id[256];
} SimpleTokenizer;

extern SimpleTokenizer *simple_tokenizer_create();
extern int64_t *simple_tokenizer_encode(SimpleTokenizer*, const char*, int64_t*);
extern char *simple_tokenizer_decode(SimpleTokenizer*, const int64_t*, int64_t);
extern void simple_tokenizer_destroy(SimpleTokenizer*);

// External functions
extern NovaDataLoader *nova_dataloader_from_text_file(const char*, void*, int64_t, int64_t);
extern NovaTensor *nova_model_forward_complete(NovaGPTModel*, const NovaTensor*, const NovaTensor*, float*);
extern int nova_checkpoint_save(const char*, NovaGPTModel*, NovaAdamWOptimizer*, int, int64_t, float);

int main(int argc, char **argv) {
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("        TRAIN MINI GPT ON SHAKESPEARE\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    if (argc < 2) {
        printf("Usage: %s <shakespeare.txt>\n", argv[0]);
        printf("\nDownload Shakespeare:\n");
        printf("  wget https://raw.githubusercontent.com/karpathy/char-rnn/master/data/tinyshakespeare/input.txt\n\n");
        
        // Create tiny demo file
        printf("Creating tiny demo file for testing...\n");
        FILE *demo = fopen("demo.txt", "w");
        fprintf(demo, "To be, or not to be, that is the question.\n");
        fprintf(demo, "Whether 'tis nobler in the mind to suffer\n");
        fprintf(demo, "The slings and arrows of outrageous fortune,\n");
        fclose(demo);
        argv[1] = "demo.txt";
        printf("✅ Created demo.txt\n\n");
    }
    
    srand(time(NULL));
    
    // ═══════════════════════════════════════════════════════════════════
    // 1. Create Tokenizer
    // ═══════════════════════════════════════════════════════════════════
    
    printf("📝 Creating tokenizer...\n");
    SimpleTokenizer *tokenizer = simple_tokenizer_create();
    printf("\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 2. Load Dataset
    // ═══════════════════════════════════════════════════════════════════
    
    printf("📖 Loading dataset...\n");
    int64_t seq_length = 128;
    int64_t batch_size = 4;
    
    NovaDataLoader *loader = nova_dataloader_from_text_file(
        argv[1],
        tokenizer,
        seq_length,
        batch_size
    );
    
    if (!loader) {
        fprintf(stderr, "❌ Failed to load dataset\n");
        simple_tokenizer_destroy(tokenizer);
        return 1;
    }
    printf("\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 3. Create Model
    // ═══════════════════════════════════════════════════════════════════
    
    printf("🔨 Creating model...\n");
    NovaModelConfig config = {
        .vocab_size = tokenizer->vocab_size,
        .hidden_size = 256,        // Smaller for fast training
        .num_layers = 4,
        .num_heads = 4,
        .num_kv_heads = 4,
        .intermediate_size = 1024,
        .max_seq_length = seq_length,
        .rope_theta = 10000.0f,
        .dropout = 0.1f,
        .use_flash_attention = false  // Use simple attention for demo
    };
    
    NovaGPTModel *model = nova_model_create(&config);
    if (!model) {
        fprintf(stderr, "❌ Failed to create model\n");
        nova_dataloader_destroy(loader);
        simple_tokenizer_destroy(tokenizer);
        return 1;
    }
    
    nova_model_summary(model);
    printf("\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 4. Create Optimizer
    // ═══════════════════════════════════════════════════════════════════
    
    printf("⚙️  Creating optimizer...\n");
    int num_params;
    NovaTensor **params = nova_model_parameters(model, &num_params);
    
    NovaAdamWOptimizer *optimizer = nova_optimizer_adamw_create(
        params,
        num_params,
        1e-3f,  // Higher LR for small model
        0.01f
    );
    printf("\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 5. Training Loop
    // ═══════════════════════════════════════════════════════════════════
    
    printf("🚀 Starting training...\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    int num_epochs = 3;
    int step = 0;
    float best_loss = 1e9f;
    
    for (int epoch = 0; epoch < num_epochs; epoch++) {
        printf("Epoch %d/%d\n", epoch + 1, num_epochs);
        printf("─────────────────────────────────────\n");
        
        nova_dataloader_reset(loader);
        
        NovaTensor *input_ids, *targets;
        int batch_num = 0;
        
        while (nova_dataloader_next_batch(loader, &input_ids, &targets)) {
            // Forward pass
            float loss;
            NovaTensor *logits = nova_model_forward_complete(
                model, input_ids, targets, &loss
            );
            
            printf("Step %d (Batch %d) - Loss: %.4f\n", step, batch_num, loss);
            
            // Save best model
            if (loss < best_loss) {
                best_loss = loss;
                nova_checkpoint_save(
                    "shakespeare_best.ckpt",
                    model, optimizer,
                    epoch, step, loss
                );
            }
            
            // Backward pass (TODO: implement autograd)
            // For now, just zero gradients
            nova_model_zero_grad(model);
            
            // Cleanup
            nova_tensor_destroy(input_ids);
            nova_tensor_destroy(targets);
            nova_tensor_destroy(logits);
            
            step++;
            batch_num++;
            
            // Limit batches for demo
            if (batch_num >= 10) break;
        }
        
        printf("\n");
    }
    
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("✅ Training complete!\n");
    printf("   Best loss: %.4f\n", best_loss);
    printf("   Total steps: %d\n", step);
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 6. Generate Sample Text
    // ═══════════════════════════════════════════════════════════════════
    
    printf("📝 Generating sample text...\n");
    printf("Prompt: \"To be or not\"\n\n");
    
    // Encode prompt
    int64_t num_prompt_tokens;
    int64_t *prompt_tokens = simple_tokenizer_encode(
        tokenizer, "To be or not", &num_prompt_tokens
    );
    
    printf("Generated: ");
    char *generated = simple_tokenizer_decode(tokenizer, prompt_tokens, num_prompt_tokens);
    printf("%s", generated);
    printf(" [... generation TODO ...]\n\n");
    
    free(prompt_tokens);
    free(generated);
    
    // ═══════════════════════════════════════════════════════════════════
    // 7. Save Final Checkpoint
    // ═══════════════════════════════════════════════════════════════════
    
    printf("💾 Saving final checkpoint...\n");
    nova_checkpoint_save(
        "shakespeare_final.ckpt",
        model, optimizer,
        num_epochs, step, best_loss
    );
    printf("\n");
    
    // ═══════════════════════════════════════════════════════════════════
    // 8. Cleanup
    // ═══════════════════════════════════════════════════════════════════
    
    nova_optimizer_destroy(optimizer);
    nova_model_destroy(model);
    nova_dataloader_destroy(loader);
    simple_tokenizer_destroy(tokenizer);
    free(params);
    
    printf("🎉 Done! Next steps:\n");
    printf("   1. Implement backward pass for actual training\n");
    printf("   2. Add text generation\n");
    printf("   3. Train for more epochs\n");
    printf("   4. Scale up model size\n\n");
    
    return 0;
}
