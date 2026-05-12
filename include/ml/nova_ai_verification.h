/**
 * @file nova_ai_verification.h
 * @brief AI-Specific Formal Verification and Safety Guarantees
 * 
 * Extends Nova's formal verification system with AI-specific checks:
 * - Tensor shape verification
 * - Numerical stability guarantees
 * - Training safety contracts
 * - Model property verification
 */

#ifndef NOVA_AI_VERIFICATION_H
#define NOVA_AI_VERIFICATION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Tensor Shape Verification
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    size_t* dims;
    size_t ndim;
} tensor_shape_t;

typedef enum {
    SHAPE_CHECK_PASSED,
    SHAPE_CHECK_MISMATCH,
    SHAPE_CHECK_INVALID_DIM,
    SHAPE_CHECK_BROADCAST_ERROR
} shape_check_result_t;

/**
 * Verify tensor shape compatibility for operations
 */
shape_check_result_t nova_verify_shape_matmul(
    tensor_shape_t A_shape,
    tensor_shape_t B_shape,
    tensor_shape_t* out_shape
);

shape_check_result_t nova_verify_shape_broadcast(
    tensor_shape_t A_shape,
    tensor_shape_t B_shape,
    tensor_shape_t* out_shape
);

shape_check_result_t nova_verify_shape_conv2d(
    tensor_shape_t input_shape,
    tensor_shape_t kernel_shape,
    size_t stride,
    size_t padding,
    tensor_shape_t* out_shape
);

/**
 * Compile-time shape constraint checking
 */
bool nova_verify_shape_constraint(
    const char* constraint_expr,
    tensor_shape_t shape
);

// ═══════════════════════════════════════════════════════════════════════════
// Numerical Stability Verification
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    STABILITY_SAFE,
    STABILITY_GRADIENT_EXPLOSION,
    STABILITY_GRADIENT_VANISHING,
    STABILITY_OVERFLOW_RISK,
    STABILITY_UNDERFLOW_RISK,
    STABILITY_NAN_DETECTED,
    STABILITY_INF_DETECTED
} stability_status_t;

typedef struct {
    stability_status_t status;
    float max_value;
    float min_value;
    float gradient_norm;
    bool has_nan;
    bool has_inf;
} stability_report_t;

/**
 * Check numerical stability of tensor values
 */
stability_report_t nova_verify_numerical_stability(
    const float* data,
    size_t size,
    float gradient_clip_threshold
);

/**
 * Verify gradient flow safety
 */
bool nova_verify_gradient_safety(
    const float* gradients,
    size_t size,
    float clip_min,
    float clip_max
);

/**
 * Check for NaN/Inf propagation
 */
bool nova_verify_no_nan_inf(
    const float* data,
    size_t size
);

// ═══════════════════════════════════════════════════════════════════════════
// AI Contract System
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    CONTRACT_PRECONDITION,
    CONTRACT_POSTCONDITION,
    CONTRACT_INVARIANT
} contract_type_t;

typedef struct {
    contract_type_t type;
    const char* expression;
    bool (*check_fn)(void* ctx);
    void* context;
} ai_contract_t;

typedef struct {
    ai_contract_t* contracts;
    size_t count;
    size_t capacity;
} ai_contract_set_t;

/**
 * Create AI contract set
 */
ai_contract_set_t* nova_ai_contracts_create(void);

/**
 * Add contract to set
 */
void nova_ai_contracts_add(
    ai_contract_set_t* set,
    contract_type_t type,
    const char* expr,
    bool (*check_fn)(void*)
);

/**
 * Verify all contracts
 */
bool nova_ai_contracts_verify_all(ai_contract_set_t* set);

/**
 * Destroy contract set
 */
void nova_ai_contracts_destroy(ai_contract_set_t* set);

// ═══════════════════════════════════════════════════════════════════════════
// Training Safety Verification
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    float learning_rate;
    float weight_decay;
    float gradient_clip_value;
    size_t batch_size;
    size_t num_parameters;
} training_config_t;

typedef struct {
    bool lr_in_bounds;
    bool weights_finite;
    bool gradients_clipped;
    bool batch_size_valid;
    bool loss_decreasing;
    const char* error_message;
} training_safety_report_t;

/**
 * Verify training configuration safety
 */
training_safety_report_t nova_verify_training_config(
    training_config_t config
);

/**
 * Monitor training step safety
 */
bool nova_verify_training_step(
    const float* weights,
    const float* gradients,
    float loss,
    training_config_t config
);

/**
 * Check loss monotonicity (for debugging)
 */
bool nova_verify_loss_trend(
    const float* loss_history,
    size_t history_len,
    size_t window_size
);

// ═══════════════════════════════════════════════════════════════════════════
// Model Property Verification
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    MODEL_PROP_LIPSCHITZ_CONTINUOUS,
    MODEL_PROP_MONOTONIC,
    MODEL_PROP_BOUNDED_OUTPUT,
    MODEL_PROP_ADVERSARIAL_ROBUST,
    MODEL_PROP_DIFFERENTIAL_PRIVATE
} model_property_t;

typedef struct {
    model_property_t property;
    float parameter;  // e.g., Lipschitz constant, epsilon for robustness
    bool verified;
    const char* proof;
} model_verification_result_t;

/**
 * Verify model Lipschitz continuity
 */
model_verification_result_t nova_verify_lipschitz(
    void* model,
    float (*forward_fn)(void*, const float*, size_t),
    float lipschitz_constant,
    size_t input_dim
);

/**
 * Verify epsilon-robustness against adversarial perturbations
 */
model_verification_result_t nova_verify_adversarial_robustness(
    void* model,
    float (*forward_fn)(void*, const float*, size_t),
    const float* input,
    size_t input_size,
    float epsilon
);

/**
 * Verify output bounds
 */
bool nova_verify_output_bounds(
    const float* outputs,
    size_t size,
    float min_bound,
    float max_bound
);

// ═══════════════════════════════════════════════════════════════════════════
// Integration with Existing Verification Systems
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Register AI verification checks with invariant engine
 */
void nova_register_ai_invariants(void* invariant_engine);

/**
 * Generate SMT constraints for tensor operations
 */
char* nova_generate_tensor_smt_constraints(
    const char* operation,
    tensor_shape_t* shapes,
    size_t num_tensors
);

/**
 * Integrate with kernel contracts
 */
void nova_extend_kernel_contracts_for_ai(void);

// ═══════════════════════════════════════════════════════════════════════════
// Verification Summary and Reporting
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    size_t total_checks;
    size_t passed_checks;
    size_t failed_checks;
    size_t shape_checks;
    size_t stability_checks;
    size_t contract_checks;
    size_t property_checks;
    const char** error_messages;
    size_t num_errors;
} ai_verification_summary_t;

/**
 * Create verification summary
 */
ai_verification_summary_t* nova_ai_verification_summary_create(void);

/**
 * Print verification summary
 */
void nova_ai_verification_summary_print(ai_verification_summary_t* summary);

/**
 * Destroy verification summary
 */
void nova_ai_verification_summary_destroy(ai_verification_summary_t* summary);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_AI_VERIFICATION_H */
