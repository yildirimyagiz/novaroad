/**
 * nova_metal_stub.c - Non-Apple stub (Metal backend unavailable)
 */

#include "metal/nova_metal_gpu.h"
#include <stdint.h>

int64_t nova_metal_init(void) { return 0; }
int64_t nova_metal_get_device_count(void) { return 0; }
void nova_metal_cleanup(void) {}
void nova_metal_print_info(void) {}

int64_t nova_metal_copy_to_gpu(const void *host_data, int64_t size) {
  (void)host_data;
  (void)size;
  return -1;
}
int64_t nova_metal_copy_from_gpu(int64_t gpu_buffer, void *host_data, int64_t size) {
  (void)gpu_buffer;
  (void)host_data;
  (void)size;
  return -1;
}
int64_t nova_metal_free_buffer(int64_t gpu_buffer) {
  (void)gpu_buffer;
  return -1;
}

int64_t nova_metal_add(const float *a, const float *b, float *c, int64_t count) {
  (void)a;
  (void)b;
  (void)c;
  (void)count;
  return -1;
}
int64_t nova_metal_mul(const float *a, const float *b, float *c, int64_t count) {
  (void)a;
  (void)b;
  (void)c;
  (void)count;
  return -1;
}
int64_t nova_metal_matmul(const float *a, const float *b, float *c,
                            int64_t m, int64_t n, int64_t k) {
  (void)a;
  (void)b;
  (void)c;
  (void)m;
  (void)n;
  (void)k;
  return -1;
}
int64_t nova_metal_relu(const float *in, float *out, int64_t n) {
  (void)in;
  (void)out;
  (void)n;
  return -1;
}
int64_t nova_metal_softmax(const float *in, float *out, int64_t n) {
  (void)in;
  (void)out;
  (void)n;
  return -1;
}
int64_t nova_metal_lora_forward(const float *i, const float *la,
                                  const float *lb, float *o, int64_t b,
                                  int64_t id, int64_t r, int64_t od) {
  (void)i;
  (void)la;
  (void)lb;
  (void)o;
  (void)b;
  (void)id;
  (void)r;
  (void)od;
  return -1;
}
int64_t nova_metal_lora_backward(const float *go, const float *i, float *gla,
                                   float *glb, int64_t b, int64_t id, int64_t r,
                                   int64_t od) {
  (void)go;
  (void)i;
  (void)gla;
  (void)glb;
  (void)b;
  (void)id;
  (void)r;
  (void)od;
  return -1;
}
