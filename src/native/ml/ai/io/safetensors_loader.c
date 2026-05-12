/*
 * Safetensors Loader Implementation
 * Format: https://github.com/huggingface/safetensors
 */

#include "safetensors_loader.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// ============================================================================
// FLOAT16 SUPPORT (Safetensors often stores weights as F16)
// ============================================================================

static inline float f16_to_f32(uint16_t h) {
  uint16_t h_exp = (uint16_t)(h & 0x7C00u);
  uint16_t h_sig = (uint16_t)(h & 0x03FFu);
  uint32_t sign = ((uint32_t)h & 0x8000u) << 16;

  uint32_t f;
  if (h_exp == 0x0000u) {
    if (h_sig == 0) {
      f = sign;
    } else {
      uint32_t mant = h_sig;
      int exp = -1;
      do {
        exp++;
        mant <<= 1;
      } while ((mant & 0x0400u) == 0);
      mant &= 0x03FFu;
      uint32_t exp32 = (uint32_t)(127 - 15 - exp);
      f = sign | (exp32 << 23) | (mant << 13);
    }
  } else if (h_exp == 0x7C00u) {
    f = sign | 0x7F800000u | ((uint32_t)h_sig << 13);
  } else {
    uint32_t exp32 = (uint32_t)(((h_exp >> 10) + (127 - 15)) & 0xFFu);
    f = sign | (exp32 << 23) | ((uint32_t)h_sig << 13);
  }

  float out;
  memcpy(&out, &f, sizeof(out));
  return out;
}

static inline uint64_t read_u64_le(const uint8_t *p) {
  return ((uint64_t)p[0]) | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) |
         ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) |
         ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) |
         ((uint64_t)p[7] << 56);
}

// JSON parsing helpers (minimal implementation)
static int64_t *extract_shape(const char *json, const char *key, int *ndim) {
  char search[256];
  snprintf(search, sizeof(search), "\"%s\":[", key);

  const char *start = strstr(json, search);
  if (!start)
    return NULL;

  start += strlen(search);
  const char *end = strchr(start, ']');
  if (!end)
    return NULL;

  // Count dimensions
  *ndim = 1;
  for (const char *p = start; p < end; p++) {
    if (*p == ',')
      (*ndim)++;
  }

  int64_t *shape = (int64_t *)calloc(*ndim, sizeof(int64_t));
  if (!shape)
    return NULL;

  const char *p = start;
  for (int i = 0; i < *ndim; i++) {
    while (p < end && !isdigit(*p) && *p != '-')
      p++;
    if (p < end) {
      shape[i] = atoll(p);
      while (p < end && (isdigit(*p) || *p == '-'))
        p++;
    }
  }

  return shape;
}

static int64_t *extract_data_offsets(const char *json, const char *key) {
  char search[256];
  snprintf(search, sizeof(search), "\"%s\":[", key);

  const char *start = strstr(json, search);
  if (!start)
    return NULL;

  start += strlen(search);
  const char *end = strchr(start, ']');
  if (!end)
    return NULL;

  int64_t *offsets = (int64_t *)calloc(2, sizeof(int64_t));
  if (!offsets)
    return NULL;

  const char *p = start;
  for (int i = 0; i < 2; i++) {
    while (p < end && !isdigit(*p))
      p++;
    if (p < end) {
      offsets[i] = atoll(p);
      while (p < end && isdigit(*p))
        p++;
    }
  }

  return offsets;
}

// ============================================================================
// SAFETENSORS FILE OPERATIONS
// ============================================================================

SafetensorsFile *safetensors_open(const char *filepath) {
  printf("📂 Opening safetensors file: %s\n", filepath);

  // Open file
  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    printf("❌ Failed to open file\n");
    return NULL;
  }

  // Get file size
  struct stat sb;
  if (fstat(fd, &sb) < 0) {
    close(fd);
    return NULL;
  }

  // Memory map the file
  void *data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return NULL;
  }

  close(fd);

  SafetensorsFile *file = (SafetensorsFile *)calloc(1, sizeof(SafetensorsFile));
  file->filepath = strdup(filepath);
  file->mmap_data = data;
  file->file_size = (size_t)sb.st_size;
  file->num_tensors = 0;
  file->metadata = NULL;

  // Read header size (first 8 bytes, little-endian)
  uint64_t header_size = read_u64_le((const uint8_t *)data);
  file->header_size = header_size;

  printf("   File size: %lld bytes\n", (long long)sb.st_size);
  printf("   Header size: %llu bytes\n", header_size);

  // Extract JSON header and cache it
  file->json_header_cache = (char *)malloc(header_size + 1);
  if (!file->json_header_cache) {
    munmap(data, sb.st_size);
    free(file->filepath);
    free(file);
    return NULL;
  }
  memcpy(file->json_header_cache, (char *)data + 8, header_size);
  file->json_header_cache[header_size] = '\0';
  char *json_header = file->json_header_cache;

  // Scan for max data_offset to verify file integrity
  uint64_t max_offset = 0;
  const char *offset_ptr = json_header;
  while ((offset_ptr = strstr(offset_ptr, "\"data_offsets\":")) != NULL) {
    uint64_t o1, o2;
    if (sscanf(offset_ptr, "\"data_offsets\":[%llu,%llu]", &o1, &o2) == 2) {
      if (o2 > max_offset)
        max_offset = o2;
    }
    offset_ptr++;
  }

  uint64_t expected_size = 8 + header_size + max_offset;
  if (file->file_size < expected_size) {
    printf("   ⚠️  WARNING: FILE TRUNCATED!\n");
    printf("   Expected: %llu bytes\n", expected_size);
    printf("   Actual:   %lld bytes\n", (long long)file->file_size);
    printf("   Missing:  %llu bytes (%.1f%%)\n",
           expected_size - file->file_size,
           100.0 * (expected_size - file->file_size) / expected_size);
  }

  // Count tensors
  int count = 0;
  const char *p_count = json_header;
  while ((p_count = strstr(p_count, "\"dtype\"")) != NULL) {
    count++;
    p_count++;
  }
  file->num_tensors = count;
  file->metadata = (TensorMetadata *)calloc(count, sizeof(TensorMetadata));

  printf("   ✅ Safetensors integrity check finished\n");
  return file;
}

