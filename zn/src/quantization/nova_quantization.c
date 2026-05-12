/**
 * nova_quantization.c - INT8 quantization, calibration, per-channel support
 */

#include "../../include/nova_quantization.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef NOVA_QUANT_MIN
#define NOVA_QUANT_MIN (-128)
#endif
#ifndef NOVA_QUANT_MAX
#define NOVA_QUANT_MAX 127
#endif

static inline int32_t clamp_int32(int32_t v, int32_t lo, int32_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void nova_quantize_f32_to_int8(const float *in_f32, int8_t *out_int8,
                                 size_t n, float scale, int32_t zero_point) {
  if (!in_f32 || !out_int8 || scale == 0.f)
    return;
  for (size_t i = 0; i < n; i++) {
    float q = in_f32[i] / scale + (float)zero_point;
    int32_t v = (int32_t)roundf(q);
    v = clamp_int32(v, NOVA_QUANT_MIN, NOVA_QUANT_MAX);
    out_int8[i] = (int8_t)v;
  }
}

void nova_dequantize_int8_to_f32(const int8_t *in_int8, float *out_f32,
                                   size_t n, float scale, int32_t zero_point) {
  if (!in_int8 || !out_f32)
    return;
  for (size_t i = 0; i < n; i++)
    out_f32[i] = ((float)(int32_t)in_int8[i] - (float)zero_point) * scale;
}

void nova_quantize_calibrate_minmax(const float *data, size_t n,
                                      int symmetric,
                                      float *out_scale, int32_t *out_zero_point) {
  if (!data || n == 0 || !out_scale || !out_zero_point) {
    if (out_scale) *out_scale = 1.f;
    if (out_zero_point) *out_zero_point = 0;
    return;
  }
  float min_val = data[0], max_val = data[0];
  for (size_t i = 1; i < n; i++) {
    if (data[i] < min_val) min_val = data[i];
    if (data[i] > max_val) max_val = data[i];
  }
  if (symmetric) {
    float abs_max = fmaxf(fabsf(min_val), fabsf(max_val));
    *out_scale = (abs_max > 1e-9f) ? (abs_max / (float)NOVA_QUANT_MAX) : 1.f;
    *out_zero_point = 0;
  } else {
    float range = max_val - min_val;
    *out_scale = (range > 1e-9f) ? (range / 255.f) : 1.f;
    *out_zero_point = clamp_int32((int32_t)roundf(-min_val / *out_scale),
                                  NOVA_QUANT_MIN, NOVA_QUANT_MAX);
  }
}

void nova_quantize_calibrate_per_channel(const float *data, int64_t rows,
                                           int64_t cols, int channel_dim,
                                           int symmetric,
                                           float *out_scale,
                                           int32_t *out_zero_point) {
  if (!data || !out_scale || !out_zero_point) return;
  if (channel_dim == 0) {
    for (int64_t r = 0; r < rows; r++) {
      const float *row = data + r * cols;
      nova_quantize_calibrate_minmax(row, (size_t)cols, symmetric,
                                      out_scale + r, out_zero_point + r);
    }
  } else {
    for (int64_t c = 0; c < cols; c++) {
      float min_val = data[c], max_val = data[c];
      for (int64_t r = 0; r < rows; r++) {
        float v = data[r * cols + c];
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
      }
      if (symmetric) {
        float abs_max = fmaxf(fabsf(min_val), fabsf(max_val));
        out_scale[c] = (abs_max > 1e-9f) ? (abs_max / (float)NOVA_QUANT_MAX) : 1.f;
        out_zero_point[c] = 0;
      } else {
        float range = max_val - min_val;
        out_scale[c] = (range > 1e-9f) ? (range / 255.f) : 1.f;
        out_zero_point[c] = clamp_int32((int32_t)roundf(-min_val / out_scale[c]),
                                        NOVA_QUANT_MIN, NOVA_QUANT_MAX);
      }
    }
  }
}

void nova_quantize_f32_to_int8_per_channel(const float *in_f32, int8_t *out_int8,
                                             int64_t rows, int64_t cols,
                                             int channel_dim,
                                             const float *scale,
                                             const int32_t *zero_point) {
  if (!in_f32 || !out_int8 || !scale || !zero_point) return;
  if (channel_dim == 0) {
    for (int64_t r = 0; r < rows; r++) {
      nova_quantize_f32_to_int8(in_f32 + r * cols, out_int8 + r * cols,
                                  (size_t)cols, scale[r], zero_point[r]);
    }
  } else {
    for (int64_t c = 0; c < cols; c++) {
      float s = scale[c];
      int32_t zp = zero_point[c];
      if (s == 0.f) s = 1.f;
      for (int64_t r = 0; r < rows; r++) {
        float q = in_f32[r * cols + c] / s + (float)zp;
        int32_t v = clamp_int32((int32_t)roundf(q), NOVA_QUANT_MIN, NOVA_QUANT_MAX);
        out_int8[r * cols + c] = (int8_t)v;
      }
    }
  }
}

void nova_dequantize_int8_to_f32_per_channel(const int8_t *in_int8,
                                               float *out_f32,
                                               int64_t rows, int64_t cols,
                                               int channel_dim,
                                               const float *scale,
                                               const int32_t *zero_point) {
  if (!in_int8 || !out_f32 || !scale || !zero_point) return;
  if (channel_dim == 0) {
    for (int64_t r = 0; r < rows; r++) {
      nova_dequantize_int8_to_f32(in_int8 + r * cols, out_f32 + r * cols,
                                    (size_t)cols, scale[r], zero_point[r]);
    }
  } else {
    for (int64_t c = 0; c < cols; c++) {
      float s = scale[c];
      int32_t zp = zero_point[c];
      for (int64_t r = 0; r < rows; r++)
        out_f32[r * cols + c] = ((float)(int32_t)in_int8[r * cols + c] - (float)zp) * s;
    }
  }
}
