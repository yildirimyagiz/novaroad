/**
 * Nova File I/O Runtime
 * Native file operations for model saving/loading
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include "nova_file_io.h"

// File handle management
typedef struct {
    FILE* fp;
    char* path;
    int flags;
} NovaFile;

#define MAX_OPEN_FILES 256
static NovaFile open_files[MAX_OPEN_FILES];
static int next_handle = 1;

/**
 * Create/open a file for writing
 * Returns: file handle (>0) or 0 on error
 */
int64_t nova_file_create(const char* path) {
    if (next_handle >= MAX_OPEN_FILES) return 0;
    
    FILE* fp = fopen(path, "wb");
    if (!fp) return 0;
    
    int handle = next_handle++;
    open_files[handle].fp = fp;
    open_files[handle].path = strdup(path);
    open_files[handle].flags = 1; // Write mode
    
    return handle;
}

/**
 * Open file for reading
 */
int64_t nova_file_open_read(const char* path) {
    if (next_handle >= MAX_OPEN_FILES) return 0;
    
    FILE* fp = fopen(path, "rb");
    if (!fp) return 0;
    
    int handle = next_handle++;
    open_files[handle].fp = fp;
    open_files[handle].path = strdup(path);
    open_files[handle].flags = 0; // Read mode
    
    return handle;
}

/**
 * Write bytes to file
 * Returns: bytes written
 */
int64_t nova_file_write(int64_t handle, const void* data, int64_t size) {
    if (handle <= 0 || handle >= MAX_OPEN_FILES) return 0;
    if (!open_files[handle].fp) return 0;
    
    size_t written = fwrite(data, 1, size, open_files[handle].fp);
    return (int64_t)written;
}

/**
 * Read bytes from file
 * Returns: bytes read
 */
int64_t nova_file_read(int64_t handle, void* buffer, int64_t size) {
    if (handle <= 0 || handle >= MAX_OPEN_FILES) return 0;
    if (!open_files[handle].fp) return 0;
    
    size_t read = fread(buffer, 1, size, open_files[handle].fp);
    return (int64_t)read;
}

/**
 * Close file
 * Returns: 0 on success
 */
int64_t nova_file_close(int64_t handle) {
    if (handle <= 0 || handle >= MAX_OPEN_FILES) return -1;
    if (!open_files[handle].fp) return -1;
    
    fclose(open_files[handle].fp);
    free(open_files[handle].path);
    open_files[handle].fp = NULL;
    open_files[handle].path = NULL;
    
    return 0;
}

/**
 * Get file size
 * Returns: size in bytes
 */
int64_t nova_file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (int64_t)st.st_size;
}

/**
 * Check if file exists
 * Returns: 1 if exists, 0 otherwise
 */
int64_t nova_file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0) ? 1 : 0;
}

/**
 * Write float array to file (for tensors)
 */
int64_t nova_write_float_array(int64_t handle, const double* data, int64_t count) {
    if (handle <= 0 || handle >= MAX_OPEN_FILES) return 0;
    if (!open_files[handle].fp) return 0;
    
    // Convert to float32 for efficiency
    float* f32_data = (float*)malloc(count * sizeof(float));
    for (int64_t i = 0; i < count; i++) {
        f32_data[i] = (float)data[i];
    }
    
    size_t written = fwrite(f32_data, sizeof(float), count, open_files[handle].fp);
    free(f32_data);
    
    return (int64_t)written;
}

/**
 * Read float array from file (for tensors)
 */
int64_t nova_read_float_array(int64_t handle, double* buffer, int64_t count) {
    if (handle <= 0 || handle >= MAX_OPEN_FILES) return 0;
    if (!open_files[handle].fp) return 0;
    
    // Read as float32
    float* f32_data = (float*)malloc(count * sizeof(float));
    size_t read = fread(f32_data, sizeof(float), count, open_files[handle].fp);
    
    // Convert to double
    for (int64_t i = 0; i < read; i++) {
        buffer[i] = (double)f32_data[i];
    }
    
    free(f32_data);
    return (int64_t)read;
}

/**
 * Write SafeTensors header
 * Format: 8 bytes length + JSON metadata
 */
int64_t nova_write_safetensors_header(int64_t handle, const char* metadata) {
    if (handle <= 0 || handle >= MAX_OPEN_FILES) return 0;
    if (!open_files[handle].fp) return 0;
    
    // Write metadata length (8 bytes)
    int64_t meta_len = strlen(metadata);
    fwrite(&meta_len, sizeof(int64_t), 1, open_files[handle].fp);
    
    // Write metadata JSON
    fwrite(metadata, 1, meta_len, open_files[handle].fp);
    
    return 8 + meta_len;
}
