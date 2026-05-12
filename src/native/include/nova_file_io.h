/**
 * Nova File I/O Runtime - Header
 */

#ifndef NOVA_FILE_IO_H
#define NOVA_FILE_IO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// File operations
int64_t nova_file_create(const char* path);
int64_t nova_file_open_read(const char* path);
int64_t nova_file_write(int64_t handle, const void* data, int64_t size);
int64_t nova_file_read(int64_t handle, void* buffer, int64_t size);
int64_t nova_file_close(int64_t handle);
int64_t nova_file_size(const char* path);
int64_t nova_file_exists(const char* path);

// Tensor I/O
int64_t nova_write_float_array(int64_t handle, const double* data, int64_t count);
int64_t nova_read_float_array(int64_t handle, double* buffer, int64_t count);

// SafeTensors format
int64_t nova_write_safetensors_header(int64_t handle, const char* metadata);

#ifdef __cplusplus
}
#endif

#endif // NOVA_FILE_IO_H
