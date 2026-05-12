#include "federated_learning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

static uint64_t get_timestamp_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void federated_init(FederatedLearning *fl, ModelType type) {
    printf("🧠 Initializing Federated Learning...\n");
    
    memset(fl, 0, sizeof(FederatedLearning));
    
    // Initialize local model
    snprintf(fl->local_model.model_id, MODEL_VERSION_SIZE, "model-local-%llu", get_timestamp_ms());
    fl->local_model.type = type;
    fl->local_model.status = TRAINING_IDLE;
    fl->local_model.version = 1;
    fl->local_model.round = 0;
    fl->local_model.participant_count = 1;
    fl->local_model.accuracy = 0.0;
    fl->local_model.loss = 1.0;
    
    // Initialize global model
    snprintf(fl->global_model.model_id, MODEL_VERSION_SIZE, "model-global-%llu", get_timestamp_ms());
    fl->global_model.type = type;
    fl->global_model.status = TRAINING_IDLE;
    fl->global_model.version = 1;
    fl->global_model.round = 0;
    fl->global_model.participant_count = 0;
    
    // Initialize privacy parameters
    fl->privacy.epsilon = EPSILON;
    fl->privacy.delta = DELTA;
    fl->privacy.max_gradient_norm = MAX_GRADIENT_NORM;
    fl->privacy.enabled = true;
    
    fl->is_initialized = true;
    fl->sync_count = 0;
    
    printf("  ✅ Model type: %d\n", type);
    printf("  ✅ Privacy enabled: %s\n", fl->privacy.enabled ? "yes" : "no");
    printf("  ✅ Federated Learning initialized\n");
}

void federated_train_local(FederatedLearning *fl, const float *features, uint32_t feature_count, const float *labels, uint32_t label_count) {
    printf("🎓 Training local model on %d samples...\n", label_count);
    
    fl->local_model.status = TRAINING_ACTIVE;
    
    // Simulate local training
    uint32_t weights_to_update = feature_count < MAX_WEIGHTS ? feature_count : MAX_WEIGHTS;
    
    for (uint32_t i = 0; i < weights_to_update; i++) {
        // Simulate weight update based on features
        fl->local_model.weights.weights[i] = features[i % feature_count] * 0.1f;
    }
    
    fl->local_model.weights.weight_count = weights_to_update;
    fl->local_model.weights.last_updated = get_timestamp_ms();
    
    // Calculate simulated loss
    float total_loss = 0.0f;
    for (uint32_t i = 0; i < label_count; i++) {
        float prediction = fl->local_model.weights.weights[i % weights_to_update];
        float error = labels[i] - prediction;
        total_loss += error * error;
    }
    fl->local_model.loss = total_loss / label_count;
    
    // Calculate simulated accuracy
    uint32_t correct = 0;
    for (uint32_t i = 0; i < label_count; i++) {
        float prediction = fl->local_model.weights.weights[i % weights_to_update];
        if (fabsf(prediction - labels[i]) < 0.5f) {
            correct++;
        }
    }
    fl->local_model.accuracy = (double)correct / label_count;
    
    // Extract gradients
    for (uint32_t i = 0; i < weights_to_update; i++) {
        fl->local_gradients.gradients[i] = (labels[i % label_count] - fl->local_model.weights.weights[i]) * LEARNING_RATE;
    }
    fl->local_gradients.gradient_count = weights_to_update;
    fl->local_gradients.loss = fl->local_model.loss;
    fl->local_gradients.sample_count = label_count;
    fl->local_gradients.is_private = false;
    
    fl->local_model.status = TRAINING_IDLE;
    fl->local_model.round++;
    
    printf("  ✅ Training complete: loss=%.4f, accuracy=%.2f%%\n", 
           fl->local_model.loss, fl->local_model.accuracy * 100);
}

void federated_update_model(FederatedLearning *fl, const GradientInfo *gradients) {
    printf("📊 Updating local model with gradients...\n");
    
    // Apply gradients to local model
    for (uint32_t i = 0; i < gradients->gradient_count && i < MAX_WEIGHTS; i++) {
        fl->local_model.weights.weights[i] -= gradients->gradients[i] * LEARNING_RATE;
    }
    
    fl->local_model.weights.last_updated = get_timestamp_ms();
    fl->local_model.version++;
    
    printf("  ✅ Model updated to version %d\n", fl->local_model.version);
}

