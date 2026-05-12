/**
 * evaluation.c - Model Evaluation Metrics
 * 
 * Compute perplexity, accuracy, and other metrics
 */

#include "../../include/ml/nova_training.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// Perplexity
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Compute perplexity on a dataset
 * 
 * Perplexity = exp(average cross-entropy loss)
 * Lower is better
 */
float nova_compute_perplexity(
    NovaGPTModel *model,
    NovaDataLoader *dataloader
) {
    printf("📊 Computing perplexity...\n");
    
    nova_dataloader_reset(dataloader);
    
    float total_loss = 0.0f;
    int64_t num_batches = 0;
    
    NovaTensor *input_ids, *targets;
    
    extern NovaTensor *nova_model_forward_complete(NovaGPTModel*, const NovaTensor*, const NovaTensor*, float*);
    
    while (nova_dataloader_next_batch(dataloader, &input_ids, &targets)) {
        float batch_loss;
        NovaTensor *logits = nova_model_forward_complete(
            model, input_ids, targets, &batch_loss
        );
        
        total_loss += batch_loss;
        num_batches++;
        
        nova_tensor_destroy(input_ids);
        nova_tensor_destroy(targets);
        nova_tensor_destroy(logits);
    }
    
    float avg_loss = total_loss / (float)num_batches;
    float perplexity = expf(avg_loss);
    
    printf("   Loss: %.4f, Perplexity: %.2f\n", avg_loss, perplexity);
    
    return perplexity;
}

// ═══════════════════════════════════════════════════════════════════════════
// Accuracy
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Compute next-token prediction accuracy
 */
float nova_compute_accuracy(
    NovaGPTModel *model,
    NovaDataLoader *dataloader
) {
    printf("📊 Computing accuracy...\n");
    
    nova_dataloader_reset(dataloader);
    
    int64_t correct = 0;
    int64_t total = 0;
    
    NovaTensor *input_ids, *targets;
    
    extern NovaTensor *nova_model_forward_complete(NovaGPTModel*, const NovaTensor*, const NovaTensor*, float*);
    
    while (nova_dataloader_next_batch(dataloader, &input_ids, &targets)) {
        NovaTensor *logits = nova_model_forward_complete(
            model, input_ids, targets, NULL
        );
        
        // Get predictions (argmax)
        int64_t batch = logits->shape[0];
        int64_t seq_len = logits->shape[1];
        int64_t vocab_size = logits->shape[2];
        
        float *logits_data = (float *)logits->data;
        float *targets_data = (float *)targets->data;
        
        for (int64_t b = 0; b < batch; b++) {
            for (int64_t s = 0; s < seq_len; s++) {
                float *logit_row = logits_data + (b * seq_len + s) * vocab_size;
                
                // Find argmax
                int64_t pred_id = 0;
                float max_logit = logit_row[0];
                for (int64_t v = 1; v < vocab_size; v++) {
                    if (logit_row[v] > max_logit) {
                        max_logit = logit_row[v];
                        pred_id = v;
                    }
                }
                
                int64_t target_id = (int64_t)targets_data[b * seq_len + s];
                
                if (pred_id == target_id) {
                    correct++;
                }
                total++;
            }
        }
        
        nova_tensor_destroy(input_ids);
        nova_tensor_destroy(targets);
        nova_tensor_destroy(logits);
    }
    
    float accuracy = (float)correct / (float)total;
    printf("   Accuracy: %.2f%% (%lld/%lld)\n", accuracy * 100.0f, correct, total);
    
    return accuracy;
}

// ═══════════════════════════════════════════════════════════════════════════
// Training Metrics Tracker
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    float *train_losses;
    float *val_losses;
    float *perplexities;
    float *accuracies;
    int num_records;
    int capacity;
} MetricsTracker;

/**
 * Create metrics tracker
 */
MetricsTracker *nova_metrics_create(int initial_capacity) {
    MetricsTracker *tracker = malloc(sizeof(MetricsTracker));
    
    tracker->capacity = initial_capacity;
    tracker->num_records = 0;
    
    tracker->train_losses = malloc(initial_capacity * sizeof(float));
    tracker->val_losses = malloc(initial_capacity * sizeof(float));
    tracker->perplexities = malloc(initial_capacity * sizeof(float));
    tracker->accuracies = malloc(initial_capacity * sizeof(float));
    
    return tracker;
}

/**
 * Record metrics
 */
void nova_metrics_record(
    MetricsTracker *tracker,
    float train_loss,
    float val_loss,
    float perplexity,
    float accuracy
) {
    if (tracker->num_records >= tracker->capacity) {
        // Grow arrays
        tracker->capacity *= 2;
        tracker->train_losses = realloc(tracker->train_losses, 
                                       tracker->capacity * sizeof(float));
        tracker->val_losses = realloc(tracker->val_losses,
                                     tracker->capacity * sizeof(float));
        tracker->perplexities = realloc(tracker->perplexities,
                                       tracker->capacity * sizeof(float));
        tracker->accuracies = realloc(tracker->accuracies,
                                     tracker->capacity * sizeof(float));
    }
    
    tracker->train_losses[tracker->num_records] = train_loss;
    tracker->val_losses[tracker->num_records] = val_loss;
    tracker->perplexities[tracker->num_records] = perplexity;
    tracker->accuracies[tracker->num_records] = accuracy;
    tracker->num_records++;
}

/**
 * Print metrics summary
 */
void nova_metrics_summary(const MetricsTracker *tracker) {
    if (tracker->num_records == 0) return;
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("            TRAINING METRICS SUMMARY\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // Find best metrics
    float best_train_loss = tracker->train_losses[0];
    float best_val_loss = tracker->val_losses[0];
    float best_perplexity = tracker->perplexities[0];
    float best_accuracy = tracker->accuracies[0];
    
    for (int i = 1; i < tracker->num_records; i++) {
        if (tracker->train_losses[i] < best_train_loss) {
            best_train_loss = tracker->train_losses[i];
        }
        if (tracker->val_losses[i] < best_val_loss) {
            best_val_loss = tracker->val_losses[i];
        }
        if (tracker->perplexities[i] < best_perplexity) {
            best_perplexity = tracker->perplexities[i];
        }
        if (tracker->accuracies[i] > best_accuracy) {
            best_accuracy = tracker->accuracies[i];
        }
    }
    
    printf("Best Metrics:\n");
    printf("  Train Loss:   %.4f\n", best_train_loss);
    printf("  Val Loss:     %.4f\n", best_val_loss);
    printf("  Perplexity:   %.2f\n", best_perplexity);
    printf("  Accuracy:     %.2f%%\n", best_accuracy * 100.0f);
    
    printf("\nFinal Metrics:\n");
    int last = tracker->num_records - 1;
    printf("  Train Loss:   %.4f\n", tracker->train_losses[last]);
    printf("  Val Loss:     %.4f\n", tracker->val_losses[last]);
    printf("  Perplexity:   %.2f\n", tracker->perplexities[last]);
    printf("  Accuracy:     %.2f%%\n", tracker->accuracies[last] * 100.0f);
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
}

/**
 * Free metrics tracker
 */
void nova_metrics_destroy(MetricsTracker *tracker) {
    if (!tracker) return;
    
    free(tracker->train_losses);
    free(tracker->val_losses);
    free(tracker->perplexities);
    free(tracker->accuracies);
    free(tracker);
}
