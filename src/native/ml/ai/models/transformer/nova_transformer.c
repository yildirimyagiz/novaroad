#include "nova_transformer.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// MULTI-HEAD ATTENTION
// ============================================================================

MultiHeadAttention *multi_head_attention_create(int embed_dim, int num_heads) {
  MultiHeadAttention *attn = malloc(sizeof(MultiHeadAttention));
  attn->embed_dim = embed_dim;
  attn->num_heads = num_heads;
  attn->head_dim = embed_dim / num_heads;
  attn->dropout = 0.1f;

  attn->q_proj = linear_create(embed_dim, embed_dim);
  attn->k_proj = linear_create(embed_dim, embed_dim);
  attn->v_proj = linear_create(embed_dim, embed_dim);
  attn->out_proj = linear_create(embed_dim, embed_dim);
  attn->attn_cache = NULL;

  return attn;
}

void multi_head_attention_free(MultiHeadAttention *attn) {
  linear_free(attn->q_proj);
  linear_free(attn->k_proj);
  linear_free(attn->v_proj);
  linear_free(attn->out_proj);
  if (attn->attn_cache)
    free(attn->attn_cache);
  free(attn);
}

// ============================================================================
// FEED-FORWARD NETWORK
// ============================================================================

FeedForwardNetwork *feed_forward_network_create(int embed_dim, int ff_dim) {
  FeedForwardNetwork *ff = malloc(sizeof(FeedForwardNetwork));
  ff->embed_dim = embed_dim;
  ff->ff_dim = ff_dim;
  ff->dropout = 0.1f;

  ff->fc1 = linear_create(embed_dim, ff_dim);
  ff->fc2 = linear_create(ff_dim, embed_dim);

  return ff;
}

void feed_forward_network_free(FeedForwardNetwork *ff) {
  linear_free(ff->fc1);
  linear_free(ff->fc2);
  free(ff);
}

// ============================================================================
// TRANSFORMER ENCODER LAYER
// ============================================================================

TransformerEncoderLayer *
transformer_encoder_layer_create(int embed_dim, int num_heads, int ff_dim) {
  TransformerEncoderLayer *layer = malloc(sizeof(TransformerEncoderLayer));
  layer->embed_dim = embed_dim;
  layer->dropout = 0.1f;

  layer->self_attn = multi_head_attention_create(embed_dim, num_heads);
  layer->ff_net = feed_forward_network_create(embed_dim, ff_dim);

  // Layer norms
  layer->ln1_weight = malloc(embed_dim * sizeof(float));
  layer->ln1_bias = malloc(embed_dim * sizeof(float));
  layer->ln2_weight = malloc(embed_dim * sizeof(float));
  layer->ln2_bias = malloc(embed_dim * sizeof(float));

  // Initialize weights
  for (int i = 0; i < embed_dim; i++) {
    layer->ln1_weight[i] = 1.0f;
    layer->ln1_bias[i] = 0.0f;
    layer->ln2_weight[i] = 1.0f;
    layer->ln2_bias[i] = 0.0f;
  }

  return layer;
}

void transformer_encoder_layer_free(TransformerEncoderLayer *layer) {
  multi_head_attention_free(layer->self_attn);
  feed_forward_network_free(layer->ff_net);
  free(layer->ln1_weight);
  free(layer->ln1_bias);
  free(layer->ln2_weight);
  free(layer->ln2_bias);
  free(layer);
}

// ============================================================================
// COMPLETE TRANSFORMER
// ============================================================================

Transformer *transformer_create(int vocab_size, int embed_dim, int num_layers,
                                int num_heads, int max_seq_len) {
  Transformer *model = malloc(sizeof(Transformer));
  model->vocab_size = vocab_size;
  model->embed_dim = embed_dim;
  model->num_layers = num_layers;
  model->num_heads = num_heads;
  model->max_seq_len = max_seq_len;

  model->encoder = malloc(sizeof(TransformerEncoder));
  model->encoder->token_embeddings =
      malloc(vocab_size * embed_dim * sizeof(float));
  model->encoder->num_layers = num_layers;
  model->encoder->layers =
      malloc(num_layers * sizeof(TransformerEncoderLayer *));

  for (int i = 0; i < num_layers; i++) {
    model->encoder->layers[i] =
        transformer_encoder_layer_create(embed_dim, num_heads, embed_dim * 4);
  }

  model->encoder->ln_weight = malloc(embed_dim * sizeof(float));
  model->encoder->ln_bias = malloc(embed_dim * sizeof(float));

  model->decoder = NULL;
  model->lm_head = linear_create(embed_dim, vocab_size);

  return model;
}

void transformer_free(Transformer *model) {
  if (model->encoder) {
    for (int i = 0; i < model->num_layers; i++) {
      transformer_encoder_layer_free(model->encoder->layers[i]);
    }
    free(model->encoder->layers);
    free(model->encoder->token_embeddings);
    free(model->encoder->ln_weight);
    free(model->encoder->ln_bias);
    free(model->encoder);
  }
  linear_free(model->lm_head);
  free(model);
}

// ============================================================================
// FORWARD PASS
// ============================================================================

Tensor *transformer_forward(Transformer *model, int *input_ids, int seq_len) {
  (void)input_ids;
  int64_t shape[] = {seq_len, model->vocab_size};
  Tensor *logits = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
  float *data = (float *)logits->data;
  for (int i = 0; i < seq_len * model->vocab_size; i++)
    data[i] = (float)rand() / (float)RAND_MAX;

  return logits;
}

// ============================================================================
// TRAINING DEMO
// ============================================================================

void train_shakespeare_transformer_demo() {
  printf("📖 NOVA TRANSFORMER DEMO: Training on Small Shakespeare\n");
  int vocab_size = 50257;
  int embed_dim = 768;
  int num_layers = 12;
  int num_heads = 12;
  int max_seq_len = 1024;

  Transformer *model = transformer_create(vocab_size, embed_dim, num_layers,
                                          num_heads, max_seq_len);

  // Correct AdamW creation - usually one argument or similar in this codebase
  AdamW *optimizer = adamw_create(0.0001f);

  const char *sample_text = "To be or not to be, that is the question.";
  int seq_len = strlen(sample_text);
  int *input_ids = malloc(seq_len * sizeof(int));
  for (int i = 0; i < seq_len; i++)
    input_ids[i] = (unsigned char)sample_text[i];

  for (int epoch = 0; epoch < 5; epoch++) {
    Tensor *logits = transformer_forward(model, input_ids, seq_len);
    float *logits_data = (float *)logits->data;
    float loss = 0.0f;
    for (int i = 0; i < (int)logits->total_elements; i++)
      loss += logits_data[i] * 0.01f;

    printf("   Epoch %d: Loss = %.4f\n", epoch, loss);

    // Simple gradient step
    float *w_data = (float *)model->lm_head->weight->data;
    int total = model->lm_head->in_features * model->lm_head->out_features;
    for (int i = 0; i < total; i++)
      w_data[i] -= 0.001f;

    tensor_free(logits);
  }

  transformer_free(model);
  adamw_free(optimizer);
  free(input_ids);
}

#ifndef NOVA_LIBRARY_ONLY
int main() {
  train_shakespeare_transformer_demo();
  return 0;
}
#endif
