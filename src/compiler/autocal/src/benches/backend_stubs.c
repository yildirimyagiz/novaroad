/**
 * @file backend_stubs.c
 * @brief Stub implementations for backend functions (for standalone autocal)
 */

#include <stdint.h>

// Weak symbols - will be overridden if real backends are linked
__attribute__((weak)) int64_t nova_cuda_init(void) { return 0; }
__attribute__((weak)) int64_t nova_cuda_matmul(const float *a, const float *b, float *c,
                                                 int64_t m, int64_t n, int64_t k) {
    (void)a; (void)b; (void)c; (void)m; (void)n; (void)k;
    return -1;
}
__attribute__((weak)) void nova_cuda_cleanup(void) {}

#ifdef __APPLE__
__attribute__((weak)) int64_t nova_metal_init(void) { return 0; }
__attribute__((weak)) int64_t nova_metal_matmul(const float *a, const float *b, float *c,
                                                  int64_t m, int64_t n, int64_t k) {
    (void)a; (void)b; (void)c; (void)m; (void)n; (void)k;
    return -1;
}
__attribute__((weak)) void nova_metal_cleanup(void) {}
#endif
