/**
 * nova_autocal.c - Platform-specific maximum performance calibration
 * Her platform için maksimum verim; özellik eşitliği hedeflenmez.
 */

#include "nova_autocal.h"
#ifdef NOVA_AUTOCAL_USE_DISPATCH
#include "nova_backend_dispatch.h" /* before autotune to avoid NovaBackendType redefinition */
#endif
#include "nova_autotune.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#endif

#define AUTOCAL_MATMUL_M 256
#define AUTOCAL_MATMUL_K 256
#define AUTOCAL_MATMUL_N 256
#define AUTOCAL_WARMUP 3
#define AUTOCAL_ITERS 20

static NovaAutocalPlatformConfig s_configs[NOVA_PLATFORM_COUNT];
static int s_configs_initialized;

static double now_ns(void) {
#ifdef __APPLE__
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return (double)(mach_absolute_time() * tb.numer / tb.denom);
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
#endif
}

double nova_autocal_now_ms(void) {
  return now_ns() / 1e6;
}

static void init_configs(void) {
  if (s_configs_initialized)
    return;
  for (int i = 0; i < NOVA_PLATFORM_COUNT; i++) {
    s_configs[i].platform = (NovaPlatformId)i;
    s_configs[i].matmul_tile_m = 8;
    s_configs[i].matmul_tile_k = 64;
    s_configs[i].matmul_tile_n = 4;
    s_configs[i].prefetch_distance = 4;
    s_configs[i].attn_tile_size = 32;
    s_configs[i].gpu_threadgroup_x = 16;
    s_configs[i].gpu_threadgroup_y = 16;
    s_configs[i].gpu_block_size = 256;
    s_configs[i].calibrated = false;
  }
  s_configs_initialized = 1;
}

void nova_autocal_detect_hardware(void) {
  init_configs();
  memset(s_configs[NOVA_PLATFORM_CPU].device_name, 0, sizeof(s_configs[0].device_name));

#ifdef __APPLE__
  size_t sz = sizeof(s_configs[NOVA_PLATFORM_CPU].device_name) - 1;
  if (sysctlbyname("hw.model", s_configs[NOVA_PLATFORM_CPU].device_name, &sz, NULL, 0) != 0)
    strncpy(s_configs[NOVA_PLATFORM_CPU].device_name, "Apple Silicon/Intel", sizeof(s_configs[0].device_name) - 1);
#if defined(__arm64__) || defined(__aarch64__)
  strncat(s_configs[NOVA_PLATFORM_CPU].device_name, " (ARM)", 24);
#else
  strncat(s_configs[NOVA_PLATFORM_CPU].device_name, " (x86)", 24);
#endif
#elif defined(__linux__)
  strncpy(s_configs[NOVA_PLATFORM_CPU].device_name, "Linux CPU", sizeof(s_configs[0].device_name) - 1);
#elif defined(_WIN32)
  strncpy(s_configs[NOVA_PLATFORM_CPU].device_name, "Windows CPU", sizeof(s_configs[0].device_name) - 1);
#else
  strncpy(s_configs[NOVA_PLATFORM_CPU].device_name, "Unknown", sizeof(s_configs[0].device_name) - 1);
#endif
}

/** CPU: run existing autotuner and copy best config */
static void calibrate_cpu(void) {
  NovaAutotuner *at = nova_autotuner_create(NULL);
  if (!at)
    return;

  const NovaTunedConfig *tc = nova_get_tuned_config();
  NovaAutocalPlatformConfig *c = &s_configs[NOVA_PLATFORM_CPU];
  c->matmul_tile_m = tc->matmul_tile_m;
  c->matmul_tile_k = tc->matmul_tile_k;
  c->matmul_tile_n = 4;
  c->prefetch_distance = tc->prefetch_distance;
  c->attn_tile_size = tc->attn_tile_size;
  c->peak_gflops = at->hw_bench.flops / 1e9;
  c->memory_gb_s = at->hw_bench.memory_bandwidth;
  c->best_matmul_ns = at->best_cost;
  c->calibrated = true;

  nova_autotuner_destroy(at);
}

