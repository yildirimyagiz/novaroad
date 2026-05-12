/**
 * @file knowledge_distillation.c
 * @brief Knowledge distillation - compress large models to small ones
 * 
 * Knowledge distillation:
 * - Train small "student" model to mimic large "teacher" model
 * - Student learns from teacher's soft outputs (probabilities)
 * - Preserves ~95% of teacher's accuracy with 10-100× fewer params
 * 
 * Benefits:
 * - 10-100× smaller models
 * - 10-100× faster inference
 * - Minimal accuracy loss (<5%)
 * - Deploy large models on mobile/edge
 * 
 * Famous examples:
 * - DistilBERT (6 layers) vs BERT (12 layers): 2× faster, 60% smaller
 * - MobileNet vs ResNet-50: 10× smaller
 * - TinyBERT: 7.5× smaller, 9.4× faster
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * Distillation configuration
 */
typedef struct {
    float temperature;      // Temperature for softening outputs (typically 2-10)
    float alpha;            // Weight for distillation loss (typically 0.5-0.9)
    float beta;             // Weight for ground truth loss (1 - alpha)
    int use_hard_targets;   // Also train on ground truth labels
} DistillationConfig;

/**
 * Softmax with temperature
 * 
 * Higher temperature → softer probabilities → more information
 * Lower temperature → harder probabilities → less information
 */
void nova_softmax_with_temperature(
    const float* logits,
    float* probs,
    int n,
    float temperature)
{
    // Find max for numerical stability
    float max_logit = logits[0];
    for (int i = 1; i < n; i++) {
        if (logits[i] > max_logit) max_logit = logits[i];
    }
    
    // Compute exp(logit / T - max/T)
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        probs[i] = expf((logits[i] - max_logit) / temperature);
        sum += probs[i];
    }
    
    // Normalize
    for (int i = 0; i < n; i++) {
        probs[i] /= sum;
    }
}

/**
 * KL divergence loss between teacher and student
 * 
 * KL(P_teacher || P_student) = Σ P_teacher * log(P_teacher / P_student)
 */
float nova_kl_divergence_loss(
    const float* teacher_probs,
    const float* student_probs,
    int n)
{
    float loss = 0.0f;
    
    for (int i = 0; i < n; i++) {
        if (teacher_probs[i] > 1e-10f && student_probs[i] > 1e-10f) {
            loss += teacher_probs[i] * logf(teacher_probs[i] / student_probs[i]);
        }
    }
    
    return loss;
}

/**
 * Cross-entropy loss (for ground truth labels)
 */
float nova_cross_entropy_loss(
    const float* probs,
    int true_label,
    int n)
{
    (void)n; // unused
    return -logf(probs[true_label] + 1e-10f);
}

/**
 * Distillation loss (combined)
 * 
 * Loss = alpha * KL(teacher || student) + (1-alpha) * CE(true_label)
 */
float nova_distillation_loss(
    const float* teacher_logits,
    const float* student_logits,
    int true_label,
    int n,
    const DistillationConfig* config)
{
    // Soft targets from teacher
    float* teacher_probs = (float*)malloc(n * sizeof(float));
    nova_softmax_with_temperature(teacher_logits, teacher_probs, n,
                                  config->temperature);
    
    // Soft outputs from student
    float* student_probs_soft = (float*)malloc(n * sizeof(float));
    nova_softmax_with_temperature(student_logits, student_probs_soft, n,
                                  config->temperature);
    
    // KL divergence (distillation loss)
    float kl_loss = nova_kl_divergence_loss(teacher_probs, student_probs_soft, n);
    
    // Scale by T^2 (from distillation paper)
    kl_loss *= config->temperature * config->temperature;
    
    float total_loss = config->alpha * kl_loss;
    
    // Hard targets (ground truth)
    if (config->use_hard_targets) {
        float* student_probs_hard = (float*)malloc(n * sizeof(float));
        nova_softmax_with_temperature(student_logits, student_probs_hard, n, 1.0f);
        
        float ce_loss = nova_cross_entropy_loss(student_probs_hard, true_label, n);
        total_loss += config->beta * ce_loss;
        
        free(student_probs_hard);
    }
    
    free(teacher_probs);
    free(student_probs_soft);
    
    return total_loss;
}

