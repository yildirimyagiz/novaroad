/**
 * nova_qat.h — Quantization-Aware Training (QAT)
 *
 * QAT simulates INT8 quantization during FP32 training using the
 * Straight-Through Estimator (STE): gradients pass through the
 * fake-quantize op unchanged, but activations/weights see the
 * quantization error during the forward pass.
 *
 * Usage:
 *   1. Wrap each weight/activation with nova_fake_quantize_f32()
 *   2. Run forward + backward normally (STE gradient)
 *   3. After training, call nova_qat_export_int8() to get INT8 weights
 */

#ifndef NOVA_QAT_H
#define NOVA_QAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * QAT Layer state — per layer scale + zero_point, updated each step
 * ----------------------------------------------------------------------- */
typedef struct {
    float    scale;
    int32_t  zero_point;
    bool     symmetric;
    bool     per_channel;       /* if true, scale[] has 'n_channels' entries */
    int      n_channels;

    /* per-channel arrays (NULL if per-tensor) */
    float    *ch_scale;
    int32_t  *ch_zp;

    /* EMA momentum for running scale estimate */
    float    ema_momentum;      /* e.g. 0.1 */
    float    ema_scale;         /* running EMA of calibrated scale */
} NovaQATLayer;

/* -----------------------------------------------------------------------
 * INT8 exported weight (after QAT training is done)
 * ----------------------------------------------------------------------- */
typedef struct {
    int8_t  *data;
    size_t   n;
    float    scale;
    int32_t  zero_point;
} NovaINT8Weight;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/** Create a QAT layer config */
NovaQATLayer *nova_qat_layer_create(bool symmetric, bool per_channel,
                                     int n_channels, float ema_momentum);

/** Destroy a QAT layer */
void nova_qat_layer_free(NovaQATLayer *layer);

/**
 * Fake-quantize a weight/activation tensor (in-place simulation).
 *
 * Forward:  x_fq = dequantize(quantize(x))   — introduces quantization error
 * Backward: gradient passes through unchanged (Straight-Through Estimator)
 *
 * @param in_f32   input float32 values
 * @param out_f32  output fake-quantized float32 values (same shape)
 * @param n        number of elements
 * @param layer    QAT layer state (scale is updated via EMA)
 */
void nova_fake_quantize_f32(const float *in_f32, float *out_f32,
                              size_t n, NovaQATLayer *layer);

/**
 * Calibrate QAT layer scale from current data (call every N steps or once).
 */
void nova_qat_calibrate(NovaQATLayer *layer, const float *data, size_t n);

/**
 * Export trained FP32 weights to INT8 using the QAT layer's calibrated scale.
 */
NovaINT8Weight *nova_qat_export_int8(const float *weights_f32, size_t n,
                                      const NovaQATLayer *layer);

/** Free an exported INT8 weight */
void nova_int8_weight_free(NovaINT8Weight *w);

/**
 * INT8 linear layer forward pass (weights are INT8, activations FP32).
 *
 * out[batch x out_feat] = in[batch x in_feat] @ W_int8[out_feat x in_feat]^T
 *                         (dequantized on the fly) + bias
 */
void nova_int8_linear_forward(
    const float   *input,        /* [batch x in_feat]  */
    const int8_t  *w_int8,       /* [out_feat x in_feat] INT8 */
    const float   *bias,         /* [out_feat] or NULL  */
    float         *output,       /* [batch x out_feat]  */
    int            batch,
    int            in_feat,
    int            out_feat,
    float          w_scale,      /* dequant scale for weights */
    int32_t        w_zp          /* dequant zero-point */
);

#endif /* NOVA_QAT_H */
