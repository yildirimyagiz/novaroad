/**
 * @file inference.h
 * @brief AI model inference engine
 */

#ifndef NOVA_INFERENCE_H
#define NOVA_INFERENCE_H

#include "tensor.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Model Types
 * ========================================================================== */

typedef struct nova_model nova_model_t;
typedef struct nova_inference_session nova_inference_session_t;

typedef enum {
    NOVA_MODEL_FORMAT_GGUF,      /**< GGUF format (llama.cpp) */
    NOVA_MODEL_FORMAT_ONNX,      /**< ONNX format */
    NOVA_MODEL_FORMAT_SAFETENSORS, /**< SafeTensors format */
    NOVA_MODEL_FORMAT_PYTORCH,   /**< PyTorch (.pt/.pth) */
    NOVA_MODEL_FORMAT_TENSORFLOW, /**< TensorFlow SavedModel */
} nova_model_format_t;

// Use nova_device_t from tensor.h for device types
// Extended device types for inference engine
typedef enum {
    NOVA_DEVICE_TYPE_CPU = NOVA_DEVICE_CPU,      /**< CPU device */
    NOVA_DEVICE_TYPE_GPU,                        /**< Generic GPU device */
    NOVA_DEVICE_TYPE_CUDA,                       /**< NVIDIA CUDA device */
    NOVA_DEVICE_TYPE_METAL = NOVA_DEVICE_METAL,  /**< Apple Metal device */
    NOVA_DEVICE_TYPE_VULKAN,                     /**< Vulkan device */
    NOVA_DEVICE_TYPE_OPENCL,                     /**< OpenCL device */
} nova_device_type_t;

/* ============================================================================
 * Inference Configuration
 * ========================================================================== */

typedef struct {
    nova_device_type_t device;
    int device_id;                /**< Device ID (for multi-GPU) */
    size_t max_batch_size;
    bool enable_profiling;
    bool use_fp16;                /**< Use half-precision */
    bool use_int8;                /**< Use 8-bit quantization */
    size_t cache_size_mb;         /**< KV cache size in MB */
    int num_threads;              /**< CPU threads */
} nova_inference_config_t;

/**
 * Get default inference configuration
 * @return Default configuration
 */
nova_inference_config_t nova_inference_default_config(void);

/* ============================================================================
 * Model Loading
 * ========================================================================== */

/**
 * Load model from file
 * @param filename Path to model file
 * @param format Model format (auto-detect if FORMAT_AUTO)
 * @return Loaded model or NULL on error
 */
nova_model_t *nova_model_load(const char *filename, nova_model_format_t format);

/**
 * Load model from memory
 * @param data Model data in memory
 * @param size Size of model data
 * @param format Model format
 * @return Loaded model or NULL on error
 */
nova_model_t *nova_model_load_from_memory(const void *data, size_t size,
                                          nova_model_format_t format);

/**
 * Get model metadata
 * @param model The model
 * @param key Metadata key (e.g., "name", "version", "description")
 * @return Metadata value or NULL if not found
 */
const char *nova_model_get_metadata(nova_model_t *model, const char *key);

/**
 * Get model input names
 * @param model The model
 * @param num_inputs Output: number of inputs
 * @return Array of input names
 */
const char **nova_model_get_input_names(nova_model_t *model, size_t *num_inputs);

/**
 * Get model output names
 * @param model The model
 * @param num_outputs Output: number of outputs
 * @return Array of output names
 */
const char **nova_model_get_output_names(nova_model_t *model, size_t *num_outputs);

/**
 * Destroy model and free resources
 * @param model The model to destroy
 */
void nova_model_destroy(nova_model_t *model);

/* ============================================================================
 * Inference Session
 * ========================================================================== */

/**
 * Create inference session
 * @param model The model
 * @param config Inference configuration
 * @return Created session or NULL on error
 */
nova_inference_session_t *nova_inference_create_session(
    nova_model_t *model,
    const nova_inference_config_t *config
);

/**
 * Run inference with single input/output
 * @param session The session
 * @param input Input tensor
 * @return Output tensor or NULL on error
 */
nova_tensor_t *nova_inference_run(nova_inference_session_t *session,
                                  nova_tensor_t *input);

/**
 * Run inference with multiple inputs/outputs
 * @param session The session
 * @param inputs Array of input tensors
 * @param num_inputs Number of inputs
 * @param outputs Output: array of output tensors
 * @param num_outputs Output: number of outputs
 * @return 0 on success, negative on error
 */
int nova_inference_run_multi(nova_inference_session_t *session,
                             nova_tensor_t **inputs, size_t num_inputs,
                             nova_tensor_t ***outputs, size_t *num_outputs);

/**
 * Run inference asynchronously
 * @param session The session
 * @param input Input tensor
 * @param callback Callback function when done
 * @param user_data User data passed to callback
 * @return Request ID or negative on error
 */
int64_t nova_inference_run_async(nova_inference_session_t *session,
                                 nova_tensor_t *input,
                                 void (*callback)(nova_tensor_t *, void *),
                                 void *user_data);

/**
 * Wait for async inference to complete
 * @param session The session
 * @param request_id Request ID from run_async
 * @return Output tensor or NULL on error
 */
nova_tensor_t *nova_inference_wait(nova_inference_session_t *session,
                                   int64_t request_id);

/**
 * Get inference statistics
 * @param session The session
 * @param key Statistic key (e.g., "inference_time_ms", "throughput")
 * @return Statistic value
 */
double nova_inference_get_stat(nova_inference_session_t *session, const char *key);

/**
 * Reset session statistics
 * @param session The session
 */
void nova_inference_reset_stats(nova_inference_session_t *session);

/**
 * Destroy inference session
 * @param session The session to destroy
 */
void nova_inference_destroy_session(nova_inference_session_t *session);

/* ============================================================================
 * Model Quantization
 * ========================================================================== */

typedef enum {
    NOVA_QUANT_NONE,
    NOVA_QUANT_INT8,
    NOVA_QUANT_INT4,
    NOVA_QUANT_FP16,
    NOVA_QUANT_BFLOAT16,
} nova_quant_type_t;

/**
 * Quantize model to reduce size and improve performance
 * @param model Original model
 * @param quant_type Quantization type
 * @return Quantized model or NULL on error
 */
nova_model_t *nova_model_quantize(nova_model_t *model, nova_quant_type_t quant_type);

/**
 * Save quantized model to file
 * @param model The model
 * @param filename Output filename
 * @return 0 on success, negative on error
 */
int nova_model_save(nova_model_t *model, const char *filename);

/* ============================================================================
 * Streaming Inference (for LLMs)
 * ========================================================================== */

typedef void (*nova_token_callback_t)(int32_t token_id, const char *token_text,
                                     void *user_data);

/**
 * Generate text with streaming output
 * @param session The session
 * @param prompt Input prompt
 * @param max_tokens Maximum tokens to generate
 * @param callback Called for each generated token
 * @param user_data User data passed to callback
 * @return Number of tokens generated or negative on error
 */
int nova_inference_generate(nova_inference_session_t *session,
                           const char *prompt,
                           size_t max_tokens,
                           nova_token_callback_t callback,
                           void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_INFERENCE_H */
