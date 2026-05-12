#include "nova_mirror_v2.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MIN_PATTERN_LENGTH 8

ZMirrorV2Packet *zmirror_v2_encode(const float *prev, const float *curr,
                                   uint32_t count, float threshold) {
  ZMirrorV2Packet *packet = calloc(1, sizeof(ZMirrorV2Packet));
  packet->recipes =
      malloc(sizeof(ZMirrorRecipe) * (count / MIN_PATTERN_LENGTH + 1));
  packet->patch_data = malloc(count * sizeof(float));

  uint32_t r_idx = 0;
  uint32_t p_offset = 0;

  for (uint32_t i = 0; i < count;) {
    // 1. Durum: Değişiklik yok (Skip)
    bool is_same = false;
    if (threshold == 0.0f) {
      is_same = (*(uint32_t *)&curr[i] == *(uint32_t *)&prev[i]);
    } else {
      is_same = (fabsf(curr[i] - prev[i]) < threshold);
    }

    if (is_same) {
      i++;
      continue;
    }

    uint32_t start_idx = i;

    // 2. Durum: REPEAT Pattern Analizi (Örn: Hepsi 0.5)
    uint32_t rep_len = 1;
    while (i + rep_len < count &&
           fabsf(curr[i + rep_len] - curr[i]) < threshold &&
           fabsf(curr[i + rep_len] - prev[i + rep_len]) >= threshold) {
      rep_len++;
    }

    if (rep_len >= MIN_PATTERN_LENGTH) {
      packet->recipes[r_idx++] = (ZMirrorRecipe){.type = ZMIRROR_RECIPE_REPEAT,
                                                 .offset = start_idx,
                                                 .length = rep_len,
                                                 .data.f_val = curr[start_idx]};
      i += rep_len;
      continue;
    }

    // 3. Durum: LINEAR Pattern Analizi (Örn: 0.1, 0.2, 0.3...)
    if (i + 2 < count) {
      float step = curr[i + 1] - curr[i];
      uint32_t lin_len = 2;
      while (i + lin_len < count &&
             fabsf((curr[i + lin_len] - curr[i + lin_len - 1]) - step) <
                 threshold) {
        lin_len++;
      }
      if (lin_len >= MIN_PATTERN_LENGTH) {
        packet->recipes[r_idx++] = (ZMirrorRecipe){
            .type = ZMIRROR_RECIPE_LINEAR,
            .offset = start_idx,
            .length = lin_len,
            .data.linear = {.start = curr[start_idx], .step = step}};
        i += lin_len;
        continue;
      }
    }

    // 4. Durum: Standart DELTA (Patch)
    packet->recipes[r_idx++] = (ZMirrorRecipe){.type = ZMIRROR_RECIPE_DELTA,
                                               .offset = start_idx,
                                               .length = 1,
                                               .data.patch_offset = p_offset};
    memcpy(packet->patch_data + p_offset, &curr[i], sizeof(float));
    p_offset += sizeof(float);
    i++;
  }

  packet->num_recipes = r_idx;
  packet->patch_size = p_offset;
  return packet;
}

void zmirror_v2_apply(float *target, const ZMirrorV2Packet *packet) {
  if (!target || !packet)
    return;

  for (uint32_t i = 0; i < packet->num_recipes; i++) {
    ZMirrorRecipe *r = &packet->recipes[i];
    switch (r->type) {
    case ZMIRROR_RECIPE_REPEAT:
      for (uint32_t k = 0; k < r->length; k++)
        target[r->offset + k] = r->data.f_val;
      break;
    case ZMIRROR_RECIPE_LINEAR:
      for (uint32_t k = 0; k < r->length; k++)
        target[r->offset + k] = r->data.linear.start + k * r->data.linear.step;
      break;
    case ZMIRROR_RECIPE_DELTA:
      memcpy(&target[r->offset], packet->patch_data + r->data.patch_offset,
             sizeof(float));
      break;
    }
  }
}

void zmirror_v2_destroy_packet(ZMirrorV2Packet *packet) {
  if (packet) {
    free(packet->recipes);
    free(packet->patch_data);
    free(packet);
  }
}
