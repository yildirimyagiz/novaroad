#include <stdbool.h>

// SourceManager Implementation
// File registry and source buffer management for Nova compiler

#include "compiler/sourcemgr.h"
#include "std/file_io.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// SourceManager Implementation
// ═══════════════════════════════════════════════════════════════════════════

struct nova_sourcemgr {
    // File registry
    char** filenames;           // Array of filenames
    unsigned char** sources;    // Array of source buffers
    size_t* lengths;            // Array of source lengths
    int* line_counts;           // Array of line counts per file
    int** line_offsets;         // Array of line start offsets per file
    size_t capacity;            // Current capacity
    size_t count;               // Number of files loaded
};

// Create new source manager
nova_sourcemgr_t* nova_sourcemgr_create(void) {
    nova_sourcemgr_t* sm = (nova_sourcemgr_t*)malloc(sizeof(nova_sourcemgr_t));
    if (!sm) return NULL;

    sm->capacity = 16;
    sm->count = 0;

    sm->filenames = (char**)malloc(sizeof(char*) * sm->capacity);
    sm->sources = (unsigned char**)malloc(sizeof(unsigned char*) * sm->capacity);
    sm->lengths = (size_t*)malloc(sizeof(size_t) * sm->capacity);
    sm->line_counts = (int*)malloc(sizeof(int) * sm->capacity);
    sm->line_offsets = (int**)malloc(sizeof(int*) * sm->capacity);

    if (!sm->filenames || !sm->sources || !sm->lengths ||
        !sm->line_counts || !sm->line_offsets) {
        nova_sourcemgr_destroy(sm);
        return NULL;
    }

    // Initialize arrays
    memset(sm->filenames, 0, sizeof(char*) * sm->capacity);
    memset(sm->sources, 0, sizeof(unsigned char*) * sm->capacity);
    memset(sm->lengths, 0, sizeof(size_t) * sm->capacity);
    memset(sm->line_counts, 0, sizeof(int) * sm->capacity);
    memset(sm->line_offsets, 0, sizeof(int*) * sm->capacity);

    return sm;
}

// Destroy source manager
void nova_sourcemgr_destroy(nova_sourcemgr_t* sm) {
    if (!sm) return;

    for (size_t i = 0; i < sm->count; i++) {
        free(sm->filenames[i]);
        nova_file_free(sm->sources[i]);
        free(sm->line_offsets[i]);
    }

    free(sm->filenames);
    free(sm->sources);
    free(sm->lengths);
    free(sm->line_counts);
    free(sm->line_offsets);
    free(sm);
}

// Build line index for a source buffer
static void build_line_index(unsigned char* source, size_t length,
                           int* out_line_count, int** out_line_offsets) {
    // Count lines and build offset table
    int line_count = 1; // At least one line
    int* offsets = (int*)malloc(sizeof(int) * 1024); // Initial capacity
    int offset_capacity = 1024;
    int offset_count = 0;

    if (!offsets) return;

    offsets[offset_count++] = 0; // First line starts at 0

    for (size_t i = 0; i < length; i++) {
        if (source[i] == '\n') {
            line_count++;
            if (offset_count >= offset_capacity) {
                offset_capacity *= 2;
                int* new_offsets = (int*)realloc(offsets, sizeof(int) * offset_capacity);
                if (!new_offsets) {
                    free(offsets);
                    return;
                }
                offsets = new_offsets;
            }
            offsets[offset_count++] = (int)(i + 1); // Next line starts after \n
        }
    }

    *out_line_count = line_count;
    *out_line_offsets = offsets;
}

