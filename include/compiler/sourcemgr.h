// SourceManager Header
// File registry and source buffer management for Nova compiler

#ifndef NOVA_SOURCEMGR_H
#define NOVA_SOURCEMGR_H

#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// SourceManager API
// ═══════════════════════════════════════════════════════════════════════════

typedef struct nova_sourcemgr nova_sourcemgr_t;

// Lifecycle
nova_sourcemgr_t* nova_sourcemgr_create(void);
void nova_sourcemgr_destroy(nova_sourcemgr_t* sm);

// File loading
int nova_sourcemgr_load_file(nova_sourcemgr_t* sm, const char* filename); // Returns file_id

// Source access
const char* nova_sourcemgr_get_filename(nova_sourcemgr_t* sm, int file_id);
const unsigned char* nova_sourcemgr_get_source(nova_sourcemgr_t* sm, int file_id, size_t* out_length);

// Line/column conversion
void nova_sourcemgr_offset_to_line_col(nova_sourcemgr_t* sm, int file_id,
                                     size_t offset, int* out_line, int* out_col);

// Line content
const char* nova_sourcemgr_get_line(nova_sourcemgr_t* sm, int file_id, int line,
                                  size_t* out_length);

// Statistics
size_t nova_sourcemgr_get_file_count(nova_sourcemgr_t* sm);

#endif // NOVA_SOURCEMGR_H
