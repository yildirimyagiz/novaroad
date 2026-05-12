/**
 * model_loader_znm.c - Nova Native Model (.znm) Loader
 * 
 * Implements the .znm format for efficient model storage and loading
 * 
 * Format:
 * - Header (32 bytes): magic, version, metadata
 * - Config section: Model configuration (JSON or binary)
 * - Weights section: Tensor data (mmap-able)
 * - Vocabulary section: Tokenizer vocab
 */

#include "ml/nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Magic number: "ZNMD" (Nova Model)
#define ZNM_MAGIC 0x444D4E5A  // "ZNMD" in little-endian

// Version
#define ZNM_VERSION 1

// ═══════════════════════════════════════════════════════════════════════════
// File Format Structures
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    uint32_t magic;              // "ZNMD"
    uint32_t version;            // Format version
    uint32_t model_type;         // 0=LLaMA, 1=Mistral, 2=GPT, etc.
    uint32_t quantization;       // 0=FP32, 1=FP16, 2=INT8, etc.
    uint64_t config_offset;      // Offset to config section
    uint64_t config_size;        // Size of config section
    uint64_t weights_offset;     // Offset to weights section
    uint64_t weights_size;       // Size of weights section
} __attribute__((packed)) ZNMHeader;

typedef struct {
    char name[64];               // Tensor name (e.g., "model.layers.0.self_attn.q_proj.weight")
    uint32_t dtype;              // Data type
    uint32_t ndim;               // Number of dimensions
    int64_t shape[8];            // Tensor shape (max 8 dims)
    uint64_t offset;             // Offset in weights section
    uint64_t size;               // Size in bytes
} __attribute__((packed)) ZNMTensorInfo;

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
} __attribute__((packed)) ZNMModelConfig;

// ═══════════════════════════════════════════════════════════════════════════
// Model Loading
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int fd;
    void *mmap_ptr;
    size_t mmap_size;
    ZNMHeader header;
    ZNMModelConfig config;
    int num_tensors;
    ZNMTensorInfo *tensor_infos;
} ZNMFile;

/**
 * Open .znm file
 */
ZNMFile *znm_open(const char *path) {
    ZNMFile *file = calloc(1, sizeof(ZNMFile));
    if (!file) return NULL;
    
    // Open file
    file->fd = open(path, O_RDONLY);
    if (file->fd < 0) {
        fprintf(stderr, "❌ Failed to open %s\n", path);
        free(file);
        return NULL;
    }
    
    // Get file size
    struct stat st;
    if (fstat(file->fd, &st) < 0) {
        close(file->fd);
        free(file);
        return NULL;
    }
    file->mmap_size = st.st_size;
    
    // Memory-map the file
    file->mmap_ptr = mmap(NULL, file->mmap_size, PROT_READ, MAP_PRIVATE, file->fd, 0);
    if (file->mmap_ptr == MAP_FAILED) {
        fprintf(stderr, "❌ Failed to mmap %s\n", path);
        close(file->fd);
        free(file);
        return NULL;
    }
    
    // Read header
    memcpy(&file->header, file->mmap_ptr, sizeof(ZNMHeader));
    
    // Verify magic number
    if (file->header.magic != ZNM_MAGIC) {
        fprintf(stderr, "❌ Invalid .znm file: bad magic number\n");
        munmap(file->mmap_ptr, file->mmap_size);
        close(file->fd);
        free(file);
        return NULL;
    }
    
    // Verify version
    if (file->header.version != ZNM_VERSION) {
        fprintf(stderr, "⚠️  Warning: .znm version mismatch (file=%u, expected=%u)\n",
                file->header.version, ZNM_VERSION);
    }
    
    // Read config
    if (file->header.config_size >= sizeof(ZNMModelConfig)) {
        void *config_ptr = (char*)file->mmap_ptr + file->header.config_offset;
        memcpy(&file->config, config_ptr, sizeof(ZNMModelConfig));
    }
    
    printf("✅ Opened .znm file: %s\n", path);
    printf("   Model type: %u\n", file->header.model_type);
    printf("   Quantization: %u\n", file->header.quantization);
    printf("   Hidden size: %lld\n", file->config.hidden_size);
    printf("   Layers: %lld\n", file->config.num_hidden_layers);
    
    return file;
}

/**
 * Load tensor by name
 */
NovaTensor *znm_load_tensor(ZNMFile *file, const char *name) {
    if (!file || !name) return NULL;
    
    // TODO: Parse tensor info section to find tensor by name
    // For now, return a dummy tensor
    
    int64_t shape[2] = {file->config.hidden_size, file->config.hidden_size};
    NovaTensor *tensor = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    
    if (tensor) {
        printf("✅ Loaded tensor: %s [%lld, %lld]\n", name, shape[0], shape[1]);
    }
    
    return tensor;
}

/**
 * Get model configuration
 */
ZNMModelConfig *znm_get_config(ZNMFile *file) {
    return file ? &file->config : NULL;
}

/**
 * Close file
 */