// Load file into source manager
int nova_sourcemgr_load_file(nova_sourcemgr_t* sm, const char* filename) {
    if (!sm || !filename) return -1;

    // Check if file already loaded
    for (size_t i = 0; i < sm->count; i++) {
        if (strcmp(sm->filenames[i], filename) == 0) {
            return (int)i; // Return existing file ID
        }
    }

    // Expand capacity if needed
    if (sm->count >= sm->capacity) {
        sm->capacity *= 2;

        sm->filenames = (char**)realloc(sm->filenames, sizeof(char*) * sm->capacity);
        sm->sources = (unsigned char**)realloc(sm->sources, sizeof(unsigned char*) * sm->capacity);
        sm->lengths = (size_t*)realloc(sm->lengths, sizeof(size_t) * sm->capacity);
        sm->line_counts = (int*)realloc(sm->line_counts, sizeof(int) * sm->capacity);
        sm->line_offsets = (int**)realloc(sm->line_offsets, sizeof(int*) * sm->capacity);

        if (!sm->filenames || !sm->sources || !sm->lengths ||
            !sm->line_counts || !sm->line_offsets) {
            return -1;
        }
    }

    // Read file
    size_t length;
    unsigned char* source = nova_file_read_all(filename, &length);
    if (!source) {
        return -1;
    }

    // Build line index
    int line_count;
    int* line_offsets;
    build_line_index(source, length, &line_count, &line_offsets);

    // Store in registry
    size_t index = sm->count++;
    sm->filenames[index] = strdup(filename);
    sm->sources[index] = source;
    sm->lengths[index] = length;
    sm->line_counts[index] = line_count;
    sm->line_offsets[index] = line_offsets;

    return (int)index; // Return file ID
}

// Get source info
const char* nova_sourcemgr_get_filename(nova_sourcemgr_t* sm, int file_id) {
    if (!sm || file_id < 0 || (size_t)file_id >= sm->count) return NULL;
    return sm->filenames[file_id];
}

const unsigned char* nova_sourcemgr_get_source(nova_sourcemgr_t* sm, int file_id, size_t* out_length) {
    if (!sm || file_id < 0 || (size_t)file_id >= sm->count) {
        if (out_length) *out_length = 0;
        return NULL;
    }
    if (out_length) *out_length = sm->lengths[file_id];
    return sm->sources[file_id];
}

// Convert byte offset to line/column
void nova_sourcemgr_offset_to_line_col(nova_sourcemgr_t* sm, int file_id,
                                     size_t offset, int* out_line, int* out_col) {
    if (!sm || file_id < 0 || (size_t)file_id >= sm->count) {
        *out_line = 0;
        *out_col = 0;
        return;
    }

    int* offsets = sm->line_offsets[file_id];
    int line_count = sm->line_counts[file_id];

    // Binary search for line
    int left = 0;
    int right = line_count - 1;
    int line = 0;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if ((size_t)offsets[mid] <= offset) {
            line = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    // Calculate column
    int col = (int)(offset - offsets[line]);

    *out_line = line + 1; // 1-based
    *out_col = col + 1;   // 1-based
}

// Get line content
const char* nova_sourcemgr_get_line(nova_sourcemgr_t* sm, int file_id, int line,
                                  size_t* out_length) {
    if (!sm || file_id < 0 || (size_t)file_id >= sm->count ||
        line < 1 || line > sm->line_counts[file_id]) {
        if (out_length) *out_length = 0;
        return NULL;
    }

    int* offsets = sm->line_offsets[file_id];
    int line_index = line - 1; // 0-based

    size_t start_offset = offsets[line_index];
    size_t end_offset = (line_index + 1 < sm->line_counts[file_id]) ?
                       offsets[line_index + 1] : sm->lengths[file_id];

    // Exclude trailing \n
    if (end_offset > start_offset && sm->sources[file_id][end_offset - 1] == '\n') {
        end_offset--;
    }
    if (end_offset > start_offset && sm->sources[file_id][end_offset - 1] == '\r') {
        end_offset--;
    }

    size_t line_length = end_offset - start_offset;
    if (out_length) *out_length = line_length;

    return (const char*)&sm->sources[file_id][start_offset];
}

// Get file count
size_t nova_sourcemgr_get_file_count(nova_sourcemgr_t* sm) {
    return sm ? sm->count : 0;
}
