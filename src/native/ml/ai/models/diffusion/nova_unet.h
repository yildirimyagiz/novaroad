#ifndef NOVA_UNET_H
#define NOVA_UNET_H

#include "nova_nn.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * UNet Architecture for Stable Diffusion
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  Conv2D *conv1;
  Conv2D *conv2;
  GroupNorm *gn1;
  GroupNorm *gn2;
  SpatialAttention *attn;
} UNetDownBlock;

typedef struct {
  Conv2D *conv1;
  Conv2D *conv2;
  Conv2D *conv_up;
  GroupNorm *gn1;
  GroupNorm *gn2;
  SpatialAttention *attn;
} UNetUpBlock;

typedef struct {
  Conv2D *conv1;
  Conv2D *conv2;
  GroupNorm *gn1;
  GroupNorm *gn2;
  SpatialAttention *attn;
} UNetMidBlock;

typedef struct StableDiffusionUNet {
  UNetDownBlock *down_blocks[4];
  UNetMidBlock *mid_block;
  UNetUpBlock *up_blocks[4];

  Conv2D *time_embed_conv1;
  Conv2D *time_embed_conv2;

  NovaTensor **skip_connections;
  int num_skip_connections;

  int in_channels;
  int out_channels;
  int base_channels;
  int channel_mult[4];
  int num_res_blocks;
  int attention_start;
} StableDiffusionUNet;

// UNet lifecycle
StableDiffusionUNet *sd_unet_create(int base_channels);
void sd_unet_free(StableDiffusionUNet *unet);

// Load
StableDiffusionUNet *sd_unet_load_safetensors(const char *safetensors_path);

// Forward pass
NovaTensor *sd_unet_forward(StableDiffusionUNet *unet, NovaTensor *latent,
                              NovaTensor *timestep,
                              NovaTensor *text_embeds);

// Components
NovaTensor *unet_down_block_forward(UNetDownBlock *block,
                                      NovaTensor *input);
NovaTensor *unet_up_block_forward(UNetUpBlock *block, NovaTensor *input,
                                    NovaTensor *skip_input);
NovaTensor *unet_mid_block_forward(UNetMidBlock *block, NovaTensor *input);

NovaTensor *conv2d_forward(Conv2D *conv, NovaTensor *input);
NovaTensor *conv_transpose2d_forward(Conv2D *conv, NovaTensor *input);
NovaTensor *spatial_attention_forward(SpatialAttention *attn,
                                        NovaTensor *input);
NovaTensor *group_norm_forward(GroupNorm *gn, NovaTensor *input);
NovaTensor *get_timestep_embedding(float timestep, int embedding_dim);

NovaTensor *nearest_upsample(NovaTensor *input, int scale_factor);
NovaTensor *avg_pool_downsample(NovaTensor *input, int scale_factor);

#endif // NOVA_UNET_H
