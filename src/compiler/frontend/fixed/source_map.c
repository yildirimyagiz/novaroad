/**
 * @file source_map.c
 * @brief Source location tracking
 *
 * Fixes vs original:
 *  - locations array was never allocated — reads from it were UB
 *  - nova_source_map_get did not check locations==NULL before indexing
 *  - nova_source_map_clear() added so memory can be reclaimed
 */

#include "source_map.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *filename; /* interned — not owned */
  int         line;
  int         column;
} source_location_t;

static source_location_t *locations     = NULL;
static size_t              num_locations = 0;
static size_t              cap_locations = 0;

void nova_source_map_add(const char *filename, int line, int column) {
  /* FIX: actually store the location instead of only incrementing a counter */
  if (num_locations >= cap_locations) {
    size_t new_cap = cap_locations == 0 ? 64 : cap_locations * 2;
    source_location_t *tmp =
        realloc(locations, sizeof(source_location_t) * new_cap);
    if (!tmp) return; /* OOM: silently drop rather than crash */
    locations    = tmp;
    cap_locations = new_cap;
  }
  locations[num_locations].filename = filename;
  locations[num_locations].line     = line;
  locations[num_locations].column   = column;
  num_locations++;
}

void nova_source_map_get(int index, const char **filename,
                         int *line, int *column) {
  /* FIX: guard against NULL locations pointer (was UB in original) */
  if (!locations || index < 0 || (size_t)index >= num_locations) {
    *filename = "unknown";
    *line     = 0;
    *column   = 0;
    return;
  }
  *filename = locations[index].filename;
  *line     = locations[index].line;
  *column   = locations[index].column;
}

void nova_source_map_clear(void) {
  free(locations);
  locations     = NULL;
  num_locations = 0;
  cap_locations = 0;
}