#ifdef NOVA_AUTOCAL_USE_DISPATCH
/** GPU: benchmark matmul via dispatch and record best throughput */
static void calibrate_gpu(NovaPlatformId platform) {
  NovaBackendStatus st = nova_backend_status();
  bool available = (platform == NOVA_PLATFORM_METAL && st.metal_available) ||
                  (platform == NOVA_PLATFORM_CUDA && st.cuda_available) ||
                  (platform == NOVA_PLATFORM_ROCM && st.rocm_available) ||
                  (platform == NOVA_PLATFORM_VULKAN && st.vulkan_available) ||
                  (platform == NOVA_PLATFORM_OPENCL && st.opencl_available);
  if (!available)
    return;

  int64_t M = AUTOCAL_MATMUL_M, K = AUTOCAL_MATMUL_K, N = AUTOCAL_MATMUL_N;
  size_t size_a = (size_t)(M * K) * sizeof(float);
  size_t size_b = (size_t)(K * N) * sizeof(float);
  size_t size_c = (size_t)(M * N) * sizeof(float);
  float *a = (float *)malloc(size_a);
  float *b = (float *)malloc(size_b);
  float *c = (float *)malloc(size_c);
  if (!a || !b || !c) {
    free(a); free(b); free(c);
    return;
  }
  for (size_t i = 0; i < (size_t)(M * K); i++) a[i] = (float)(i % 17) * 0.01f;
  for (size_t i = 0; i < (size_t)(K * N); i++) b[i] = (float)(i % 13) * 0.01f;

  for (int w = 0; w < AUTOCAL_WARMUP; w++)
    nova_dispatch_matmul(a, b, c, M, N, K);

  double best_ns = 1e99;
  for (int it = 0; it < AUTOCAL_ITERS; it++) {
    double t0 = now_ns();
    nova_dispatch_matmul(a, b, c, M, N, K);
    double elapsed = now_ns() - t0;
    if (elapsed < best_ns) best_ns = elapsed;
  }

  double flops = (double)(2 * M * N * K);
  double gflops = flops / best_ns;

  NovaAutocalPlatformConfig *cfg = &s_configs[platform];
  {
    NovaBackendStatus cur = nova_backend_status();
    const char *backend_name = nova_backend_name(cur.active);
    strncpy(cfg->device_name, backend_name ? backend_name : "GPU", sizeof(cfg->device_name) - 1);
  }
  cfg->best_matmul_ns = best_ns;
  cfg->peak_gflops = gflops;
  cfg->calibrated = true;
  cfg->gpu_threadgroup_x = 16;
  cfg->gpu_threadgroup_y = 16;

  free(a); free(b); free(c);
}
#endif

void nova_autocal_run_platform(NovaPlatformId platform) {
  init_configs();
  nova_autocal_detect_hardware();

  if (platform == NOVA_PLATFORM_CPU) {
    calibrate_cpu();
    return;
  }
#ifdef NOVA_AUTOCAL_USE_DISPATCH
  calibrate_gpu(platform);
#endif
}

void nova_autocal_run(void) {
  init_configs();
  nova_autocal_detect_hardware();

  /* CPU: always calibrate for max throughput (tile/prefetch) */
  calibrate_cpu();

#ifdef NOVA_AUTOCAL_USE_DISPATCH
  /* Probe backends and calibrate every available GPU for max throughput */
  nova_backend_init(NOVA_BACKEND_AUTO);
  NovaBackendStatus st = nova_backend_status();

  if (st.metal_available) {
    nova_backend_init(NOVA_BACKEND_METAL);
    calibrate_gpu(NOVA_PLATFORM_METAL);
  }
  if (st.cuda_available) {
    nova_backend_init(NOVA_BACKEND_CUDA);
    calibrate_gpu(NOVA_PLATFORM_CUDA);
  }
  if (st.rocm_available) {
    nova_backend_init(NOVA_BACKEND_ROCM);
    calibrate_gpu(NOVA_PLATFORM_ROCM);
  }
  if (st.vulkan_available) {
    nova_backend_init(NOVA_BACKEND_VULKAN);
    calibrate_gpu(NOVA_PLATFORM_VULKAN);
  }
  if (st.opencl_available) {
    nova_backend_init(NOVA_BACKEND_OPENCL);
    calibrate_gpu(NOVA_PLATFORM_OPENCL);
  }
  nova_backend_init(NOVA_BACKEND_AUTO);
#endif
}

