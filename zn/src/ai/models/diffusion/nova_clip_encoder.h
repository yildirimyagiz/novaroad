#ifndef NOVA_CLIP_ENCODER_H
#define NOVA_CLIP_ENCODER_H

#include "nova_clip_tokenizer.h"
#include "nova_tensor.h"
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * CLIP Text Encoder for Stable Diffusion
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  NovaTensor *token_embedding;    // [vocab_size, embed_dim]
  NovaTensor *position_embedding; // [max_seq_len, embed_dim]

  struct {
    NovaTensor *q_proj;
    NovaTensor *k_proj;
    NovaTensor *v_proj;
    NovaTensor *out_proj;

    NovaTensor *ln1_weight;
    NovaTensor *ln1_bias;
    NovaTensor *ln2_weight;
    NovaTensor *ln2_bias;

    NovaTensor *mlp_fc1_weight;
    NovaTensor *mlp_fc1_bias;
    NovaTensor *mlp_fc2_weight;
    NovaTensor *mlp_fc2_bias;
  } layers[12];

  NovaTensor *final_ln_weight;
  NovaTensor *final_ln_bias;

  int embed_dim;
  int num_heads;
  int num_layers;
  int max_seq_len;
  int vocab_size;
} CLIPEmbedder;

// Lifecycle
CLIPEmbedder *clip_encoder_create();
void clip_encoder_free(CLIPEmbedder *encoder);

// Load
CLIPEmbedder *clip_encoder_load_safetensors(const char *safetensors_path);

// Forward
NovaTensor *clip_encoder_forward(CLIPEmbedder *encoder,
                                   const uint32_t *tokens, int seq_len);
NovaTensor *clip_encoder_forward_batch(CLIPEmbedder *encoder,
                                         const uint32_t **token_batch,
                                         int batch_size, int max_seq_len);

// Attention
NovaTensor *clip_attention_forward(NovaTensor *q, NovaTensor *k,
                                     NovaTensor *v, int num_heads,
                                     int embed_dim);

// Utils
NovaTensor *clip_layer_norm(NovaTensor *input, NovaTensor *weight,
                              NovaTensor *bias);
NovaTensor *clip_mlp_forward(NovaTensor *input, NovaTensor *fc1_weight,
                               NovaTensor *fc1_bias, NovaTensor *fc2_weight,
                               NovaTensor *fc2_bias);
NovaTensor *clip_get_position_embeddings(CLIPEmbedder *encoder, int seq_len);

typedef struct CLIPConditioner {
  CLIPEmbedder *encoder;
  CLIPTokenizer *tokenizer;
} CLIPConditioner;

CLIPConditioner *clip_conditioner_create();
void clip_conditioner_free(CLIPConditioner *conditioner);

NovaTensor *clip_conditioner_encode_prompt(CLIPConditioner *conditioner,
                                             const char *prompt);
NovaTensor *clip_conditioner_encode_batch(CLIPConditioner *conditioner,
                                            const char **prompts,
                                            int batch_size);

#endif // NOVA_CLIP_ENCODER_H
