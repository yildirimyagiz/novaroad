/**
 * nova_rocm_stub.c - ROCm backend stub when SDK not available (dispatch link)
 */
#include "rocm/nova_rocm.h"
#include <stdint.h>

bool nova_rocm_is_available(void) { return false; }
int64_t nova_rocm_init(void) { return 0; }
void nova_rocm_cleanup(void) {}
int64_t nova_rocm_get_device_count(void) { return 0; }
void nova_rocm_print_info(void) {}

int64_t nova_rocm_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k) {
  (void)a;(void)b;(void)c;(void)m;(void)n;(void)k;
  return -1;
}
int64_t nova_rocm_add(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
int64_t nova_rocm_mul(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
int64_t nova_rocm_relu(const float *in, float *out, int64_t n) {
  (void)in;(void)out;(void)n;
  return -1;
}
int64_t nova_rocm_softmax(const float *in, float *out, int64_t n) {
  (void)in;(void)out;(void)n;
  return -1;
}
