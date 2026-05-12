#ifndef SAFETENSORS_LOADER_H
#define SAFETENSORS_LOADER_H

#include "nova_tensor.h"
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * Safetensors Loader for Nova
 * Loads .safetensors files (Stable Diffusion models)
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Safetensors metadata
typedef struct {
  char *key;
  char *dtype;
  int64_t *shape;
  int ndim;
  int64_t data_offset;
  int64_t data_size;
} TensorMetadata;

// Safetensors file
typedef struct {
  char *filepath;
  void *mmap_data;
  size_t file_size;
  uint64_t header_size;
  char *json_header_cache;
  int num_tensors;
  TensorMetadata *metadata;
} SafetensorsFile;

// File operations
SafetensorsFile *safetensors_open(const char *filepath);
void safetensors_close(SafetensorsFile *file);

// Tensor loading
NovaTensor *safetensors_load_tensor(NovaContext *ctx, SafetensorsFile *file,
                                      const char *key);
int safetensors_get_tensor_count(SafetensorsFile *file);
char **safetensors_list_tensors(SafetensorsFile *file);

// Model loading helpers (Legacy SD15 draft)
typedef struct {
  NovaTensor **q_proj;   // Query projections (16 layers)
  NovaTensor **k_proj;   // Key projections (16 layers)
  NovaTensor **v_proj;   // Value projections (16 layers)
  NovaTensor **out_proj; // Output projections (16 layers)
  int num_layers;
  int hidden_size; // 768 for SD 1.5
} SD15UNet;

SD15UNet *load_sd15_unet(NovaContext *ctx, const char *safetensors_path);
void free_sd15_unet(SD15UNet *unet);

void safetensors_save_lora(const char *filepath, NovaLoRALayer *lora);

#endif // SAFETENSORS_LOADER_H
