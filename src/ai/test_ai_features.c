/**
 * @file test_ai_features.c
 * @brief Comprehensive test for all AI features
 */

#include "ai/tensor.h"
#include "../nn/pooling.h"
#include "../nn/normalization.h"
#include "../nn/loss.h"
#include "../nn/optim.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TEST_PASS "\033[32m✓ PASS\033[0m"
#define TEST_FAIL "\033[31m✗ FAIL\033[0m"

int test_pooling_layers(void) {
    printf("\n=== Testing Pooling Layers ===\n");
    
    // Create test input (1, 1, 4, 4)
    size_t shape[] = {1, 1, 4, 4};
    nova_tensor_t* input = nova_tensor_zeros(shape, 4, NOVA_DTYPE_FLOAT32);
    float* data = (float*)input->data;
    
    // Fill with test pattern
    for (int i = 0; i < 16; i++) {
        data[i] = (float)i;
    }
    
    // Test MaxPool2D
    nova_tensor_t* max_pooled = nova_pool2d_max(input, 2, 2, 0);
    if (max_pooled && max_pooled->shape[2] == 2 && max_pooled->shape[3] == 2) {
        printf("  MaxPool2D: %s\n", TEST_PASS);
    } else {
        printf("  MaxPool2D: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test AvgPool2D
    nova_tensor_t* avg_pooled = nova_pool2d_avg(input, 2, 2, 0);
    if (avg_pooled && avg_pooled->shape[2] == 2 && avg_pooled->shape[3] == 2) {
        printf("  AvgPool2D: %s\n", TEST_PASS);
    } else {
        printf("  AvgPool2D: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test AdaptiveAvgPool
    nova_tensor_t* adaptive = nova_pool2d_adaptive_avg(input, 1, 1);
    if (adaptive && adaptive->shape[2] == 1 && adaptive->shape[3] == 1) {
        printf("  AdaptiveAvgPool2D: %s\n", TEST_PASS);
    } else {
        printf("  AdaptiveAvgPool2D: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test GlobalAvgPool
    nova_tensor_t* global = nova_pool2d_global_avg(input);
    if (global && global->shape[2] == 1 && global->shape[3] == 1) {
        printf("  GlobalAvgPool2D: %s\n", TEST_PASS);
    } else {
        printf("  GlobalAvgPool2D: %s\n", TEST_FAIL);
        return -1;
    }
    
    nova_tensor_free(input);
    nova_tensor_free(max_pooled);
    nova_tensor_free(avg_pooled);
    nova_tensor_free(adaptive);
    nova_tensor_free(global);
    
    return 0;
}

int test_normalization_layers(void) {
    printf("\n=== Testing Normalization Layers ===\n");
    
    // Create test input
    size_t shape[] = {2, 4, 8, 8};
    nova_tensor_t* input = nova_tensor_randn(shape, 4, NOVA_DTYPE_FLOAT32);
    
    // Test LayerNorm
    nova_tensor_t* layer_normed = nova_layer_norm(input, 1, 1e-5f);
    if (layer_normed) {
        printf("  LayerNorm: %s\n", TEST_PASS);
        nova_tensor_free(layer_normed);
    } else {
        printf("  LayerNorm: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test BatchNorm
    size_t channel_shape[] = {4};
    nova_tensor_t* running_mean = nova_tensor_zeros(channel_shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t* running_var = nova_tensor_ones(channel_shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t* gamma = nova_tensor_ones(channel_shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t* beta = nova_tensor_zeros(channel_shape, 1, NOVA_DTYPE_FLOAT32);
    
    nova_tensor_t* batch_normed = nova_batch_norm(input, running_mean, running_var, 
                                                   gamma, beta, 1e-5f, 0.1f, true);
    if (batch_normed) {
        printf("  BatchNorm: %s\n", TEST_PASS);
        nova_tensor_free(batch_normed);
    } else {
        printf("  BatchNorm: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test GroupNorm
    nova_tensor_t* group_normed = nova_group_norm(input, 2, 1e-5f);
    if (group_normed) {
        printf("  GroupNorm: %s\n", TEST_PASS);
        nova_tensor_free(group_normed);
    } else {
        printf("  GroupNorm: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test RMSNorm
    nova_tensor_t* rms_normed = nova_rms_norm(input, 1e-5f);
    if (rms_normed) {
        printf("  RMSNorm: %s\n", TEST_PASS);
        nova_tensor_free(rms_normed);
    } else {
        printf("  RMSNorm: %s\n", TEST_FAIL);
        return -1;
    }
    
    nova_tensor_free(input);
    nova_tensor_free(running_mean);
    nova_tensor_free(running_var);
    nova_tensor_free(gamma);
    nova_tensor_free(beta);
    
    return 0;
}

int test_loss_functions(void) {
    printf("\n=== Testing Loss Functions ===\n");
    
    // Create test tensors
    size_t shape[] = {4, 10};
    nova_tensor_t* predictions = nova_tensor_randn(shape, 2, NOVA_DTYPE_FLOAT32);
    nova_tensor_t* targets = nova_tensor_zeros(shape, 2, NOVA_DTYPE_FLOAT32);
    
    // Test MSE Loss
    float mse = nova_loss_mse(predictions, targets);
    if (mse >= 0.0f) {
        printf("  MSE Loss: %s (value: %.4f)\n", TEST_PASS, mse);
    } else {
        printf("  MSE Loss: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test MAE Loss
    float mae = nova_loss_mae(predictions, targets);
    if (mae >= 0.0f) {
        printf("  MAE Loss: %s (value: %.4f)\n", TEST_PASS, mae);
    } else {
        printf("  MAE Loss: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test Cross Entropy Loss
    size_t class_shape[] = {4};
    nova_tensor_t* class_targets = nova_tensor_zeros(class_shape, 1, NOVA_DTYPE_FLOAT32);
    float ce = nova_loss_cross_entropy(predictions, class_targets);
    if (ce >= 0.0f) {
        printf("  CrossEntropy Loss: %s (value: %.4f)\n", TEST_PASS, ce);
    } else {
        printf("  CrossEntropy Loss: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test Binary Cross Entropy with Logits
    float bce = nova_loss_bce_with_logits(predictions, targets);
    if (bce >= 0.0f) {
        printf("  BCE with Logits: %s (value: %.4f)\n", TEST_PASS, bce);
    } else {
        printf("  BCE with Logits: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test Huber Loss
    float huber = nova_loss_huber(predictions, targets, 1.0f);
    if (huber >= 0.0f) {
        printf("  Huber Loss: %s (value: %.4f)\n", TEST_PASS, huber);
    } else {
        printf("  Huber Loss: %s\n", TEST_FAIL);
        return -1;
    }
    
    nova_tensor_free(predictions);
    nova_tensor_free(targets);
    nova_tensor_free(class_targets);
    
    return 0;
}

int test_optimizers(void) {
    printf("\n=== Testing Optimizers ===\n");
    
    // Create test parameters and gradients
    size_t shape[] = {10, 10};
    nova_tensor_t* param = nova_tensor_randn(shape, 2, NOVA_DTYPE_FLOAT32);
    nova_tensor_t* grad = nova_tensor_randn(shape, 2, NOVA_DTYPE_FLOAT32);
    
    nova_tensor_t* params[] = {param};
    nova_tensor_t* grads[] = {grad};
    
    // Test SGD
    nova_optimizer_t* sgd = nova_optim_sgd(0.01f, 0.0f, 0.0f, false);
    if (sgd) {
        nova_optim_step(sgd, params, grads, 1);
        printf("  SGD: %s\n", TEST_PASS);
        nova_optim_free(sgd);
    } else {
        printf("  SGD: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test SGD with Momentum
    nova_optimizer_t* momentum = nova_optim_sgd(0.01f, 0.9f, 0.0f, false);
    if (momentum) {
        nova_optim_step(momentum, params, grads, 1);
        printf("  SGD with Momentum: %s\n", TEST_PASS);
        nova_optim_free(momentum);
    } else {
        printf("  SGD with Momentum: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test Adam
    nova_optimizer_t* adam = nova_optim_adam(0.001f, 0.9f, 0.999f, 1e-8f, 0.0f);
    if (adam) {
        nova_optim_step(adam, params, grads, 1);
        printf("  Adam: %s\n", TEST_PASS);
        nova_optim_free(adam);
    } else {
        printf("  Adam: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test AdamW
    nova_optimizer_t* adamw = nova_optim_adamw(0.001f, 0.9f, 0.999f, 1e-8f, 0.01f);
    if (adamw) {
        nova_optim_step(adamw, params, grads, 1);
        printf("  AdamW: %s\n", TEST_PASS);
        nova_optim_free(adamw);
    } else {
        printf("  AdamW: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test RMSprop
    nova_optimizer_t* rmsprop = nova_optim_rmsprop(0.01f, 0.99f, 1e-8f, 0.0f, 0.0f);
    if (rmsprop) {
        nova_optim_step(rmsprop, params, grads, 1);
        printf("  RMSprop: %s\n", TEST_PASS);
        nova_optim_free(rmsprop);
    } else {
        printf("  RMSprop: %s\n", TEST_FAIL);
        return -1;
    }
    
    // Test Adagrad
    nova_optimizer_t* adagrad = nova_optim_adagrad(0.01f, 1e-8f, 0.0f);
    if (adagrad) {
        nova_optim_step(adagrad, params, grads, 1);
        printf("  Adagrad: %s\n", TEST_PASS);
        nova_optim_free(adagrad);
    } else {
        printf("  Adagrad: %s\n", TEST_FAIL);
        return -1;
    }
    
    nova_tensor_free(param);
    nova_tensor_free(grad);
    
    return 0;
}

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║         Nova AI Feature Completeness Test Suite              ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    
    int result = 0;
    
    result |= test_pooling_layers();
    result |= test_normalization_layers();
    result |= test_loss_functions();
    result |= test_optimizers();
    
    printf("\n");
    if (result == 0) {
        printf("╔═══════════════════════════════════════════════════════════════╗\n");
        printf("║                   %s ALL TESTS PASSED %s                      ║\n", TEST_PASS, "");
        printf("╚═══════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔═══════════════════════════════════════════════════════════════╗\n");
        printf("║                   %s SOME TESTS FAILED %s                     ║\n", TEST_FAIL, "");
        printf("╚═══════════════════════════════════════════════════════════════╝\n");
    }
    printf("\n");
    
    return result;
}
