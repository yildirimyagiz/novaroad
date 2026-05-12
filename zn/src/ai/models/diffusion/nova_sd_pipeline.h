#ifndef NOVA_SD_PIPELINE_H
#define NOVA_SD_PIPELINE_H

#include "nova_advanced_optimizations.h"
#include "nova_fp16_kernels.h"
#include "nova_memory_arena.h"
#include "nova_tensor.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * COMPLETE STABLE DIFFUSION PIPELINE - Phase 5
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Error handling macros
#define NOVA_SD_CHECK(expr)                                                  \
  do {                                                                         \
    NovaSDErrorCode _err = (expr);                                           \
    if (_err != NOVA_SD_SUCCESS) {                                           \
      return _err;                                                             \
    }                                                                          \
  } while (0)

typedef struct {
  // Core components
  CLIPConditioner *text_encoder;
  StableDiffusionUNet *unet;
  VAEDecoder *vae_decoder;

  // Memory & Execution
  NovaMemoryArena *cpu_arena;
  NovaGPUMemoryManager *gpu_manager;
  NovaOperationPlanner *op_planner;
  NovaContext *context;
  NovaDevice device;

  // Model configuration
  int image_size;
  int latent_size;
  int embed_dim;
  int unet_channels;
  int num_inference_steps;

  // Performance settings
  int use_fp16;
  int use_attention_slicing;
  int use_vae_slicing;

  // Error handling
  NovaSDError last_error;
  int error_count;
  void (*error_callback)(NovaSDError *error);

  // Statistics
  double total_inference_time;
  uint64_t images_generated;
} NovaSDPipeline;

// Pipeline lifecycle
NovaSDPipeline *nova_sd_pipeline_create(NovaContext *ctx,
                                            NovaDevice device, int use_fp16);
void nova_sd_pipeline_free(NovaSDPipeline *pipeline);

// Model loading
NovaSDErrorCode nova_sd_load_models(NovaSDPipeline *pipeline,
                                        const char *clip_model_path,
                                        const char *unet_model_path,
                                        const char *vae_model_path);

// Generation
NovaSDErrorCode nova_sd_generate(NovaSDPipeline *pipeline,
                                     const char *prompt,
                                     const char *negative_prompt,
                                     float guidance_scale, int num_steps,
                                     unsigned int seed,
                                     NovaTensor **output_image);

// LoRA support
typedef struct {
  NovaLoRALayer **lora_layers;
  float *lora_scales;
  int num_loras;
  char **lora_names;
} NovaLoRAStack;

int nova_sd_apply_lora(NovaSDPipeline *pipeline, const char *lora_path,
                         float scale);
int nova_sd_remove_lora(NovaSDPipeline *pipeline, const char *lora_name);

#endif // NOVA_SD_PIPELINE_H
