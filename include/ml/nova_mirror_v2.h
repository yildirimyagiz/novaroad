/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_mirror_v2.h - Pattern-based Reconstruction Recipes
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_MIRROR_V2_H
#define NOVA_MIRROR_V2_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  ZMIRROR_RECIPE_DELTA,    // Standart fark (v1)
  ZMIRROR_RECIPE_REPEAT,   // Tüm değerler aynı (A, A, A, ...)
  ZMIRROR_RECIPE_LINEAR,   // Aritmetik dizi (A, A+S, A+2S, ...)
  ZMIRROR_RECIPE_ZERO_RUN, // Uzun sıfır dizisi
  ZMIRROR_RECIPE_SPARSE,   // Sadece belirli indexlerde veri var
} ZMirrorRecipeType;

typedef struct {
  uint8_t type;    // ZMirrorRecipeType
  uint32_t offset; // Verinin başladığı yer
  uint32_t length; // Kaç element etkilendi
  union {
    float f_val; // REPEAT için değer
    struct {
      float start;
      float step;
    } linear;              // LINEAR için parametreler
    uint32_t patch_offset; // Delta verisinin patch buffer içindeki yeri
  } data;
} ZMirrorRecipe;

typedef struct {
  ZMirrorRecipe *recipes;
  uint32_t num_recipes;
  uint8_t *patch_data;
  uint32_t patch_size;

  // 🛡️ [Sentinel] Security Header
  uint8_t signature[32]; // SHA-256 Integrity Hash
  uint64_t timestamp;    // Anti-replay
  uint32_t sequence;     // Global sequence
} ZMirrorV2Packet;

// API
ZMirrorV2Packet *zmirror_v2_encode(const float *prev, const float *curr,
                                   uint32_t count, float threshold);
void zmirror_v2_apply(float *target, const ZMirrorV2Packet *packet);
void zmirror_v2_destroy_packet(ZMirrorV2Packet *packet);

#endif // NOVA_MIRROR_V2_H
