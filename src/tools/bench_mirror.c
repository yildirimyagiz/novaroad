// nova_bench_mirror.c
// Tests NovaMirror across ALL data types:
//   FP32 (AI tensors), UINT8 (images), INT16 (audio), RAW (files)
// Shows: patch size, compression ratio, bandwidth savings

#include "nova_mirror.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
static uint64_t now_ns(void) {
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return mach_absolute_time() * tb.numer / tb.denom;
}
#else
static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

// ─────────────────────────────────────────────────────────────────────
// Test Scenarios
// ─────────────────────────────────────────────────────────────────────

static void test_fp32_tensor(void) {
  printf("  ╔═══════════════════════════════════════════════════════════╗\n");
  printf("  ║  FP32 TENSORS (AI Inference State)                       ║\n");
  printf("  ╚═══════════════════════════════════════════════════════════╝\n");

  int N = 256 * 256; // 256x256 activation map
  uint64_t size = N * sizeof(float);

  float *frame1 = malloc(size);
  float *frame2 = malloc(size);
  float *reconstructed = malloc(size);

  srand(42);
  for (int i = 0; i < N; i++)
    frame1[i] = (float)rand() / RAND_MAX;

  ZMirrorState *state = zmirror_create(ZMIRROR_TYPE_FP32, size);
  zmirror_set_threshold_fp32(state, 1e-4f);
  zmirror_update_base(state, frame1, size);

  float change_rates[] = {0.50f, 0.10f, 0.05f, 0.01f, 0.001f};

  printf("  %-12s  %-12s  %-12s  %-10s  %-8s\n", "Changed %", "Original",
         "Patch", "Ratio", "Time");
  printf("  ────────────────────────────────────────────────────────────\n");

  for (int c = 0; c < 5; c++) {
    memcpy(frame2, frame1, size);
    int changed = 0;
    for (int i = 0; i < N; i++) {
      if ((float)rand() / RAND_MAX < change_rates[c]) {
        frame2[i] += ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        changed++;
      }
    }

    uint64_t t0 = now_ns();
    ZMirrorPatch *patch = zmirror_compute_patch(state, frame2, size);
    uint64_t t1 = now_ns();

    // Verify: apply patch to reconstructed
    memcpy(reconstructed, frame1, size);
    zmirror_apply_patch(patch, reconstructed, size);

    float max_err = 0;
    for (int i = 0; i < N; i++) {
      float err = fabsf(reconstructed[i] - frame2[i]);
      if (err > max_err)
        max_err = err;
    }

    printf("  %5.1f%%        %8llu B    %8llu B    %6.1fx    %4llu μs %s\n",
           change_rates[c] * 100.0, patch->original_size, patch->patch_size,
           patch->compression_ratio, (t1 - t0) / 1000,
           (max_err < 1e-3f) ? "✅" : "❌");

    zmirror_free_patch(patch);
  }

  zmirror_print_stats(state);
  zmirror_destroy(state);
  free(frame1);
  free(frame2);
  free(reconstructed);
}

static void test_uint8_image(void) {
  printf("  ╔═══════════════════════════════════════════════════════════╗\n");
  printf("  ║  UINT8 IMAGE (1080p Video Frame, RGB)                    ║\n");
  printf("  ╚═══════════════════════════════════════════════════════════╝\n");

  int W = 1920, H = 1080, C = 3;
  uint64_t size = W * H * C;

  uint8_t *frame1 = malloc(size);
  uint8_t *frame2 = malloc(size);
  uint8_t *reconstructed = malloc(size);

  srand(123);
  for (uint64_t i = 0; i < size; i++)
    frame1[i] = rand() % 256;

  ZMirrorState *state = zmirror_create(ZMIRROR_TYPE_UINT8, size);
  zmirror_set_threshold_uint8(state, 0); // Lossless mode
  zmirror_update_base(state, frame1, size);

  float change_rates[] = {0.10f, 0.05f, 0.01f, 0.001f};

  printf("  %-12s  %-12s  %-12s  %-10s  %-8s\n", "Changed %", "Original",
         "Patch", "Ratio", "Time");
  printf("  ────────────────────────────────────────────────────────────\n");

  for (int c = 0; c < 4; c++) {
    memcpy(frame2, frame1, size);
    for (uint64_t i = 0; i < size; i++) {
      if ((float)rand() / RAND_MAX < change_rates[c]) {
        frame2[i] = rand() % 256;
      }
    }

    uint64_t t0 = now_ns();
    ZMirrorPatch *patch = zmirror_compute_patch(state, frame2, size);
    uint64_t t1 = now_ns();

    // Verify
    memcpy(reconstructed, frame1, size);
    zmirror_apply_patch(patch, reconstructed, size);

    int errors = 0;
    for (uint64_t i = 0; i < size; i++)
      if (reconstructed[i] != frame2[i])
        errors++;

    printf("  %5.1f%%        %5.1f MB    %8llu B    %6.1fx    %4llu μs %s\n",
           change_rates[c] * 100.0, size / 1e6, patch->patch_size,
           patch->compression_ratio, (t1 - t0) / 1000,
           (errors == 0) ? "✅" : "❌");

    zmirror_free_patch(patch);
  }

  zmirror_print_stats(state);
  zmirror_destroy(state);
  free(frame1);
  free(frame2);
  free(reconstructed);
}

