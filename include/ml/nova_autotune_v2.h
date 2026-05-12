#ifndef NOVA_AUTOTUNE_V2_H
#define NOVA_AUTOTUNE_V2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA AUTO-TUNE V2
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  int unroll_factor;
  int vector_width;
  int tile_size;
  uint32_t cache_line_size;
  bool enable_prefetching;
  int pipeline_depth;
} TuningParams;

typedef struct {
  char hw_id[64];
  TuningParams best_params;
  double measured_latency;
} HardwareProfile;

typedef struct {
  HardwareProfile *profiles;
  size_t profile_count;
  char *persistence_path;
} NovaAutoTuneV2;

NovaAutoTuneV2 *nova_autotune_init(const char *path);
void nova_autotune_shutdown(NovaAutoTuneV2 *at);

// Tuning API
TuningParams nova_autotune_get_params(NovaAutoTuneV2 *at,
                                        const char *hw_id);
void nova_autotune_record_result(NovaAutoTuneV2 *at, const char *hw_id,
                                   TuningParams params, double latency);

// Persistence
void nova_autotune_save(NovaAutoTuneV2 *at);
void nova_autotune_load(NovaAutoTuneV2 *at);

#endif // NOVA_AUTOTUNE_V2_H
