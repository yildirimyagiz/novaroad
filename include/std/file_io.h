// File I/O Header
// Real file operations for Nova compiler

#ifndef NOVA_FILE_IO_H
#define NOVA_FILE_IO_H

#include <stdbool.h>
#include <stddef.h>

// C API for file operations
bool nova_file_exists(const char* path);
unsigned char* nova_file_read_all(const char* path, size_t* out_len); // malloc
bool nova_file_write_all(const char* path, const unsigned char* data, size_t len);
void nova_file_free(void* p);

#endif // NOVA_FILE_IO_H
