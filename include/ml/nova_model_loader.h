/**
 * nova_model_loader.h - Nova Native Model Loading
 * 
 * API for loading models in .znm (Nova Native Model) format
 */

#ifndef NOVA_MODEL_LOADER_H
#define NOVA_MODEL_LOADER_H

#include "nova_tensor.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct ZNMFile ZNMFile;
typedef struct ZNMWriter ZNMWriter;

// Model configuration
typedef struct {
    int64_t vocab_size;
    int64_t hidden_size;
    int64_t num_hidden_layers;
    int64_t num_attention_heads;
    int64_t num_key_value_heads;
    int64_t intermediate_size;
    int64_t max_position_embeddings;
    float rope_theta;
    float rms_norm_eps;
} ZNMModelConfig;

// ═══════════════════════════════════════════════════════════════════════════
// Model Loading (Reader)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Open .znm file for reading
 * Uses memory mapping for efficient zero-copy loading
 */
ZNMFile *znm_open(const char *path);

/**
 * Load tensor by name
 * Returns tensor with data directly mapped from file (zero-copy)
 */
NovaTensor *znm_load_tensor(ZNMFile *file, const char *name);

/**
 * Get model configuration
 */
ZNMModelConfig *znm_get_config(ZNMFile *file);

/**
 * Close file and release resources
 */
void znm_close(ZNMFile *file);

// ═══════════════════════════════════════════════════════════════════════════
// Model Saving (Writer)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create .znm file for writing
 */
ZNMWriter *znm_writer_create(const char *path, const ZNMModelConfig *config);

/**
 * Add tensor to model
 */
int znm_writer_add_tensor(ZNMWriter *writer, const char *name, 
                          const NovaTensor *tensor);

/**
 * Finalize and close writer
 */
void znm_writer_close(ZNMWriter *writer);

// ═══════════════════════════════════════════════════════════════════════════
// Conversion Utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Convert PyTorch checkpoint to .znm format
 */
int znm_convert_from_pytorch(const char *input_path, const char *output_path);

/**
 * Convert HuggingFace safetensors to .znm format
 */
int znm_convert_from_safetensors(const char *input_path, const char *output_path);

/**
 * Convert GGUF format to .znm format
 */
int znm_convert_from_gguf(const char *input_path, const char *output_path);

#endif // NOVA_MODEL_LOADER_H
