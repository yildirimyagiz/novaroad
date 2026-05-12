// ═══════════════════════════════════════════════════════════════════════════
// nova_mirror.c - Zero-Copy Delta Encoding & Remote Reconstruction
// ═══════════════════════════════════════════════════════════════════════════

#include "nova_mirror.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// CREATE / DESTROY
// ═══════════════════════════════════════════════════════════════════════════

ZMirrorState *zmirror_create(ZMirrorDataType type, uint64_t data_size) {
  ZMirrorState *s = calloc(1, sizeof(ZMirrorState));
  s->type = type;
  s->base_size = data_size;
  s->base_data = calloc(1, data_size);

  // Sensible defaults
  s->fp32_threshold = 1e-5f;
  s->uint8_threshold = 2;  // Ignore noise < 2 pixel values
  s->int16_threshold = 16; // Ignore noise < 16 sample values
  return s;
}

void zmirror_destroy(ZMirrorState *state) {
  if (!state)
    return;
  free(state->base_data);
  free(state);
}

void zmirror_set_threshold_fp32(ZMirrorState *s, float t) {
  s->fp32_threshold = t;
}
void zmirror_set_threshold_uint8(ZMirrorState *s, uint8_t t) {
  s->uint8_threshold = t;
}
void zmirror_set_threshold_int16(ZMirrorState *s, int16_t t) {
  s->int16_threshold = t;
}

// ═══════════════════════════════════════════════════════════════════════════
// ELEMENT-LEVEL COMPARISON (type-aware)
// ═══════════════════════════════════════════════════════════════════════════

static inline int is_different_fp32(const float *a, const float *b,
                                    float thresh) {
  return fabsf(*a - *b) > thresh;
}

static inline int is_different_uint8(const uint8_t *a, const uint8_t *b,
                                     uint8_t thresh) {
  int diff = (int)*a - (int)*b;
  return (diff > thresh || diff < -thresh);
}

static inline int is_different_int16(const int16_t *a, const int16_t *b,
                                     int16_t thresh) {
  int diff = (int)*a - (int)*b;
  return (diff > thresh || diff < -thresh);
}

static inline int is_different_raw(const uint8_t *a, const uint8_t *b) {
  return *a != *b;
}

// ═══════════════════════════════════════════════════════════════════════════
// COMPUTE PATCH: Scan for changed regions, pack into compact format
// ═══════════════════════════════════════════════════════════════════════════

