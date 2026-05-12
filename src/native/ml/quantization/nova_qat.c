/**
 * nova_qat.c — Quantization-Aware Training implementation
 *
 * Straight-Through Estimator (STE) based fake-quantization.
 * Compatible with any FP32 optimizer (AdamW, SGD).
 */

#include "nova_qat.h"
#include "nova_quantization.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

static inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float fake_quant_scalar(float x, float scale, int32_t zp) {
    /* quantize */
    int32_t q = clamp_i32((int32_t)roundf(x / scale + (float)zp), -128, 127);
    /* dequantize */
    return ((float)(q - zp)) * scale;
}

/* -----------------------------------------------------------------------
 * Creation / destruction
 * ----------------------------------------------------------------------- */

NovaQATLayer *nova_qat_layer_create(bool symmetric, bool per_channel,
                                     int n_channels, float ema_momentum) {
    NovaQATLayer *l = (NovaQATLayer *)calloc(1, sizeof(NovaQATLayer));
    if (!l) return NULL;

    l->symmetric     = symmetric;
    l->per_channel   = per_channel;
    l->n_channels    = n_channels;
    l->ema_momentum  = ema_momentum;
    l->ema_scale     = 0.0f;
    l->scale         = 1.0f;
    l->zero_point    = 0;

    if (per_channel && n_channels > 0) {
        l->ch_scale = (float    *)calloc(n_channels, sizeof(float));
        l->ch_zp    = (int32_t  *)calloc(n_channels, sizeof(int32_t));
        for (int i = 0; i < n_channels; i++) l->ch_scale[i] = 1.0f;
    }
    return l;
}

void nova_qat_layer_free(NovaQATLayer *layer) {
    if (!layer) return;
    free(layer->ch_scale);
    free(layer->ch_zp);
    free(layer);
}

/* -----------------------------------------------------------------------
 * Calibration — update EMA scale
 * ----------------------------------------------------------------------- */

void nova_qat_calibrate(NovaQATLayer *layer, const float *data, size_t n) {
    if (!layer || !data || n == 0) return;

    float s;
    int32_t zp;
    nova_quantize_calibrate_minmax(data, n, (int)layer->symmetric, &s, &zp);

    /* EMA update */
    if (layer->ema_scale < 1e-12f) {
        layer->ema_scale = s;
    } else {
        float m = layer->ema_momentum;
        layer->ema_scale = (1.0f - m) * layer->ema_scale + m * s;
    }

    layer->scale       = layer->ema_scale;
    layer->zero_point  = zp;
}

/* -----------------------------------------------------------------------
 * Fake-quantize (per-tensor)
 * ----------------------------------------------------------------------- */

void nova_fake_quantize_f32(const float *in_f32, float *out_f32,
                              size_t n, NovaQATLayer *layer) {
    if (!in_f32 || !out_f32 || !layer || n == 0) return;

    /* Calibrate scale from current batch */
    nova_qat_calibrate(layer, in_f32, n);

    float   s  = layer->scale > 1e-12f ? layer->scale : 1e-7f;
    int32_t zp = layer->zero_point;

    for (size_t i = 0; i < n; i++)
        out_f32[i] = fake_quant_scalar(in_f32[i], s, zp);
}

/* -----------------------------------------------------------------------
 * Export to INT8
 * ----------------------------------------------------------------------- */

NovaINT8Weight *nova_qat_export_int8(const float *weights_f32, size_t n,
                                      const NovaQATLayer *layer) {
    if (!weights_f32 || n == 0 || !layer) return NULL;

    NovaINT8Weight *w = (NovaINT8Weight *)malloc(sizeof(NovaINT8Weight));
    if (!w) return NULL;

    w->data        = (int8_t *)malloc(n * sizeof(int8_t));
    w->n           = n;
    w->scale       = layer->scale;
    w->zero_point  = layer->zero_point;

    if (!w->data) { free(w); return NULL; }

    nova_quantize_f32_to_int8(weights_f32, w->data, n,
                               w->scale, w->zero_point);
    return w;
}

void nova_int8_weight_free(NovaINT8Weight *w) {
    if (!w) return;
    free(w->data);
    free(w);
}

/* -----------------------------------------------------------------------
 * INT8 linear layer forward (weights INT8, activations FP32)
 * output = (input @ W_dequant^T) + bias
 * ----------------------------------------------------------------------- */

void nova_int8_linear_forward(
    const float   *input,
    const int8_t  *w_int8,
    const float   *bias,
    float         *output,
    int            batch,
    int            in_feat,
    int            out_feat,
    float          w_scale,
    int32_t        w_zp)
{
    if (!input || !w_int8 || !output) return;

    float eff_scale = (w_scale > 1e-12f) ? w_scale : 1e-7f;

    for (int b = 0; b < batch; b++) {
        for (int o = 0; o < out_feat; o++) {
            float acc = 0.0f;
            const int8_t *row = w_int8 + o * in_feat;
            for (int i = 0; i < in_feat; i++) {
                /* dequantize weight element */
                float w_f32 = ((float)((int32_t)row[i] - w_zp)) * eff_scale;
                acc += input[b * in_feat + i] * w_f32;
            }
            output[b * out_feat + o] = acc + (bias ? bias[o] : 0.0f);
        }
    }
}
