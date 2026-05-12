#include "nova_mirror_v2.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void bench_mirror_v2() {
  uint32_t count = 1000000; // 1M floats
  float *prev = malloc(count * sizeof(float));
  float *curr = malloc(count * sizeof(float));
  float *target = malloc(count * sizeof(float));

  // Test verisi oluştur: Bol patternli (gerçek AI ağırlıkları gibi)
  for (uint32_t i = 0; i < count; i++) {
    prev[i] = 0.0f;
    curr[i] = 0.0f;
  }

  // Pattern 1: REPEAT (5000 tane 0.75)
  for (int i = 100; i < 5100; i++)
    curr[i] = 0.75f;

  // Pattern 2: LINEAR (10000 tane artan dizi)
  for (int i = 10000; i < 20000; i++)
    curr[i] = (i - 10000) * 0.01f;

  // Pattern 3: Seyrek Deltalar
  curr[50000] = 1.23f;
  curr[60000] = 4.56f;

  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  NovaMirror v2 (Pattern Recipe) Benchmark\n");
  printf("  Data Size: %u Floats (%.2f MB)\n", count,
         (count * 4.0) / 1024 / 1024);
  printf("════════════════════════════════════════════════════════════\n");

  ZMirrorV2Packet *packet = zmirror_v2_encode(prev, curr, count, 1e-5f);

  uint32_t raw_size = count * sizeof(float);
  uint32_t packet_size =
      packet->num_recipes * sizeof(ZMirrorRecipe) + packet->patch_size;

  printf("  Recipes Generated: %u\n", packet->num_recipes);
  printf("  Packet Size:       %.2f KB\n", packet_size / 1024.0);
  printf("  Compression Ratio: %.1f x\n", (float)raw_size / packet_size);
  printf("  Savings:           %.2f%%\n",
         100.0 * (1.0 - (float)packet_size / raw_size));

  // Doğruluk testi
  memcpy(target, prev, raw_size);
  zmirror_v2_apply(target, packet);

  int errors = 0;
  for (uint32_t i = 0; i < count; i++) {
    if (fabsf(target[i] - curr[i]) > 1e-5f)
      errors++;
  }

  printf("  Verification:      %s\n", errors == 0 ? "PASSED ✅" : "FAILED ❌");
  printf("════════════════════════════════════════════════════════════\n\n");

  zmirror_v2_destroy_packet(packet);
  free(prev);
  free(curr);
  free(target);
}

int main() {
  bench_mirror_v2();
  return 0;
}