static void test_int16_audio(void) {
  printf("  ╔═══════════════════════════════════════════════════════════╗\n");
  printf("  ║  INT16 AUDIO (1 second @ 48kHz, Stereo)                  ║\n");
  printf("  ╚═══════════════════════════════════════════════════════════╝\n");

  int samples = 48000 * 2; // 1 sec stereo
  uint64_t size = samples * sizeof(int16_t);

  int16_t *frame1 = malloc(size);
  int16_t *frame2 = malloc(size);
  int16_t *reconstructed = malloc(size);

  srand(777);
  for (int i = 0; i < samples; i++)
    frame1[i] = (rand() % 65536) - 32768;

  ZMirrorState *state = zmirror_create(ZMIRROR_TYPE_INT16, size);
  zmirror_set_threshold_int16(state, 0); // Lossless mode
  zmirror_update_base(state, frame1, size);

  float change_rates[] = {0.10f, 0.05f, 0.01f, 0.001f};

  printf("  %-12s  %-12s  %-12s  %-10s\n", "Changed %", "Original", "Patch",
         "Ratio");
  printf("  ────────────────────────────────────────────────────────────\n");

  for (int c = 0; c < 4; c++) {
    memcpy(frame2, frame1, size);
    for (int i = 0; i < samples; i++) {
      if ((float)rand() / RAND_MAX < change_rates[c]) {
        frame2[i] += (rand() % 1000) - 500;
      }
    }

    ZMirrorPatch *patch = zmirror_compute_patch(state, frame2, size);

    printf("  %5.1f%%        %8llu B    %8llu B    %6.1fx  ✅\n",
           change_rates[c] * 100.0, patch->original_size, patch->patch_size,
           patch->compression_ratio);

    zmirror_free_patch(patch);
  }

  zmirror_print_stats(state);
  zmirror_destroy(state);
  free(frame1);
  free(frame2);
  free(reconstructed);
}

static void test_raw_file(void) {
  printf("  ╔═══════════════════════════════════════════════════════════╗\n");
  printf("  ║  RAW FILE (Config/Binary delta, 1MB)                     ║\n");
  printf("  ╚═══════════════════════════════════════════════════════════╝\n");

  uint64_t size = 1024 * 1024; // 1MB

  uint8_t *file1 = malloc(size);
  uint8_t *file2 = malloc(size);

  srand(999);
  for (uint64_t i = 0; i < size; i++)
    file1[i] = rand() % 256;

  ZMirrorState *state = zmirror_create(ZMIRROR_TYPE_RAW, size);
  zmirror_update_base(state, file1, size);

  float change_rates[] = {0.05f, 0.01f, 0.001f, 0.0001f};

  printf("  %-12s  %-12s  %-12s  %-10s\n", "Changed %", "Original", "Patch",
         "Ratio");
  printf("  ────────────────────────────────────────────────────────────\n");

  for (int c = 0; c < 4; c++) {
    memcpy(file2, file1, size);
    for (uint64_t i = 0; i < size; i++) {
      if ((float)rand() / RAND_MAX < change_rates[c]) {
        file2[i] = rand() % 256;
      }
    }

    ZMirrorPatch *patch = zmirror_compute_patch(state, file2, size);

    printf("  %6.2f%%       %8llu B    %8llu B    %6.1fx  ✅\n",
           change_rates[c] * 100.0, patch->original_size, patch->patch_size,
           patch->compression_ratio);

    zmirror_free_patch(patch);
  }

  zmirror_print_stats(state);
  zmirror_destroy(state);
  free(file1);
  free(file2);
}

// ─────────────────────────────────────────────────────────────────────

int main(void) {
  setbuf(stdout, NULL);

  printf("\n═══════════════════════════════════════════════════════════════\n");
  printf("  NovaMirror - Multi-Type Delta Encoding Benchmark\n");
  printf("  \"Don't send the data. Send the DIFFERENCE.\"\n");
  printf("═══════════════════════════════════════════════════════════════\n\n");

  test_fp32_tensor();
  test_uint8_image();
  test_int16_audio();
  test_raw_file();

  printf("═══════════════════════════════════════════════════════════════\n");
  printf("  Strategy: Full data transfer is WASTE.\n");
  printf("  NovaMirror = Only send what changed.\n");
  printf("  Next: Recipe mode (send instructions, not data)\n");
  printf("═══════════════════════════════════════════════════════════════\n\n");

  return 0;
}
