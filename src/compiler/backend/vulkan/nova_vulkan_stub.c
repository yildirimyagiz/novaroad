/**
 * nova_vulkan_stub.c - Vulkan backend stub when SDK not available (dispatch link)
 */
#include "vulkan/nova_vulkan.h"
#include <stdint.h>

bool nova_vk_is_available(void) { return false; }
int64_t nova_vk_init(void) { return 0; }
void nova_vk_cleanup(void) {}
int64_t nova_vk_get_device_count(void) { return 0; }
void nova_vk_print_info(void) {}

int64_t nova_vk_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k) {
  (void)a;(void)b;(void)c;(void)m;(void)n;(void)k;
  return -1;
}
int64_t nova_vk_add(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
int64_t nova_vk_mul(const float *a, const float *b, float *c, int64_t n) {
  (void)a;(void)b;(void)c;(void)n;
  return -1;
}
int64_t nova_vk_relu(const float *in, float *out, int64_t n) {
  (void)in;(void)out;(void)n;
  return -1;
}
