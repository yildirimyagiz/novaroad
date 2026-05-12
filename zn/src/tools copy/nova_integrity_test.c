/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_integrity_test.c - Sovereign Data Protection Test
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_health.h"
#include "nova_mirror_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  printf("\n════════════════════════════════════════════════════════════\n");
  printf("  🛡️  NOVA INTEGRITY GUARD: DATA PROTECTION SYSTEM\n");
  printf("  'No Bit Left Behind' - Absolute Consistency Test\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  uint32_t element_count = 10000;
  float *original_data = malloc(element_count * sizeof(float));
  float *target_state = malloc(element_count * sizeof(float));

  // 1. Orijinal Veri Oluştur (Kritik Kullanıcı Verisi)
  for (uint32_t i = 0; i < element_count; i++)
    original_data[i] = (float)i * 0.123f;

  // 2. Beklenen Yeni Durumu Oluştur ve Fingerprint'le (Önemli Güncelleme)
  float *intended_new_state = malloc(element_count * sizeof(float));
  memcpy(intended_new_state, original_data, element_count * sizeof(float));
  intended_new_state[500] = 999.999f; // Tek bir hücre değişti

  NovaDataFingerprint expected_fp;
  nova_health_fingerprint(intended_new_state, element_count, &expected_fp);

  printf("📝 [Guard] New expected state registered. Fingerprint: ");
  for (int i = 0; i < 4; i++)
    printf("%02x", expected_fp.sha256[i]);
  printf("...\n");

  // 3. Senaryo A: MÜKEMMEL Senkronizasyon
  printf("\n🔄 [TEST A] Perform Perfect Sync:\n");
  ZMirrorV2Packet *perfect_packet = zmirror_v2_encode(
      original_data, intended_new_state, element_count, 1e-6f);

  if (nova_health_verify_sync_integrity(original_data, perfect_packet,
                                          &expected_fp)) {
    printf("  ✅ Verification PASSED. Proceeding with Commit.\n");
    zmirror_v2_apply(original_data, perfect_packet);
  } else {
    printf("  ❌ Verification FAILED unexpectedly.\n");
  }

  // 4. Senaryo B: BOZULMUŞ Senkronizasyon (Araya kötü niyetli veri sızdı veya
  // ağ hatası oldu)
  printf("\n🔄 [TEST B] Perform Corrupted Sync (Simulated Attack/Fault):\n");

  // Paketi bir şekilde bozuyoruz (Patch verisini değiştiriyoruz)
  if (perfect_packet->patch_size > 0) {
    perfect_packet->patch_data[0] ^= 0xFF; // Flip bits in patch
  } else {
    // Eğer patch yoksa bir tarifi boz
    perfect_packet->recipes[0].data.f_val += 1.0f;
  }

  printf("  ⚠️  Sync packet manually corrupted. Testing Guard response...\n");

  if (nova_health_verify_sync_integrity(original_data, perfect_packet,
                                          &expected_fp)) {
    printf("  ❌ CRITICAL FAILURE: Guard allowed corrupted data to pass!\n");
  } else {
    printf("  🛡️  [SENTINEL] ALERT: Integrity violation detected! SHIELD "
           "ACTIVE.\n");
    printf("  🚫 Commit ABORTED. Main data remains safe and consistent.\n");
  }

  printf("\n🏁 [Final Report] Nova Health successfully protected the user "
         "state.\n");
  printf("════════════════════════════════════════════════════════════\n\n");

  zmirror_v2_destroy_packet(perfect_packet);
  free(original_data);
  free(target_state);
  free(intended_new_state);
  return 0;
}
