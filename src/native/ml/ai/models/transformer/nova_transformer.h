#ifndef NOVA_TRANSFORMER_H
#define NOVA_TRANSFORMER_H

#include "nova_advanced_optimizations.h"
#include "nova_nn.h"
#include "nova_tensor_ops.h"
#include <stdlib.h>

// ============================================================================
// TRANSFORMER ARCHITECTURE - Production-Grade Implementation
// ============================================================================
// Complete Transformer with multi-head attention, feed-forward networks,
// and positional encodings for NLP and vision tasks

// ============================================================================
// POSITIONAL ENCODING
// ============================================================================

typedef struct {
  int max_seq_len;
  int embed_dim;
  float *pos_encoding; // [max_seq_len, embed_dim]
} PositionalEncoding;

// ============================================================================
// MULTI-HEAD ATTENTION
// ============================================================================

typedef struct {
  int embed_dim;
  int num_heads;
  int head_dim;
  float dropout;

  // Linear layers
  LinearLayer *q_proj;   // [embed_dim, embed_dim]
  LinearLayer *k_proj;   // [embed_dim, embed_dim]
  LinearLayer *v_proj;   // [embed_dim, embed_dim]
  LinearLayer *out_proj; // [embed_dim, embed_dim]

  // Attention cache for inference
  float *attn_cache; // For KV cache optimization
} MultiHeadAttention;

// ============================================================================
// FEED-FORWARD NETWORK
// ============================================================================

typedef struct {
  int embed_dim;
  int ff_dim; // Usually 4 * embed_dim
  float dropout;

  LinearLayer *fc1; // [embed_dim, ff_dim]
  LinearLayer *fc2; // [ff_dim, embed_dim]
} FeedForwardNetwork;

// ============================================================================
// TRANSFORMER ENCODER LAYER
// ============================================================================

typedef struct {
  MultiHeadAttention *self_attn;
  FeedForwardNetwork *ff_net;

  // Layer normalization
  float *ln1_weight; // [embed_dim]
  float *ln1_bias;   // [embed_dim]
  float *ln2_weight; // [embed_dim]
  float *ln2_bias;   // [embed_dim]

  int embed_dim;
  float dropout;
} TransformerEncoderLayer;

// ============================================================================
// TRANSFORMER DECODER LAYER
// ============================================================================

typedef struct {
  MultiHeadAttention *self_attn;
  MultiHeadAttention *cross_attn; // For encoder-decoder attention
  FeedForwardNetwork *ff_net;

  // Layer normalization
  float *ln1_weight; // [embed_dim]
  float *ln1_bias;   // [embed_dim]
  float *ln2_weight; // [embed_dim]
  float *ln2_bias;   // [embed_dim]
  float *ln3_weight; // [embed_dim]
  float *ln3_bias;   // [embed_dim]

  int embed_dim;
  float dropout;
} TransformerDecoderLayer;

// ============================================================================
// COMPLETE TRANSFORMER ENCODER
// ============================================================================

typedef struct {
  // Token embeddings
  float *token_embeddings; // [vocab_size, embed_dim]
  PositionalEncoding *pos_encoding;

  // Encoder layers
  TransformerEncoderLayer **layers;
  int num_layers;

  // Final layer norm
  float *ln_weight; // [embed_dim]
  float *ln_bias;   // [embed_dim]

  int vocab_size;
  int embed_dim;
  int max_seq_len;
  float dropout;
} TransformerEncoder;

// ============================================================================
// COMPLETE TRANSFORMER DECODER
// ============================================================================

typedef struct {
  // Token embeddings
  float *token_embeddings; // [vocab_size, embed_dim]
  PositionalEncoding *pos_encoding;

  // Decoder layers
  TransformerDecoderLayer **layers;
  int num_layers;

  // Final layer norm
  float *ln_weight; // [embed_dim]
  float *ln_bias;   // [embed_dim]

  int vocab_size;
  int embed_dim;
  int max_seq_len;
  float dropout;
} TransformerDecoder;

// ============================================================================
// COMPLETE TRANSFORMER (ENCODER-DECODER)
// ============================================================================

typedef struct {
  TransformerEncoder *encoder;
  TransformerDecoder *decoder;

  // Output projection
  LinearLayer *lm_head; // [embed_dim, vocab_size] for language modeling

  int vocab_size;
  int embed_dim;
  int num_layers;
  int num_heads;
  int max_seq_len;
} Transformer;

// ============================================================================
// TRAINING UTILITIES
// ============================================================================

typedef struct {
  Transformer *model;
  NovaKVCache *kv_cache; // For autoregressive generation
  NovaLossScaler *loss_scaler;
  AdamW *optimizer;

  // Training config
  int batch_size;
  int max_seq_len;
  float learning_rate;
  float label_smoothing;
} TransformerTrainer;

// ============================================================================
// TEXT PROCESSING FOR TRANSFORMER
// ============================================================================

typedef struct {
  CLIPTokenizer *tokenizer;
  int max_length;
  int vocab_size;
  bool add_special_tokens;
} TextProcessor;

// ============================================================================
// API FUNCTIONS
// ============================================================================

// Model lifecycle
Transformer *transformer_create(int vocab_size, int embed_dim, int num_layers,
                                int num_heads, int max_seq_len);
void transformer_free(Transformer *model);

// Forward pass
Tensor *transformer_forward(Transformer *model, int *input_ids, int seq_len);
Tensor *transformer_generate(Transformer *model, int *prompt, int prompt_len,
                             int max_new_tokens, float temperature);

// Training
void transformer_train_step(Transformer *model, int *input_ids, int *labels,
                            int batch_size, int seq_len,
                            TransformerTrainer *trainer);

// Load pretrained (would integrate with safetensors)
Transformer *transformer_load_pretrained(const char *model_path);

// ============================================================================
// SPECIALIZED TRANSFORMER VARIANTS
// ============================================================================

// Vision Transformer (ViT)
typedef struct {
  // Patch embedding
  Conv2D *patch_embed; // [patch_size*patch_size*channels, embed_dim]
  PositionalEncoding *pos_embed;

  // Transformer encoder
  TransformerEncoder *transformer;

  // Classification head
  LinearLayer *cls_head; // [embed_dim, num_classes]

  int image_size;
  int patch_size;
  int num_patches;
  int num_classes;
} VisionTransformer;

// BERT-style encoder
typedef struct {
  TransformerEncoder *encoder;
  LinearLayer *pooler;   // For [CLS] token pooling
  LinearLayer *cls_head; // Classification head

  int num_classes;
} BERTModel;

// GPT-style decoder
typedef struct {
  TransformerDecoder *decoder;
  NovaKVCache *kv_cache; // Essential for generation
} GPTModel;

// ============================================================================
// TRAINING DEMO - SHAKESPEARE TEXT GENERATION
// ============================================================================

void train_shakespeare_transformer_demo();

#endif // NOVA_TRANSFORMER_H