ZMirrorPatch *zmirror_compute_patch(ZMirrorState *state, const void *new_data,
                                    uint64_t new_size) {
  if (!state || !new_data)
    return NULL;

  const uint8_t *old = (const uint8_t *)state->base_data;
  const uint8_t *cur = (const uint8_t *)new_data;
  uint64_t size = (new_size < state->base_size) ? new_size : state->base_size;

  // Determine element size based on type
  int elem_size = 1;
  switch (state->type) {
  case ZMIRROR_TYPE_FP32:
    elem_size = 4;
    break;
  case ZMIRROR_TYPE_INT16:
    elem_size = 2;
    break;
  case ZMIRROR_TYPE_UINT8:
    elem_size = 1;
    break;
  case ZMIRROR_TYPE_RAW:
    elem_size = 1;
    break;
  }

  uint64_t num_elements = size / elem_size;

  // Pass 1: Find changed elements (build bitmap)
  // We use run-length encoding: find contiguous changed regions
  uint32_t max_regions = 4096;
  ZMirrorPatchRegion *regions =
      malloc(max_regions * sizeof(ZMirrorPatchRegion));
  uint32_t num_regions = 0;
  uint64_t total_changed_bytes = 0;
  uint64_t total_same = 0;

  int in_diff = 0;
  uint32_t diff_start = 0;

  for (uint64_t i = 0; i < num_elements; i++) {
    int different = 0;

    switch (state->type) {
    case ZMIRROR_TYPE_FP32:
      different = is_different_fp32((const float *)(old + i * 4),
                                    (const float *)(cur + i * 4),
                                    state->fp32_threshold);
      break;
    case ZMIRROR_TYPE_UINT8:
      different = is_different_uint8(old + i, cur + i, state->uint8_threshold);
      break;
    case ZMIRROR_TYPE_INT16:
      different = is_different_int16((const int16_t *)(old + i * 2),
                                     (const int16_t *)(cur + i * 2),
                                     state->int16_threshold);
      break;
    case ZMIRROR_TYPE_RAW:
      different = is_different_raw(old + i, cur + i);
      break;
    }

    if (different) {
      if (!in_diff) {
        diff_start = (uint32_t)(i * elem_size);
        in_diff = 1;
      }
    } else {
      total_same++;
      if (in_diff) {
        // Close the current diff region
        // Merge close regions (within 8 bytes) to reduce overhead
        uint32_t end = (uint32_t)(i * elem_size);

        if (num_regions > 0 && diff_start - (regions[num_regions - 1].offset +
                                             regions[num_regions - 1].length) <
                                   8) {
          // Merge with previous region
          regions[num_regions - 1].length =
              end - regions[num_regions - 1].offset;
        } else {
          if (num_regions >= max_regions) {
            max_regions *= 2;
            regions =
                realloc(regions, max_regions * sizeof(ZMirrorPatchRegion));
          }
          regions[num_regions].offset = diff_start;
          regions[num_regions].length = end - diff_start;
          num_regions++;
        }
        total_changed_bytes += (end - diff_start);
        in_diff = 0;
      }
    }
  }

  // Handle trailing diff region
  if (in_diff) {
    uint32_t end = (uint32_t)(num_elements * elem_size);
    if (num_regions > 0 && diff_start - (regions[num_regions - 1].offset +
                                         regions[num_regions - 1].length) <
                               8) {
      regions[num_regions - 1].length = end - regions[num_regions - 1].offset;
    } else {
      if (num_regions >= max_regions) {
        max_regions *= 2;
        regions = realloc(regions, max_regions * sizeof(ZMirrorPatchRegion));
      }
      regions[num_regions].offset = diff_start;
      regions[num_regions].length = end - diff_start;
      num_regions++;
    }
    total_changed_bytes += (end - diff_start);
  }

  // Pass 2: Copy changed data into patch buffer
  uint32_t patch_data_size = 0;
  for (uint32_t r = 0; r < num_regions; r++)
    patch_data_size += regions[r].length;

  uint8_t *patch_data = NULL;
  if (patch_data_size > 0) {
    patch_data = malloc(patch_data_size);
    uint32_t offset = 0;
    for (uint32_t r = 0; r < num_regions; r++) {
      memcpy(patch_data + offset, cur + regions[r].offset, regions[r].length);
      offset += regions[r].length;
    }
  }

  // Build patch object
  ZMirrorPatch *patch = calloc(1, sizeof(ZMirrorPatch));
  patch->type = state->type;
  patch->strategy_used = ZMIRROR_STRATEGY_DELTA;
  patch->original_size = size;
  patch->num_regions = num_regions;
  patch->regions = regions;
  patch->patch_data = patch_data;
  patch->patch_data_size = patch_data_size;

  // Total patch overhead = region headers + data
  uint64_t header_size =
      num_regions * sizeof(ZMirrorPatchRegion) + sizeof(ZMirrorPatch);
  patch->patch_size = header_size + patch_data_size;
  patch->compression_ratio =
      (patch->patch_size > 0) ? (double)size / (double)patch->patch_size : 0.0;
  patch->similarity = (double)total_same / (double)num_elements;

  // Update stats
  state->frame_count++;
  state->total_bytes_original += size;
  state->total_bytes_saved += (size - patch->patch_size);

  return patch;
}

// ═══════════════════════════════════════════════════════════════════════════
// APPLY PATCH: Reconstruct new data from base + patch (receiver side)
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_apply_patch(const ZMirrorPatch *patch, void *base_data,
                         uint64_t base_size) {
  if (!patch || !base_data)
    return;
  (void)base_size;

  uint8_t *dst = (uint8_t *)base_data;
  uint32_t data_offset = 0;

  for (uint32_t r = 0; r < patch->num_regions; r++) {
    memcpy(dst + patch->regions[r].offset, patch->patch_data + data_offset,
           patch->regions[r].length);
    data_offset += patch->regions[r].length;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// UPDATE BASE STATE
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_update_base(ZMirrorState *state, const void *new_data,
                         uint64_t size) {
  if (!state || !new_data)
    return;
  uint64_t copy_size = (size < state->base_size) ? size : state->base_size;
  memcpy(state->base_data, new_data, copy_size);
}

// ═══════════════════════════════════════════════════════════════════════════
// FREE PATCH
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_free_patch(ZMirrorPatch *patch) {
  if (!patch)
    return;
  free(patch->regions);
  free(patch->patch_data);
  free(patch);
}

// ═══════════════════════════════════════════════════════════════════════════
// STATS
// ═══════════════════════════════════════════════════════════════════════════

void zmirror_print_stats(const ZMirrorState *state) {
  if (!state)
    return;
  const char *type_names[] = {"FP32", "UINT8 (Image)", "INT16 (Audio)",
                              "RAW (File)"};

  double savings_pct = (state->total_bytes_original > 0)
                           ? 100.0 * (double)state->total_bytes_saved /
                                 (double)state->total_bytes_original
                           : 0.0;

  printf("\n  ╔═══════════════════════════════════════════════════╗\n");
  printf("  ║  NovaMirror Stats                              ║\n");
  printf("  ╠═══════════════════════════════════════════════════╣\n");
  printf("  ║  Type:    %-20s                 ║\n", type_names[state->type]);
  printf("  ║  Frames:  %-10llu                              ║\n",
         state->frame_count);
  printf("  ║  Original: %10llu bytes                      ║\n",
         state->total_bytes_original);
  printf("  ║  Saved:    %10llu bytes (%5.1f%%)              ║\n",
         state->total_bytes_saved, savings_pct);
  printf("  ╚═══════════════════════════════════════════════════╝\n\n");
}
