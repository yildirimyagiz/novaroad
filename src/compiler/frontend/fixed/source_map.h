/**
 * @file source_map.h
 * @brief Source location tracking
 */

#ifndef NOVA_COMPILER_FRONTEND_SOURCE_MAP_H
#define NOVA_COMPILER_FRONTEND_SOURCE_MAP_H

#include <stddef.h>

/**
 * Append a source location entry.
 * @param filename  Interned string — caller must keep it alive.
 * @param line      1-based line number.
 * @param column    1-based column number.
 */
void nova_source_map_add(const char *filename, int line, int column);

/**
 * Look up a location by index.
 * Any of the out-pointers may be NULL if that field is not needed.
 * On out-of-range index, writes "unknown" / 0 / 0 into the pointers.
 */
void nova_source_map_get(int index, const char **filename,
                         int *line, int *column);

/** Return the number of entries currently stored. */
size_t nova_source_map_count(void);

/** Release all heap storage. */
void nova_source_map_free(void);

#endif /* NOVA_COMPILER_FRONTEND_SOURCE_MAP_H */
