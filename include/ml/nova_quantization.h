/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_quantization.h - Quantization (INT8 / calibration)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_QUANTIZATION_H
#define NOVA_QUANTIZATION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  NOVA_QUANT_PER_TENSOR,
  NOVA_QUANT_PER_CHANNEL
} NovaQuantScheme;

/** Per-tensor: single scale and zero_point for all elements */
typedef struct {
  float scale;
  int32_t zero_point;
} NovaQuantParams;

/** Per-channel: one scale and zero_point per channel (e.g. per output channel) */
typedef struct {
  float *scale;       /* length = num_channels */
  int32_t *zero_point;
  int num_channels;
  int channel_dim;    /* 0 = rows, 1 = cols for 2D */
} NovaQuantParamsPerChannel;

/**
 * Quantize float array to int8 using scale and zero_point.
 * out_int8[i] = round(in_f32[i] / scale) + zero_point, clamped to [-128, 127].
 */
void nova_quantize_f32_to_int8(const float *in_f32, int8_t *out_int8,
                                 size_t n, float scale, int32_t zero_point);

/**
 * Dequantize int8 to float: out_f32[i] = (in_int8[i] - zero_point) * scale.
 */
void nova_dequantize_int8_to_f32(const int8_t *in_int8, float *out_f32,
                                   size_t n, float scale, int32_t zero_point);

/**
 * Calibrate scale and zero_point from float data (min-max).
 * Symmetric: scale = max(|min|,|max|) / 127, zero_point = 0.
 * Asymmetric: scale = (max - min) / 255, zero_point = round(-min / scale).
 */
void nova_quantize_calibrate_minmax(const float *data, size_t n,
                                      int symmetric,
                                      float *out_scale, int32_t *out_zero_point);

/**
 * Per-channel calibration for 2D tensor [rows x cols].
 * channel_dim 0: one scale/zp per row; channel_dim 1: per column.
 */
void nova_quantize_calibrate_per_channel(const float *data, int64_t rows,
                                           int64_t cols, int channel_dim,
                                           int symmetric,
                                           float *out_scale,
                                           int32_t *out_zero_point);

/**
 * Quantize 2D tensor per-channel (e.g. weight matrix [K x N], one scale per column).
 * out_int8 and scales/zero_points must be pre-allocated (cols entries for channel_dim==1).
 */
void nova_quantize_f32_to_int8_per_channel(const float *in_f32, int8_t *out_int8,
                                             int64_t rows, int64_t cols,
                                             int channel_dim,
                                             const float *scale,
                                             const int32_t *zero_point);

/**
 * Dequantize per-channel int8 back to float.
 */
void nova_dequantize_int8_to_f32_per_channel(const int8_t *in_int8,
                                               float *out_f32,
                                               int64_t rows, int64_t cols,
                                               int channel_dim,
                                               const float *scale,
                                               const int32_t *zero_point);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_QUANTIZATION_H */
