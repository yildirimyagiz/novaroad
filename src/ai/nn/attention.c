/**
 * @file attention.c
 * @brief Multi-head attention implementation
 */

#include "ai/nn.h"
#include "ai/tensor.h"
#include <math.h>

/* Scaled Dot-Product Attention */
static nova_tensor_t *scaled_dot_product_attention(
    nova_tensor_t *Q,  /* [batch, seq_len, d_k] */
    nova_tensor_t *K,  /* [batch, seq_len, d_k] */
    nova_tensor_t *V,  /* [batch, seq_len, d_v] */
    float scale)
{
    /* scores = Q @ K^T / sqrt(d_k) */
    nova_tensor_t *K_T = nova_tensor_transpose(K, -1, -2);
    nova_tensor_t *scores = nova_tensor_matmul(Q, K_T);
    
    /* Scale */
    float *data = nova_tensor_data(scores);
    size_t size = nova_tensor_numel(scores);
    for (size_t i = 0; i < size; i++) {
        data[i] *= scale;
    }
    
    /* Softmax over last dimension */
    nova_tensor_t *attn_weights = nova_softmax(scores, -1);
    
    /* attn_output = attn_weights @ V */
    nova_tensor_t *output = nova_tensor_matmul(attn_weights, V);
    
    return output;
}

/* Multi-Head Attention Forward */
nova_tensor_t *nova_mha_forward(
    nova_layer_t *layer,
    nova_tensor_t *query,
    nova_tensor_t *key,
    nova_tensor_t *value)
{
    if (!layer || layer->type != NOVA_LAYER_ATTENTION) {
        return NULL;
    }
    
    /* Extract parameters */
    typedef struct {
        nova_tensor_t *Wq, *Wk, *Wv, *Wo;
        nova_tensor_t *bq, *bk, *bv, *bo;
        size_t embed_dim;
        size_t num_heads;
        size_t head_dim;
    } attention_layer_t;
    
    attention_layer_t *params = (attention_layer_t *)layer->params;
    size_t num_heads = params->num_heads;
    size_t head_dim = params->head_dim;
    
    /* Linear projections: Q, K, V */
    nova_tensor_t *Q = nova_tensor_add(
        nova_tensor_matmul(query, params->Wq), params->bq);
    nova_tensor_t *K = nova_tensor_add(
        nova_tensor_matmul(key, params->Wk), params->bk);
    nova_tensor_t *V = nova_tensor_add(
        nova_tensor_matmul(value, params->Wv), params->bv);
    
    /* Split into multiple heads: [batch, seq_len, embed_dim] -> [batch, num_heads, seq_len, head_dim] */
    /* TODO: Implement tensor reshape and split */
    
    /* Scaled dot-product attention for each head */
    float scale = 1.0f / sqrtf((float)head_dim);
    nova_tensor_t *attn_output = scaled_dot_product_attention(Q, K, V, scale);
    
    /* Concatenate heads and project */
    /* TODO: Implement concatenation */
    
    /* Final linear projection */
    nova_tensor_t *output = nova_tensor_add(
        nova_tensor_matmul(attn_output, params->Wo), params->bo);
    
    return output;
}

/* Self-Attention (Q=K=V) */
nova_tensor_t *nova_self_attention(nova_layer_t *layer, nova_tensor_t *input)
{
    return nova_mha_forward(layer, input, input, input);
}

/* Cross-Attention (K=V, Q different) */
nova_tensor_t *nova_cross_attention(
    nova_layer_t *layer,
    nova_tensor_t *query,
    nova_tensor_t *context)
{
    return nova_mha_forward(layer, query, context, context);
}