void federated_aggregate_gradients(FederatedLearning *fl, GradientInfo *gradients[], uint32_t count) {
    printf("🔄 Aggregating gradients from %d participants...\n", count);
    
    if (count < MIN_PARTICIPANTS) {
        printf("  ⚠️  Not enough participants (need %d, got %d)\n", MIN_PARTICIPANTS, count);
        return;
    }
    
    // Federated averaging
    memset(fl->aggregated_gradients.gradients, 0, sizeof(fl->aggregated_gradients.gradients));
    fl->aggregated_gradients.gradient_count = 0;
    fl->aggregated_gradients.loss = 0.0f;
    fl->aggregated_gradients.sample_count = 0;
    
    uint32_t max_gradients = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (gradients[i]->gradient_count > max_gradients) {
            max_gradients = gradients[i]->gradient_count;
        }
    }
    
    for (uint32_t i = 0; i < max_gradients && i < MAX_GRADIENTS; i++) {
        float sum = 0.0f;
        uint32_t valid_count = 0;
        
        for (uint32_t j = 0; j < count; j++) {
            if (i < gradients[j]->gradient_count) {
                sum += gradients[j]->gradients[i];
                valid_count++;
            }
        }
        
        if (valid_count > 0) {
            fl->aggregated_gradients.gradients[i] = sum / valid_count;
        }
    }
    
    fl->aggregated_gradients.gradient_count = max_gradients;
    
    // Average loss
    float total_loss = 0.0f;
    uint32_t total_samples = 0;
    for (uint32_t i = 0; i < count; i++) {
        total_loss += gradients[i]->loss;
        total_samples += gradients[i]->sample_count;
    }
    fl->aggregated_gradients.loss = total_loss / count;
    fl->aggregated_gradients.sample_count = total_samples;
    
    printf("  ✅ Aggregated %d gradients, loss=%.4f\n", 
           fl->aggregated_gradients.gradient_count, fl->aggregated_gradients.loss);
}

void federated_apply_privacy(FederatedLearning *fl, GradientInfo *gradients) {
    if (!fl->privacy.enabled) {
        return;
    }
    
    printf("🔒 Applying differential privacy (ε=%.2f, δ=%.2f)...\n", 
           fl->privacy.epsilon, fl->privacy.delta);
    
    // Gradient clipping
    float norm = 0.0f;
    for (uint32_t i = 0; i < gradients->gradient_count; i++) {
        norm += gradients->gradients[i] * gradients->gradients[i];
    }
    norm = sqrtf(norm);
    
    if (norm > fl->privacy.max_gradient_norm) {
        float scale = fl->privacy.max_gradient_norm / norm;
        for (uint32_t i = 0; i < gradients->gradient_count; i++) {
            gradients->gradients[i] *= scale;
        }
    }
    
    // Add noise (simplified - in production use proper DP mechanisms)
    for (uint32_t i = 0; i < gradients->gradient_count; i++) {
        double noise = ((double)rand() / RAND_MAX - 0.5) * 0.01;
        gradients->gradients[i] += noise;
    }
    
    gradients->is_private = true;
    
    printf("  ✅ Privacy applied\n");
}

void federated_sync_global(FederatedLearning *fl) {
    printf("📤 Preparing local gradients for global sync...\n");
    
    // Apply privacy to local gradients
    federated_apply_privacy(fl, &fl->local_gradients);
    
    // In a real implementation, this would send gradients to the server
    printf("  ✅ Local gradients ready for sync\n");
    printf("  📊 Gradient count: %d\n", fl->local_gradients.gradient_count);
    printf("  📊 Loss: %.4f\n", fl->local_gradients.loss);
    printf("  📊 Samples: %d\n", fl->local_gradients.sample_count);
    
    fl->sync_count++;
}

void federated_receive_global(FederatedLearning *fl, const FederatedModel *global) {
    printf("📥 Receiving global model update...\n");
    
    // Update local model with global weights
    memcpy(&fl->global_model, global, sizeof(FederatedModel));
    
    // Apply global weights to local model
    for (uint32_t i = 0; i < global->weights.weight_count && i < MAX_WEIGHTS; i++) {
        fl->local_model.weights.weights[i] = global->weights.weights[i];
    }
    fl->local_model.weights.weight_count = global->weights.weight_count;
    
    fl->local_model.version = global->version;
    fl->local_model.round = global->round;
    
    printf("  ✅ Global model received: version %d, round %d\n", 
           global->version, global->round);
    printf("  📊 Global accuracy: %.2f%%\n", global->accuracy * 100);
}

bool federated_should_sync(FederatedLearning *fl) {
    return fl->local_model.round % ROUNDS_PER_SYNC == 0 && fl->local_model.round > 0;
}

void federated_shutdown(FederatedLearning *fl) {
    printf("🛑 Shutting down Federated Learning...\n");
    
    fl->local_model.status = TRAINING_IDLE;
    fl->is_initialized = false;
    
    printf("  ✅ Total syncs: %d\n", fl->sync_count);
    printf("  ✅ Final accuracy: %.2f%%\n", fl->local_model.accuracy * 100);
    printf("  ✅ Federated Learning shutdown complete\n");
}

FederatedModel* federated_get_local_model(FederatedLearning *fl) {
    return &fl->local_model;
}

FederatedModel* federated_get_global_model(FederatedLearning *fl) {
    return &fl->global_model;
}