/**
 * Feature-based distillation
 * 
 * Student learns to match teacher's intermediate representations
 * Often more effective than output-only distillation
 */
float nova_feature_distillation_loss(
    const float* teacher_features,  // [N] intermediate features
    const float* student_features,  // [N] intermediate features
    int N)
{
    // MSE loss between features
    float loss = 0.0f;
    
    for (int i = 0; i < N; i++) {
        float diff = teacher_features[i] - student_features[i];
        loss += diff * diff;
    }
    
    return loss / N;
}

/**
 * Attention transfer
 * 
 * Student learns to match teacher's attention maps
 * Useful for Transformers (BERT, GPT)
 */
float nova_attention_transfer_loss(
    const float* teacher_attention,     // [H×N×N] attention weights
    const float* student_attention,     // [H×N×N] attention weights
    int num_heads,
    int seq_len)
{
    int total_elements = num_heads * seq_len * seq_len;
    float loss = 0.0f;
    
    for (int i = 0; i < total_elements; i++) {
        float diff = teacher_attention[i] - student_attention[i];
        loss += diff * diff;
    }
    
    return loss / total_elements;
}

/**
 * Self-distillation
 * 
 * Model distills knowledge to itself (for model compression)
 * Useful for pruning/quantization
 */
void nova_self_distillation_training_step(
    float* model_weights,
    const float* original_weights,
    const float* gradients,
    float learning_rate,
    float distillation_weight)
{
    // Update weights with gradient descent
    // Plus regularization to stay close to original
    
    // Simple example (in practice, use optimizer)
    int n = 1000; // example
    
    for (int i = 0; i < n; i++) {
        // Normal gradient update
        model_weights[i] -= learning_rate * gradients[i];
        
        // Regularization: stay close to original
        float diff = model_weights[i] - original_weights[i];
        model_weights[i] -= learning_rate * distillation_weight * diff;
    }
}

/**
 * Example: Distill BERT-Large to BERT-Small
 */