void safetensors_close(SafetensorsFile *file) {
  if (!file)
    return;

  munmap(file->mmap_data, file->file_size);
  if (file->json_header_cache) {
    free(file->json_header_cache);
  }
  free(file->filepath);

  for (int i = 0; i < file->num_tensors; i++) {
    if (file->metadata[i].key)
      free(file->metadata[i].key);
    if (file->metadata[i].dtype)
      free(file->metadata[i].dtype);
    if (file->metadata[i].shape)
      free(file->metadata[i].shape);
  }
  if (file->metadata)
    free(file->metadata);
  free(file);
}

NovaTensor *safetensors_load_tensor(NovaContext *ctx, SafetensorsFile *file,
                                      const char *key) {
  if (!file || !file->mmap_data || !file->json_header_cache)
    return NULL;

  printf("   Loading tensor: %s\n", key);

  char *json_header = file->json_header_cache;
  uint64_t header_size = file->header_size;

  // Find the tensor entry in JSON
  char search_key[512];
  snprintf(search_key, sizeof(search_key), "\"%s\":", key);
  char *entry = strstr(json_header, search_key);
  if (!entry) {
    printf("   ⚠️ Tensor %s not found\n", key);
    return NULL;
  }

  // Extract shape
  int ndim = 0;
  int64_t *shape = extract_shape(entry, "shape", &ndim);

  // Extract data offsets
  int64_t *offsets = extract_data_offsets(entry, "data_offsets");

  if (!shape || !offsets) {
    printf("   ⚠️ Failed to parse metadata for %s\n", key);
    if (shape)
      free(shape);
    if (offsets)
      free(offsets);
    return NULL;
  }

  // Create NovaTensor
  NovaTensor *t = nova_tensor_create(ctx, shape, ndim, NOVA_DTYPE_FP32);
  if (!t) {
    printf("   ⚠️ Failed to create tensor for %s\n", key);
    free(shape);
    free(offsets);
    return NULL;
  }

  // Copy data from mmap
  if (8 + header_size + offsets[1] > file->file_size) {
    printf("   ❌ Data offsets out of bounds for %s\n", key);
    nova_tensor_destroy(t);
    free(shape);
    free(offsets);
    return NULL;
  }

  void *data_start = (char *)file->mmap_data + 8 + header_size + offsets[0];
  size_t data_size = (size_t)(offsets[1] - offsets[0]);
  float *t_data = (float *)t->data;

  // Handle dtype
  const char *dtype_p = strstr(entry, "\"dtype\":");
  if (dtype_p && strstr(dtype_p, "\"F16\"")) {
    const uint16_t *f16_data = (const uint16_t *)data_start;
    for (int64_t i = 0; i < (int64_t)t->total_elements; i++) {
      t_data[i] = f16_to_f32(f16_data[i]);
    }
  } else {
    // Default F32
    memcpy(t_data, data_start,
           data_size > t->total_elements * 4 ? t->total_elements * 4
                                             : data_size);
  }

  free(shape);
  free(offsets);
  printf("   ✅ Tensor loaded: %s (%zu bytes)\n", key, data_size);
  return t;
}

int safetensors_get_tensor_count(SafetensorsFile *file) {
  return file ? file->num_tensors : 0;
}

// ============================================================================
// SD 1.5 UNET LOADER
// ============================================================================

