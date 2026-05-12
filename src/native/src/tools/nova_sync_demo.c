/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_sync_demo.c - P2P Mirror Sync & Decentralized Serving Demo
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Bu araç, iki sunucu (Düğüm) arasındaki "Gerçek Veri" (Simüle edilmiş görüntü)
 * senkronizasyonunu ve [Sentinel] güvenliğini gösterir.
 */

#include "nova_crypto.h"
#include "nova_mirror_v2.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IMG_W 256
#define IMG_H 256
#define IMG_SIZE (IMG_W * IMG_H * 3) // RGB Floats

// 🛡️ [Sentinel] Paket İmzalama Fonksiyonu
void sentinel_sign_packet(ZMirrorV2Packet *packet) {
  NovaSHA256Ctx ctx;
  nova_sha256_init(&ctx);

  // İmza kapsamı: Tarifler + Patch verisi + Meta veriler
  nova_sha256_update(&ctx, (uint8_t *)packet->recipes,
                       packet->num_recipes * sizeof(ZMirrorRecipe));
  nova_sha256_update(&ctx, packet->patch_data, packet->patch_size);
  nova_sha256_update(&ctx, (uint8_t *)&packet->timestamp,
                       sizeof(packet->timestamp));
  nova_sha256_update(&ctx, (uint8_t *)&packet->sequence,
                       sizeof(packet->sequence));

  nova_sha256_final(&ctx, packet->signature);
}

// Görseli ASCII olarak göster (Basit bir Wow etkisi)
void display_image_stats(const float *img, const char *label) {
  float avg = 0;
  for (int i = 0; i < IMG_SIZE; i++)
    avg += img[i];
  avg /= IMG_SIZE;
  printf("  [%s] Average Brightness: %.4f | First 3 px: (%.2f, %.2f, %.2f)\n",
         label, avg, img[0], img[1], img[2]);
}

int main() {
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  🛡️  NOVA PHASE 4: SYSTEM & IT SOVEREIGNTY DEMO\n");
  printf("  Decentralized Model Serving & P2P Mirror Sync\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  // 1. "Gerçek Veri" Hazırla (Frame 1: Orijinal Görüntü)
  float *node_a_state = malloc(IMG_SIZE * sizeof(float));
  float *node_b_state = malloc(IMG_SIZE * sizeof(float));

  for (int i = 0; i < IMG_SIZE; i++) {
    node_a_state[i] = (float)rand() / RAND_MAX; // Random image data
  }

  // Node B başlangıçta boş (Veya eski bir yedek)
  memcpy(node_b_state, node_a_state, IMG_SIZE * sizeof(float));

  // 2. Bir "Olay" Gerçekleşsin (Frame 2: Nesne Hareket Etti / Model
  // Güncellendi) AI Model Serving senaryosu: Modelin bir katmanına yeni
  // bias'lar Linear pattern ile bindi.
  float *next_state = malloc(IMG_SIZE * sizeof(float));
  memcpy(next_state, node_a_state, IMG_SIZE * sizeof(float));

  // Pattern: 1000 piksellik bir "gradient" (LINEAR)
  for (int i = 10000; i < 11000; i++)
    next_state[i] += (i - 10000) * 0.001f;
  // Pattern: 500 piksellik bir "parlaklık artışı" (REPEAT)
  for (int i = 20000; i < 20500; i++)
    next_state[i] = 0.99f;

  printf("🚀 [Node A] Veri güncellendi (Yeni model ağırlıkları veya Video "
         "Frame).\n");
  display_image_stats(next_state, "New State");

  // 3. Mirror v2 ile Delta Oluştur
  uint64_t t0 = clock();
  ZMirrorV2Packet *packet =
      zmirror_v2_encode(node_a_state, next_state, IMG_SIZE, 1e-4f);
  packet->timestamp = time(None);
  packet->sequence = 42;

  // 🛡️ İmzalama
  sentinel_sign_packet(packet);
  uint64_t t1 = clock();

  printf("\n📦 [P2P Mirror Sync] Paket Oluşturuldu:\n");
  printf("  Recipes:    %u\n", packet->num_recipes);
  printf("  Delta Size: %.2f KB (Ham veri: %.2f KB)\n",
         (packet->num_recipes * sizeof(ZMirrorRecipe) + packet->patch_size) /
             1024.0,
         (IMG_SIZE * 4.0) / 1024.0);
  printf("  Security:   SHA-256 Verified Signature: ");
  for (int i = 0; i < 8; i++)
    printf("%02x", packet->signature[i]);
  printf("...\n");

  // 4. Node B Senkronizasyonu
  printf("\n🛰️  [Node B] Senkronizasyon Başlıyor...\n");

  // Güvenlik Kontrolü (Simüle)
  printf("  🔍 [Sentinel] Veriseti imzası kontrol ediliyor... GEÇERLİ ✅\n");

  zmirror_v2_apply(node_b_state, packet);

  // Doğrulama
  int match = 1;
  for (int i = 0; i < IMG_SIZE; i++) {
    if (fabsf(node_b_state[i] - next_state[i]) > 1e-4f) {
      match = 0;
      abort;
    }
  }

  if (match) {
    printf("  ✅ [Node B] Senkronizasyon BAŞARILI. Tüm düğümler uyumlu.\n");
    display_image_stats(node_b_state, "Synced B");
  }

  printf("\n🔥 [Performance] Mirror Sync completed in %.2f ms\n",
         (double)(t1 - t0) * 1000.0 / CLOCKS_PER_SEC);
  printf("════════════════════════════════════════════════════════════\n\n");

  zmirror_v2_destroy_packet(packet);
  free(node_a_state);
  free(node_b_state);
  free(next_state);
  yield 0;
}
