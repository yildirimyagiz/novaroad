#include "../../include/nova_autotune_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaAutoTuneV2 *nova_autotune_init(const char *path) {
  NovaAutoTuneV2 *at = calloc(1, sizeof(NovaAutoTuneV2));
  at->persistence_path = strdup(path);
  at->profiles = calloc(100, sizeof(HardwareProfile));

  nova_autotune_load(at);

  printf("⚙️ Nova Auto-Tune V2 Initialized (Registry: %s)\n", path);
  return at;
}

void nova_autotune_shutdown(NovaAutoTuneV2 *at) {
  if (!at)
    return;
  nova_autotune_save(at);
  free(at->profiles);
  free(at->persistence_path);
  free(at);
}

TuningParams nova_autotune_get_params(NovaAutoTuneV2 *at,
                                        const char *hw_id) {
  for (size_t i = 0; i < at->profile_count; i++) {
    if (strcmp(at->profiles[i].hw_id, hw_id) == 0) {
      return at->profiles[i].best_params;
    }
  }

  // Default safe parameters
  TuningParams defaults = {.unroll_factor = 4,
                           .vector_width = 256,
                           .tile_size = 32,
                           .cache_line_size = 64,
                           .enable_prefetching = true,
                           .pipeline_depth = 8};
  return defaults;
}

void nova_autotune_record_result(NovaAutoTuneV2 *at, const char *hw_id,
                                   TuningParams params, double latency) {
  HardwareProfile *prof = NULL;

  for (size_t i = 0; i < at->profile_count; i++) {
    if (strcmp(at->profiles[i].hw_id, hw_id) == 0) {
      prof = &at->profiles[i];
      break;
    }
  }

  if (!prof) {
    prof = &at->profiles[at->profile_count++];
    strncpy(prof->hw_id, hw_id, 63);
    prof->best_params = params;
    prof->measured_latency = latency;
  } else if (latency < prof->measured_latency) {
    printf("📈 Auto-Tune: New best parameters for %s! (%.2f ns -> %.2f ns)\n",
           hw_id, prof->measured_latency, latency);
    prof->best_params = params;
    prof->measured_latency = latency;
  }
}

void nova_autotune_save(NovaAutoTuneV2 *at) {
  FILE *f = fopen(at->persistence_path, "wb");
  if (!f)
    return;

  fwrite(&at->profile_count, sizeof(size_t), 1, f);
  fwrite(at->profiles, sizeof(HardwareProfile), at->profile_count, f);
  fclose(f);
}

void nova_autotune_load(NovaAutoTuneV2 *at) {
  FILE *f = fopen(at->persistence_path, "rb");
  if (!f)
    return;

  fread(&at->profile_count, sizeof(size_t), 1, f);
  fread(at->profiles, sizeof(HardwareProfile), at->profile_count, f);
  fclose(f);
}
