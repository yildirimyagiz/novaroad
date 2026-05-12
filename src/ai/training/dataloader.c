/**
 * dataloader.c - Text Dataset Loader
 * 
 * Load and batch text data for training
 */

#include "../../include/ml/nova_training.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * Create data loader from token array
 */
NovaDataLoader *nova_dataloader_create_from_tokens(
    int64_t *tokens,
    int64_t num_tokens,
    int64_t seq_length,
    int64_t batch_size
) {
    NovaDataLoader *loader = calloc(1, sizeof(NovaDataLoader));
    if (!loader) return NULL;
    
    loader->data = malloc(num_tokens * sizeof(int64_t));
    memcpy(loader->data, tokens, num_tokens * sizeof(int64_t));
    
    loader->num_tokens = num_tokens;
    loader->seq_length = seq_length;
    loader->batch_size = batch_size;
    loader->current_pos = 0;
    
    int64_t num_batches = num_tokens / (seq_length * batch_size);
    
    printf("✅ DataLoader created\n");
    printf("   Tokens: %lld\n", num_tokens);
    printf("   Batch size: %lld, Seq length: %lld\n", batch_size, seq_length);
    printf("   Num batches: %lld\n", num_batches);
    
    return loader;
}

/**
 * Create data loader from text file
 */
NovaDataLoader *nova_dataloader_create(
    const char *token_file,
    int64_t seq_length,
    int64_t batch_size
) {
    // Read token file (binary format: array of int64_t)
    FILE *f = fopen(token_file, "rb");
    if (!f) {
        fprintf(stderr, "❌ Failed to open %s\n", token_file);
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    int64_t num_tokens = file_size / sizeof(int64_t);
    
    int64_t *tokens = malloc(file_size);
    fread(tokens, sizeof(int64_t), num_tokens, f);
    fclose(f);
    
    NovaDataLoader *loader = nova_dataloader_create_from_tokens(
        tokens, num_tokens, seq_length, batch_size
    );
    
    free(tokens);  // Loader has its own copy
    
    return loader;
}

/**
 * Get next batch
 */
bool nova_dataloader_next_batch(
    NovaDataLoader *loader,
    NovaTensor **input_ids,
    NovaTensor **targets
) {
    if (!loader) return false;
    
    int64_t tokens_per_batch = loader->seq_length * loader->batch_size;
    
    // Check if we have enough tokens
    if (loader->current_pos + tokens_per_batch + 1 > loader->num_tokens) {
        return false;  // No more batches
    }
    
    // Create tensors
    int64_t batch_shape[2] = {loader->batch_size, loader->seq_length};
    
    *input_ids = nova_tensor_create(NULL, batch_shape, 2, NOVA_DTYPE_FP32);
    *targets = nova_tensor_create(NULL, batch_shape, 2, NOVA_DTYPE_FP32);
    
    float *input_data = (float *)(*input_ids)->data;
    float *target_data = (float *)(*targets)->data;
    
    // Fill batch
    for (int64_t b = 0; b < loader->batch_size; b++) {
        for (int64_t s = 0; s < loader->seq_length; s++) {
            int64_t pos = loader->current_pos + b * loader->seq_length + s;
            
            input_data[b * loader->seq_length + s] = (float)loader->data[pos];
            target_data[b * loader->seq_length + s] = (float)loader->data[pos + 1];
        }
    }
    
    loader->current_pos += tokens_per_batch;
    
    return true;
}

/**
 * Reset to beginning
 */
void nova_dataloader_reset(NovaDataLoader *loader) {
    if (loader) {
        loader->current_pos = 0;
    }
}

/**
 * Free data loader
 */
void nova_dataloader_destroy(NovaDataLoader *loader) {
    if (!loader) return;
    
    free(loader->data);
    free(loader);
}

// ═══════════════════════════════════════════════════════════════════════════
// Utility: Load text file and tokenize
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Load text file, tokenize, and create data loader
 */
NovaDataLoader *nova_dataloader_from_text_file(
    const char *text_file,
    void *tokenizer,  // SimpleTokenizer*
    int64_t seq_length,
    int64_t batch_size
) {
    // Read text file
    FILE *f = fopen(text_file, "r");
    if (!f) {
        fprintf(stderr, "❌ Failed to open %s\n", text_file);
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Read entire file
    char *text = malloc(file_size + 1);
    fread(text, 1, file_size, f);
    text[file_size] = '\0';
    fclose(f);
    
    printf("📖 Loaded text file: %ld bytes\n", file_size);
    
    // Tokenize (using simple tokenizer)
    typedef struct {
        char *vocab[256];
        int vocab_size;
        int char_to_id[256];
    } SimpleTokenizer;
    
    SimpleTokenizer *tok = (SimpleTokenizer *)tokenizer;
    
    int64_t num_tokens;
    int64_t *tokens = NULL;
    
    // Character-level tokenization
    num_tokens = file_size;
    tokens = malloc(num_tokens * sizeof(int64_t));
    
    for (int64_t i = 0; i < num_tokens; i++) {
        unsigned char c = (unsigned char)text[i];
        int token_id = tok->char_to_id[c];
        tokens[i] = token_id >= 0 ? token_id : tok->char_to_id[' '];
    }
    
    free(text);
    
    printf("🔢 Tokenized: %lld tokens\n", num_tokens);
    
    // Create data loader
    NovaDataLoader *loader = nova_dataloader_create_from_tokens(
        tokens, num_tokens, seq_length, batch_size
    );
    
    free(tokens);
    
    return loader;
}