void nova_distillation_example_bert(void) {
    printf("\n=== Knowledge Distillation Example: BERT ===\n\n");
    
    DistillationConfig config = {
        .temperature = 4.0f,
        .alpha = 0.7f,
        .beta = 0.3f,
        .use_hard_targets = 1
    };
    
    int num_classes = 1000;  // Example: 1000-way classification
    
    // Simulate teacher outputs (BERT-Large)
    float* teacher_logits = (float*)malloc(num_classes * sizeof(float));
    for (int i = 0; i < num_classes; i++) {
        teacher_logits[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
    
    // Simulate student outputs (BERT-Small)
    float* student_logits = (float*)malloc(num_classes * sizeof(float));
    for (int i = 0; i < num_classes; i++) {
        student_logits[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
    
    int true_label = 42;  // Ground truth
    
    // Compute distillation loss
    float loss = nova_distillation_loss(teacher_logits, student_logits,
                                       true_label, num_classes, &config);
    
    printf("Teacher: BERT-Large (340M parameters)\n");
    printf("Student: BERT-Small (110M parameters)\n");
    printf("Compression: 3.1× smaller\n\n");
    
    printf("Distillation config:\n");
    printf("  Temperature: %.1f\n", config.temperature);
    printf("  Alpha (distill): %.2f\n", config.alpha);
    printf("  Beta (hard): %.2f\n", config.beta);
    printf("\nLoss: %.4f\n", loss);
    
    printf("\nExpected results:\n");
    printf("  BERT-Large accuracy: 90.5%%\n");
    printf("  BERT-Small (from scratch): 85.2%%\n");
    printf("  BERT-Small (distilled): 88.7%% ✅\n");
    printf("  Speedup: 3.1× faster ✅\n");
    
    free(teacher_logits);
    free(student_logits);
}

/**
 * Example: Distill ResNet-50 to MobileNet
 */
void nova_distillation_example_mobilenet(void) {
    printf("\n=== Knowledge Distillation Example: MobileNet ===\n\n");
    
    printf("Teacher: ResNet-50\n");
    printf("  Parameters: 25.6M\n");
    printf("  FLOPs: 4.1 GFLOPS\n");
    printf("  Accuracy: 76.5%%\n\n");
    
    printf("Student: MobileNetV2\n");
    printf("  Parameters: 3.5M (7.3× smaller)\n");
    printf("  FLOPs: 0.3 GFLOPS (13.7× faster)\n");
    printf("  Accuracy (from scratch): 72.0%%\n");
    printf("  Accuracy (distilled): 74.8%% ✅\n\n");
    
    printf("Distillation gain:\n");
    printf("  From scratch: 72.0%%\n");
    printf("  Distilled: 74.8%%\n");
    printf("  Improvement: +2.8 points ✅\n");
}

/**
 * Print distillation best practices
 */
void nova_distillation_best_practices(void) {
    printf("\n=== Knowledge Distillation Best Practices ===\n\n");
    
    printf("1. Temperature:\n");
    printf("   - Too low (T=1): Student only sees hard labels\n");
    printf("   - Too high (T>10): Too much smoothing\n");
    printf("   - Recommended: T=2-5 for classification, T=1-2 for regression\n\n");
    
    printf("2. Loss weights (alpha/beta):\n");
    printf("   - alpha = 0.5-0.9 (distillation loss weight)\n");
    printf("   - beta = 0.1-0.5 (hard target weight)\n");
    printf("   - Start with alpha=0.7, beta=0.3\n\n");
    
    printf("3. Teacher model:\n");
    printf("   - Larger is better (more knowledge to transfer)\n");
    printf("   - Ensemble of teachers often best\n");
    printf("   - Teacher should be well-trained (>90%% accuracy)\n\n");
    
    printf("4. Student model:\n");
    printf("   - 5-10× smaller is typical sweet spot\n");
    printf("   - Too small: can't learn everything\n");
    printf("   - Similar architecture helps (but not required)\n\n");
    
    printf("5. Training:\n");
    printf("   - Train longer than from scratch (1.5-2× epochs)\n");
    printf("   - Lower learning rate (0.5× normal)\n");
    printf("   - Use same data augmentation as teacher\n\n");
    
    printf("6. Advanced techniques:\n");
    printf("   - Feature distillation (intermediate layers)\n");
    printf("   - Attention transfer (for Transformers)\n");
    printf("   - Layer-wise distillation\n");
    printf("   - Self-distillation (iterative compression)\n");
}

/**
 * Compare compression techniques
 */
void nova_compare_compression_techniques(void) {
    printf("\n=== Model Compression Techniques Comparison ===\n\n");
    
    printf("%-25s %-15s %-15s %-15s\n",
           "Technique", "Speedup", "Size Reduction", "Accuracy Loss");
    printf("────────────────────────────────────────────────────────────────\n");
    
    printf("%-25s %-15s %-15s %-15s\n",
           "Pruning (50%)", "2×", "50%", "<1%");
    printf("%-25s %-15s %-15s %-15s\n",
           "Pruning (90%)", "10×", "90%", "2-5%");
    printf("%-25s %-15s %-15s %-15s\n",
           "Int8 Quantization", "4×", "75%", "<1%");
    printf("%-25s %-15s %-15s %-15s\n",
           "Mixed Precision (FP16)", "2×", "50%", "<0.1%");
    printf("%-25s %-15s %-15s %-15s\n",
           "Distillation (3× smaller)", "3×", "67%", "2-5%");
    printf("%-25s %-15s %-15s %-15s\n",
           "Distillation (10× smaller)", "10×", "90%", "5-10%");
    
    printf("\n✅ Best practice: Combine techniques!\n");
    printf("   Example: Distillation → Pruning → Quantization\n");
    printf("   Result: 50-100× smaller, 20-50× faster, <5%% accuracy loss\n");
}
