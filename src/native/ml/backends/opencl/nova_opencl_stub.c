/**
 * nova_opencl_stub.c - OpenCL backend stub when SDK not available (dispatch link)
 */
#include "opencl/nova_opencl.h"
#include <stdint.h>

bool nova_cl_is_available(void) { return false; }
int64_t nova_cl_init(void) { return 0; }
void nova_cl_cleanup(void) {}
int64_t nova_cl_get_platform_count(void) { return 0; }
int64_t nova_cl_get_device_count(void) { return 0; }
void nova_cl_print_info(void) {}

int64_t nova_cl_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k) {
  (void)a;(void)b;(void)c;(void)m;(void)n;(void)k;
  return -1;
}
int64_t nova_cl_add(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
int64_t nova_cl_mul(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
int64_t nova_cl_relu(const float *in, float *out, int64_t n) {
  (void)in;(void)out;(void)n;
  return -1;
}
int64_t nova_cl_softmax(const float *in, float *out, int64_t n) {
  (void)in;(void)out;(void)n;
  return -1;
}
