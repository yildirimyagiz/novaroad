/*
 * Nova Runtime I/O Functions - Header
 * FFI bindings for stdlib/core/io.zn
 */

#ifndef NOVA_RUNTIME_IO_H
#define NOVA_RUNTIME_IO_H

#include <stdint.h>

// Console output
void nova_runtime_println(const char* ptr, int64_t len);
void nova_runtime_print(const char* ptr, int64_t len);
void nova_runtime_print_int(int64_t value);
void nova_runtime_print_float(double value);
void nova_runtime_eprint(const char* ptr, int64_t len);
void nova_runtime_eprintln(const char* ptr, int64_t len);

// Console input
char* nova_runtime_read_line(void);

// File operations
int64_t nova_runtime_file_open(const char* path, int64_t len);
int64_t nova_runtime_file_create(const char* path, int64_t len);
char* nova_runtime_file_read(int64_t fd, int64_t size);
char* nova_runtime_file_read_all(int64_t fd);
int64_t nova_runtime_file_write(int64_t fd, const char* ptr, int64_t len);
void nova_runtime_file_close(int64_t fd);

// Formatting & parsing
char* nova_runtime_format_int(int64_t value);
char* nova_runtime_format_float(double value);
int64_t nova_runtime_parse_int(const char* ptr, int64_t len);
double nova_runtime_parse_float(const char* ptr, int64_t len);

#endif // NOVA_RUNTIME_IO_H
