/**
 * text_generation.c - Text Generation Strategies
 * 
 * Implements various sampling methods for autoregressive generation
 */

#include "../../include/ml/nova_training.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// Sampling Strategies
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Greedy sampling: argmax
 */
int64_t sample_greedy(const float *logits, int64_t vocab_size) {
    int64_t best_id = 0;
    float best_score = logits[0];
    
    for (int64_t i = 1; i < vocab_size; i++) {
        if (logits[i] > best_score) {
            best_score = logits[i];
            best_id = i;
        }
    }
    
    return best_id;
}

/**
 * Apply temperature to logits
 */
static void apply_temperature(float *logits, int64_t vocab_size, float temperature) {
    if (temperature != 1.0f) {
        for (int64_t i = 0; i < vocab_size; i++) {
            logits[i] /= temperature;
        }
    }
}

/**
 * Compute softmax probabilities
 */
static void softmax(const float *logits, float *probs, int64_t vocab_size) {
    // Find max for numerical stability
    float max_logit = logits[0];
    for (int64_t i = 1; i < vocab_size; i++) {
        if (logits[i] > max_logit) max_logit = logits[i];
    }
    
    // Compute exp and sum
    float sum = 0.0f;
    for (int64_t i = 0; i < vocab_size; i++) {
        probs[i] = expf(logits[i] - max_logit);
        sum += probs[i];
    }
    
    // Normalize
    for (int64_t i = 0; i < vocab_size; i++) {
        probs[i] /= sum;
    }
}

/**
 * Sample from probability distribution
 */
static int64_t sample_from_probs(const float *probs, int64_t vocab_size) {
    float r = (float)rand() / (float)RAND_MAX;
    float cumsum = 0.0f;
    
    for (int64_t i = 0; i < vocab_size; i++) {
        cumsum += probs[i];
        if (r < cumsum) {
            return i;
        }
    }
    
    return vocab_size - 1;
}

/**
 * Top-K sampling
 */
int64_t sample_top_k(
    const float *logits,
    int64_t vocab_size,
    int k,
    float temperature
) {
    // Create a copy for temperature scaling
    float *scaled_logits = malloc(vocab_size * sizeof(float));
    memcpy(scaled_logits, logits, vocab_size * sizeof(float));
    
    apply_temperature(scaled_logits, vocab_size, temperature);
    
    // Find top-k indices
    typedef struct { float score; int64_t id; } ScoreId;
    ScoreId *items = malloc(vocab_size * sizeof(ScoreId));
    
    for (int64_t i = 0; i < vocab_size; i++) {
        items[i].score = scaled_logits[i];
        items[i].id = i;
    }
    
    // Partial sort to get top-k (simple selection)
    for (int i = 0; i < k && i < vocab_size; i++) {
        int max_idx = i;
        for (int64_t j = i + 1; j < vocab_size; j++) {
            if (items[j].score > items[max_idx].score) {
                max_idx = j;
            }
        }
        // Swap
        ScoreId tmp = items[i];
        items[i] = items[max_idx];
        items[max_idx] = tmp;
    }
    
    // Create top-k distribution
    int actual_k = k < vocab_size ? k : vocab_size;
    float *top_k_logits = malloc(actual_k * sizeof(float));
    float *top_k_probs = malloc(actual_k * sizeof(float));
    
    for (int i = 0; i < actual_k; i++) {
        top_k_logits[i] = items[i].score;
    }
    
    softmax(top_k_logits, top_k_probs, actual_k);
    
    // Sample from top-k
    int sampled_idx = sample_from_probs(top_k_probs, actual_k);
    int64_t result = items[sampled_idx].id;
    
    free(scaled_logits);
    free(items);
    free(top_k_logits);
    free(top_k_probs);
    
    return result;
}

/**
 * Top-P (nucleus) sampling
 */
