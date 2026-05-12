#ifndef NOVA_VAE_DECODER_H
#define NOVA_VAE_DECODER_H

#include "nova_nn.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * VAE Decoder for Stable Diffusion
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct VAEDecoder {
  struct {
    Conv2D *conv_in;
    Conv2D *conv_out;
    Conv2D *conv_up;

    struct {
      Conv2D *conv1;
      Conv2D *conv2;
      GroupNorm *gn1;
      GroupNorm *gn2;
    } res_blocks[2];

    GroupNorm *gn_in;
    GroupNorm *gn_out;
  } decoder_blocks[4];

  Conv2D *conv_final;
  Conv2D *quantize_conv;

  int latent_channels;
  int base_channels;
  int channel_mult[4];
  int resolution;
  NovaActivationType activation;
  NovaNormType normalization;
} VAEDecoder;

// VAE Decoder lifecycle
VAEDecoder *vae_decoder_create(int base_channels);
void vae_decoder_free(VAEDecoder *decoder);

// Load
VAEDecoder *vae_decoder_load_safetensors(const char *safetensors_path);

// Forward pass
NovaTensor *vae_decoder_forward(VAEDecoder *decoder, NovaTensor *latent);
NovaTensor *vae_decoder_forward_batch(VAEDecoder *decoder,
                                        NovaTensor *latent_batch);

// Components
NovaTensor *vae_decoder_block_forward(VAEDecoder *decoder, int level,
                                        NovaTensor *input);
NovaTensor *vae_residual_block_forward(Conv2D *conv1, Conv2D *conv2,
                                         GroupNorm *gn1, GroupNorm *gn2,
                                         NovaTensor *input);

NovaTensor *bilinear_upsample(NovaTensor *input, int scale_factor);
NovaTensor *nearest_upsample_vae(NovaTensor *input, int scale_factor);

NovaTensor *vae_post_process(NovaTensor *raw_output);

// VAE full pipeline
typedef struct {
  VAEDecoder *decoder;
  Conv2D *encoder;
  NovaTensor *quantize_embed;
} VAE;

VAE *vae_create();
void vae_free(VAE *vae);

NovaTensor *vae_reconstruct(VAE *vae, NovaTensor *image);

typedef struct {
  NovaTensor *reconstructed;
  NovaTensor *mu;
  NovaTensor *logvar;
  float kl_loss;
  float reconstruction_loss;
} VAEOutput;

VAEOutput *vae_forward_train(VAE *vae, NovaTensor *image);
void vae_free_output(VAEOutput *output);

float vae_kl_divergence(NovaTensor *mu, NovaTensor *logvar);
NovaTensor *vae_preprocess_image(NovaTensor *image);

void vae_save_latent(NovaTensor *latent, const char *filepath);
NovaTensor *vae_load_latent(const char *filepath);

#endif // NOVA_VAE_DECODER_H