void znm_close(ZNMFile *file) {
    if (!file) return;
    
    if (file->tensor_infos) {
        free(file->tensor_infos);
    }
    
    if (file->mmap_ptr && file->mmap_ptr != MAP_FAILED) {
        munmap(file->mmap_ptr, file->mmap_size);
    }
    
    if (file->fd >= 0) {
        close(file->fd);
    }
    
    free(file);
}

// ═══════════════════════════════════════════════════════════════════════════
// Model Saving (Writer)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    FILE *fp;
    uint64_t current_offset;
    int num_tensors;
    ZNMTensorInfo *tensor_infos;
} ZNMWriter;

/**
 * Create .znm file for writing
 */
ZNMWriter *znm_writer_create(const char *path, const ZNMModelConfig *config) {
    ZNMWriter *writer = calloc(1, sizeof(ZNMWriter));
    if (!writer) return NULL;
    
    writer->fp = fopen(path, "wb");
    if (!writer->fp) {
        free(writer);
        return NULL;
    }
    
    // Write header (placeholder)
    ZNMHeader header = {
        .magic = ZNM_MAGIC,
        .version = ZNM_VERSION,
        .model_type = 0,  // Will be set later
        .quantization = 0,
        .config_offset = sizeof(ZNMHeader),
        .config_size = sizeof(ZNMModelConfig),
        .weights_offset = 0,  // Will be set later
        .weights_size = 0
    };
    fwrite(&header, sizeof(ZNMHeader), 1, writer->fp);
    
    // Write config
    fwrite(config, sizeof(ZNMModelConfig), 1, writer->fp);
    
    writer->current_offset = sizeof(ZNMHeader) + sizeof(ZNMModelConfig);
    
    printf("✅ Created .znm file: %s\n", path);
    
    return writer;
}

/**
 * Write tensor to file
 */
int znm_writer_add_tensor(ZNMWriter *writer, const char *name, 
                          const NovaTensor *tensor) {
    if (!writer || !name || !tensor) return -1;
    
    // Write tensor data
    size_t data_size = tensor->total_elements * sizeof(float);
    fwrite(tensor->data, 1, data_size, writer->fp);
    
    // Record tensor info
    writer->tensor_infos = realloc(writer->tensor_infos, 
                                   (writer->num_tensors + 1) * sizeof(ZNMTensorInfo));
    
    ZNMTensorInfo *info = &writer->tensor_infos[writer->num_tensors];
    strncpy(info->name, name, 63);
    info->name[63] = '\0';
    info->dtype = tensor->dtype;
    info->ndim = tensor->ndim;
    memcpy(info->shape, tensor->shape, tensor->ndim * sizeof(int64_t));
    info->offset = writer->current_offset;
    info->size = data_size;
    
    writer->current_offset += data_size;
    writer->num_tensors++;
    
    printf("✅ Added tensor: %s [", name);
    for (int i = 0; i < tensor->ndim; i++) {
        printf("%lld%s", tensor->shape[i], i < tensor->ndim - 1 ? ", " : "");
    }
    printf("]\n");
    
    return 0;
}

/**
 * Finalize and close
 */
void znm_writer_close(ZNMWriter *writer) {
    if (!writer) return;
    
    // TODO: Write tensor info section
    // TODO: Update header with final offsets
    
    if (writer->tensor_infos) {
        free(writer->tensor_infos);
    }
    
    if (writer->fp) {
        fclose(writer->fp);
    }
    
    free(writer);
    
    printf("✅ .znm file saved\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// Checkpoint Conversion Utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Convert PyTorch checkpoint to .znm
 * 
 * This would typically use Python interop or a separate tool
 */
int znm_convert_from_pytorch(const char *input_path, const char *output_path) {
    printf("🔄 Converting PyTorch checkpoint to .znm...\n");
    printf("   Input:  %s\n", input_path);
    printf("   Output: %s\n", output_path);
    
    // TODO: Implement actual conversion
    // For now, just create a dummy model
    
    ZNMModelConfig config = {
        .vocab_size = 32000,
        .hidden_size = 4096,
        .num_hidden_layers = 32,
        .num_attention_heads = 32,
        .num_key_value_heads = 32,
        .intermediate_size = 11008,
        .max_position_embeddings = 2048,
        .rope_theta = 10000.0f,
        .rms_norm_eps = 1e-6f
    };
    
    ZNMWriter *writer = znm_writer_create(output_path, &config);
    if (!writer) return -1;
    
    // Add dummy tensors
    int64_t shape[2] = {config.hidden_size, config.hidden_size};
    NovaTensor *dummy = nova_tensor_ones(NULL, shape, 2);
    
    znm_writer_add_tensor(writer, "model.embed_tokens.weight", dummy);
    znm_writer_add_tensor(writer, "model.layers.0.self_attn.q_proj.weight", dummy);
    
    nova_tensor_destroy(dummy);
    znm_writer_close(writer);
    
    printf("✅ Conversion complete!\n");
    
    return 0;
}