int64_t sample_top_p(
    const float *logits,
    int64_t vocab_size,
    float p,
    float temperature
) {
    // Create a copy for temperature scaling
    float *scaled_logits = malloc(vocab_size * sizeof(float));
    memcpy(scaled_logits, logits, vocab_size * sizeof(float));
    
    apply_temperature(scaled_logits, vocab_size, temperature);
    
    // Compute probabilities
    float *probs = malloc(vocab_size * sizeof(float));
    softmax(scaled_logits, probs, vocab_size);
    
    // Sort by probability (descending)
    typedef struct { float prob; int64_t id; } ProbId;
    ProbId *items = malloc(vocab_size * sizeof(ProbId));
    
    for (int64_t i = 0; i < vocab_size; i++) {
        items[i].prob = probs[i];
        items[i].id = i;
    }
    
    // Simple insertion sort (good enough for small vocab)
    for (int64_t i = 1; i < vocab_size; i++) {
        ProbId key = items[i];
        int64_t j = i - 1;
        while (j >= 0 && items[j].prob < key.prob) {
            items[j + 1] = items[j];
            j--;
        }
        items[j + 1] = key;
    }
    
    // Find nucleus (top-p)
    float cumsum = 0.0f;
    int64_t nucleus_size = 0;
    
    for (int64_t i = 0; i < vocab_size; i++) {
        cumsum += items[i].prob;
        nucleus_size++;
        if (cumsum >= p) break;
    }
    
    // Renormalize nucleus
    float nucleus_sum = 0.0f;
    for (int64_t i = 0; i < nucleus_size; i++) {
        nucleus_sum += items[i].prob;
    }
    
    float *nucleus_probs = malloc(nucleus_size * sizeof(float));
    for (int64_t i = 0; i < nucleus_size; i++) {
        nucleus_probs[i] = items[i].prob / nucleus_sum;
    }
    
    // Sample from nucleus
    int sampled_idx = sample_from_probs(nucleus_probs, nucleus_size);
    int64_t result = items[sampled_idx].id;
    
    free(scaled_logits);
    free(probs);
    free(items);
    free(nucleus_probs);
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Generation Loop
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    enum { GREEDY, TOP_K, TOP_P } strategy;
    float temperature;
    int top_k;
    float top_p;
    int max_new_tokens;
    int64_t eos_token_id;
} GenerationConfig;

/**
 * Generate text autoregressively
 */
int64_t *nova_generate(
    NovaGPTModel *model,
    const int64_t *prompt_tokens,
    int64_t prompt_length,
    const GenerationConfig *config,
    int64_t *generated_length
) {
    if (!model || !prompt_tokens || !config) return NULL;
    
    int64_t max_total = prompt_length + config->max_new_tokens;
    int64_t *tokens = malloc(max_total * sizeof(int64_t));
    
    // Copy prompt
    memcpy(tokens, prompt_tokens, prompt_length * sizeof(int64_t));
    int64_t current_length = prompt_length;
    
    printf("🔮 Generating %d tokens...\n", config->max_new_tokens);
    
    for (int step = 0; step < config->max_new_tokens; step++) {
        // Prepare input (last sequence)
        int64_t input_shape[2] = {1, current_length};
        NovaTensor *input_ids = nova_tensor_create(NULL, input_shape, 2, NOVA_DTYPE_FP32);
        
        float *input_data = (float *)input_ids->data;
        for (int64_t i = 0; i < current_length; i++) {
            input_data[i] = (float)tokens[i];
        }
        
        // Forward pass
        extern NovaTensor *nova_model_forward_complete(NovaGPTModel*, const NovaTensor*, const NovaTensor*, float*);
        NovaTensor *logits = nova_model_forward_complete(model, input_ids, NULL, NULL);
        
        // Get logits for last position
        int64_t vocab_size = model->config.vocab_size;
        float *last_logits = malloc(vocab_size * sizeof(float));
        
        float *logits_data = (float *)logits->data;
        for (int64_t v = 0; v < vocab_size; v++) {
            last_logits[v] = logits_data[(current_length - 1) * vocab_size + v];
        }
        
        // Sample next token
        int64_t next_token;
        
        switch (config->strategy) {
            case GREEDY:
                next_token = sample_greedy(last_logits, vocab_size);
                break;
            case TOP_K:
                next_token = sample_top_k(last_logits, vocab_size, 
                                         config->top_k, config->temperature);
                break;
            case TOP_P:
                next_token = sample_top_p(last_logits, vocab_size,
                                         config->top_p, config->temperature);
                break;
            default:
                next_token = sample_greedy(last_logits, vocab_size);
        }
        
        // Add to sequence
        tokens[current_length++] = next_token;
        
        // Check for EOS
        if (next_token == config->eos_token_id) {
            printf("   EOS token reached at step %d\n", step + 1);
            break;
        }
        
        if ((step + 1) % 10 == 0) {
            printf("   Generated %d/%d tokens\n", step + 1, config->max_new_tokens);
        }
        
        // Cleanup
        free(last_logits);
        nova_tensor_destroy(input_ids);
        nova_tensor_destroy(logits);
    }
    
    *generated_length = current_length;
    return tokens;
}

/**
 * Beam search (more sophisticated decoding)
 */
typedef struct {
    int64_t *tokens;
    int64_t length;
    float score;
} Beam;

int64_t *nova_generate_beam_search(
    NovaGPTModel *model,
    const int64_t *prompt_tokens,
    int64_t prompt_length,
    int num_beams,
    int max_new_tokens,
    int64_t *generated_length
) {
    // Simplified beam search
    // Full implementation would maintain num_beams hypotheses
    
    printf("⚠️  Beam search: using greedy fallback\n");
    
    GenerationConfig config = {
        .strategy = GREEDY,
        .temperature = 1.0f,
        .max_new_tokens = max_new_tokens,
        .eos_token_id = -1
    };
    
    return nova_generate(model, prompt_tokens, prompt_length, &config, generated_length);
}
