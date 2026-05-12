/**
 * nova_cuda_stub.c - CUDA backend stub when SDK not available (dispatch link)
 */
#include "cuda/nova_cuda.h"
#include <stdint.h>

int64_t nova_cuda_init(void) { return 0; }
void nova_cuda_cleanup(void) {}
int64_t nova_cuda_get_device_count(void) { return 0; }
void nova_cuda_print_info(void) {}

int64_t nova_cuda_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k) {
  (void)a;(void)b;(void)c;(void)m;(void)n;(void)k;
  return -1;
}
int64_t nova_cuda_add(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
int64_t nova_cuda_mul(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
