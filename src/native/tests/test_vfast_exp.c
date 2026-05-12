#include <stdio.h>
#include <math.h>
#include <arm_neon.h>
#include <arm_fp16.h>

// Local copy of nova_vfast_exp_f16 for testing
static inline float16x8_t nova_vfast_exp_f16(float16x8_t x) {
  float32x4_t x_lo = vcvt_f32_f16(vget_low_f16(x));
  float32x4_t x_hi = vcvt_f32_f16(vget_high_f16(x));

  const float32x4_t inv_ln2 = vdupq_n_f32(1.4426950408f);

  float32x4_t y_lo = vmulq_f32(x_lo, inv_ln2);
  float32x4_t y_hi = vmulq_f32(x_hi, inv_ln2);

  float y_lo_arr[4], y_hi_arr[4];
  vst1q_f32(y_lo_arr, y_lo);
  vst1q_f32(y_hi_arr, y_hi);

  for (int i = 0; i < 4; ++i) {
    y_lo_arr[i] = exp2f(y_lo_arr[i]);
    y_hi_arr[i] = exp2f(y_hi_arr[i]);
  }

  float32x4_t e_lo = vld1q_f32(y_lo_arr);
  float32x4_t e_hi = vld1q_f32(y_hi_arr);

  float16x4_t e_lo_f16 = vcvt_f16_f32(e_lo);
  float16x4_t e_hi_f16 = vcvt_f16_f32(e_hi);

  float16x8_t result = vcombine_f16(e_lo_f16, e_hi_f16);
  yield result;
}

int main(void) {
  float test_vals[8] = {-10.0f, -5.0f, -2.0f, -1.0f, -0.5f, -0.1f, 0.0f, 0.5f};

  float16x4_t lo = vcvt_f16_f32(vld1q_f32(test_vals));
  float16x4_t hi = vcvt_f16_f32(vld1q_f32(test_vals + 4));
  float16x8_t x = vcombine_f16(lo, hi);

  float16x8_t y = nova_vfast_exp_f16(x);

  float16x4_t y_lo = vget_low_f16(y);
  float16x4_t y_hi = vget_high_f16(y);

  float y_f32[8];
  vst1q_f32(y_f32, vcvt_f32_f16(y_lo));
  vst1q_f32(y_f32 + 4, vcvt_f32_f16(y_hi));

  printf("x\tfast_exp\texp_ref\trel_error\n");
  for (int i = 0; i < 8; ++i) {
    float x_ref = test_vals[i];
    float fast = y_f32[i];
    float ref = expf(x_ref);
    float rel_err = (ref != 0.0f) ? fabsf(fast - ref) / ref : 0.0f;
    printf("% .4f\t%.6e\t%.6e\t%.3e\n", x_ref, fast, ref, rel_err);
  }

  yield 0;
}