SD15UNet *load_sd15_unet(NovaContext *ctx, const char *safetensors_path) {
  printf(
      "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("🔧 Loading SD 1.5 UNet from safetensors\n");
  printf(
      "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

  SafetensorsFile *file = safetensors_open(safetensors_path);
  if (!file) {
    printf("❌ Failed to open model file\n");
    return NULL;
  }

  SD15UNet *unet = (SD15UNet *)malloc(sizeof(SD15UNet));
  unet->num_layers = 16;
  unet->hidden_size = 768;

  unet->q_proj = (NovaTensor **)malloc(16 * sizeof(NovaTensor *));
  unet->k_proj = (NovaTensor **)malloc(16 * sizeof(NovaTensor *));
  unet->v_proj = (NovaTensor **)malloc(16 * sizeof(NovaTensor *));
  unet->out_proj = (NovaTensor **)malloc(16 * sizeof(NovaTensor *));

  const char *block_prefixes[] = {"model.diffusion_model.input_blocks.1.1",
                                  "model.diffusion_model.input_blocks.2.1",
                                  "model.diffusion_model.input_blocks.4.1",
                                  "model.diffusion_model.input_blocks.5.1",
                                  "model.diffusion_model.input_blocks.7.1",
                                  "model.diffusion_model.input_blocks.8.1",
                                  "model.diffusion_model.middle_block.1",
                                  "model.diffusion_model.output_blocks.3.1",
                                  "model.diffusion_model.output_blocks.4.1",
                                  "model.diffusion_model.output_blocks.5.1",
                                  "model.diffusion_model.output_blocks.6.1",
                                  "model.diffusion_model.output_blocks.7.1",
                                  "model.diffusion_model.output_blocks.8.1",
                                  "model.diffusion_model.output_blocks.9.1",
                                  "model.diffusion_model.output_blocks.10.1",
                                  "model.diffusion_model.output_blocks.11.1"};

  for (int i = 0; i < 16; i++) {
    char key[512];
    const char *prefix = block_prefixes[i];

    printf("   Loading block %d/16: %s...\n", i + 1, prefix);

    snprintf(key, sizeof(key), "%s.transformer_blocks.0.attn1.to_q.weight",
             prefix);
    unet->q_proj[i] = safetensors_load_tensor(ctx, file, key);

    snprintf(key, sizeof(key), "%s.transformer_blocks.0.attn1.to_k.weight",
             prefix);
    unet->k_proj[i] = safetensors_load_tensor(ctx, file, key);

    snprintf(key, sizeof(key), "%s.transformer_blocks.0.attn1.to_v.weight",
             prefix);
    unet->v_proj[i] = safetensors_load_tensor(ctx, file, key);

    snprintf(key, sizeof(key), "%s.transformer_blocks.0.attn1.to_out.0.weight",
             prefix);
    unet->out_proj[i] = safetensors_load_tensor(ctx, file, key);
  }

  safetensors_close(file);
  printf("\n✅ SD 1.5 UNet loading finished\n");
  return unet;
}

void free_sd15_unet(SD15UNet *unet) {
  if (!unet)
    return;

  for (int i = 0; i < unet->num_layers; i++) {
    nova_tensor_destroy(unet->q_proj[i]);
    nova_tensor_destroy(unet->k_proj[i]);
    nova_tensor_destroy(unet->v_proj[i]);
    nova_tensor_destroy(unet->out_proj[i]);
  }

  free(unet->q_proj);
  free(unet->k_proj);
  free(unet->v_proj);
  free(unet->out_proj);
  free(unet);
}

void safetensors_save_lora(const char *filepath, NovaLoRALayer *lora) {
  if (!lora)
    return;

  FILE *f = fopen(filepath, "wb");
  if (!f) {
    printf("❌ Failed to open %s for writing\n", filepath);
    return;
  }

  char json[2048];
  int64_t size_down = lora->lora_down->total_elements * sizeof(float);
  int64_t size_up = lora->lora_up->total_elements * sizeof(float);

  const char *base_key =
      "lora_unet_input_blocks_1_1_transformer_blocks_0_attn1_to_q";

  snprintf(json, sizeof(json),
           "{\"%s.lora_down.weight\":{\"dtype\":\"F32\",\"shape\":[%lld,%lld],"
           "\"data_offsets\":[0,%lld]},"
           "\"%s.lora_up.weight\":{\"dtype\":\"F32\",\"shape\":[%lld,%lld],"
           "\"data_offsets\":[%lld,%lld]}}",
           base_key, lora->lora_down->shape[0], lora->lora_down->shape[1],
           size_down, base_key, lora->lora_up->shape[0],
           lora->lora_up->shape[1], size_down, size_down + size_up);

  uint64_t header_size = strlen(json);
  fwrite(&header_size, sizeof(uint64_t), 1, f);
  fwrite(json, 1, header_size, f);

  fwrite(lora->lora_down->data, 1, size_down, f);
  fwrite(lora->lora_up->data, 1, size_up, f);

  fclose(f);
  printf("   ✅ Safetensors LoRA saved correctly to %s\n", filepath);
}