const NovaAutocalPlatformConfig *nova_autocal_get_config(NovaPlatformId platform) {
  init_configs();
  if (platform < 0 || platform >= NOVA_PLATFORM_COUNT)
    return NULL;
  return &s_configs[platform];
}

void nova_autocal_report(void) {
  init_configs();
  printf("\n");
  printf("══════════════════════════════════════════════════════════════\n");
  printf("  Nova Autocalibration — Platform Max Throughput\n");
  printf("══════════════════════════════════════════════════════════════\n");

  for (int i = 0; i < NOVA_PLATFORM_COUNT; i++) {
    const NovaAutocalPlatformConfig *c = &s_configs[i];
    if (!c->calibrated)
      continue;
    const char *name = "?";
    switch (c->platform) {
    case NOVA_PLATFORM_CPU:    name = "CPU"; break;
    case NOVA_PLATFORM_METAL: name = "Metal"; break;
    case NOVA_PLATFORM_CUDA:  name = "CUDA"; break;
    case NOVA_PLATFORM_ROCM:  name = "ROCm"; break;
    case NOVA_PLATFORM_VULKAN: name = "Vulkan"; break;
    case NOVA_PLATFORM_OPENCL: name = "OpenCL"; break;
    default: break;
    }
    printf("  [%s] %s\n", name, c->device_name);
    printf("      MatMul tile M=%d K=%d  prefetch=%d  attn_tile=%d\n",
           c->matmul_tile_m, c->matmul_tile_k, c->prefetch_distance, c->attn_tile_size);
    printf("      Best matmul: %.1f ns  Peak: %.2f GFLOPS  Mem: %.2f GB/s\n",
           c->best_matmul_ns, c->peak_gflops, c->memory_gb_s);
    if (c->platform != NOVA_PLATFORM_CPU)
      printf("      GPU threadgroup: %dx%d\n", c->gpu_threadgroup_x, c->gpu_threadgroup_y);
    printf("\n");
  }
  printf("══════════════════════════════════════════════════════════════\n\n");
}

void nova_autocal_export_baseline(const char *filepath) {
  if (!filepath) return;
  init_configs();
  FILE *f = fopen(filepath, "w");
  if (!f) return;
  fprintf(f, "{\n  \"nova_autocal\": \"platform_max_throughput\",\n  \"platforms\": [\n");
  int first = 1;
  for (int i = 0; i < NOVA_PLATFORM_COUNT; i++) {
    const NovaAutocalPlatformConfig *c = &s_configs[i];
    if (!c->calibrated) continue;
    if (!first) fprintf(f, ",\n");
    const char *name = "cpu";
    if (c->platform == NOVA_PLATFORM_METAL) name = "metal";
    else if (c->platform == NOVA_PLATFORM_CUDA) name = "cuda";
    else if (c->platform == NOVA_PLATFORM_ROCM) name = "rocm";
    else if (c->platform == NOVA_PLATFORM_VULKAN) name = "vulkan";
    else if (c->platform == NOVA_PLATFORM_OPENCL) name = "opencl";
    fprintf(f, "    {\"id\": \"%s\", \"tile_m\": %d, \"tile_k\": %d, \"prefetch\": %d, \"gflops\": %.2f, \"mem_gb_s\": %.2f}",
            name, c->matmul_tile_m, c->matmul_tile_k, c->prefetch_distance, c->peak_gflops, c->memory_gb_s);
    first = 0;
  }
  fprintf(f, "\n  ]\n}\n");
  fclose(f);
}
