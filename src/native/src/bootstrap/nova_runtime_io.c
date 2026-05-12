/*
 * Novae I/O Functions
 * FFI bindings for stdlib/core/io.zn
 *
 * These functions are called from Nova ia @extern declarations
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================
// CONSOLE OUTPUT
// ============================================

void nova_runtime_println(const char *ptr, int64_t len) {
  if (ptr && len > 0) {
    fwrite(ptr, 1, len, stdout);
  }
  printf("\n");
  fflush(stdout);
}

void nova_runtime_print(const char *ptr, int64_t len) {
  if (ptr && len > 0) {
    fwrite(ptr, 1, len, stdout);
  }
  fflush(stdout);
}

void nova_runtime_print_int(int64_t value) {
  printf("%lld", (long long)value);
  fflush(stdout);
}

void nova_runtime_print_float(double value) {
  printf("%g", value);
  fflush(stdout);
}

void nova_runtime_eprint(const char *ptr, int64_t len) {
  if (ptr && len > 0) {
    fwrite(ptr, 1, len, stderr);
  }
  fflush(stderr);
}

void nova_runtime_eprintln(const char *ptr, int64_t len) {
  if (ptr && len > 0) {
    fwrite(ptr, 1, len, stderr);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

// ============================================
// CONSOLE INPUT
// ============================================

char *nova_runtime_read_line(void) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read = getline(&line, &len, stdin);

  if (read == -1) {
    free(line);
    return NULL;
  }

  // Remove trailing newline
  if (read > 0 && line[read - 1] == '\n') {
    line[read - 1] = '\0';
  }

  return line;
}

// ============================================
// FILE OPERATIONS
// ============================================

int64_t nova_runtime_file_open(const char *path, int64_t len) {
  char *null_terminated = strndup(path, len);
  if (!null_terminated)
    return -1;

  FILE *fp = fopen(null_terminated, "r");
  free(null_terminated);

  if (!fp)
    return -1;
  return (int64_t)fp;
}

int64_t nova_runtime_file_create(const char *path, int64_t len) {
  char *null_terminated = strndup(path, len);
  if (!null_terminated)
    return -1;

  FILE *fp = fopen(null_terminated, "w");
  free(null_terminated);

  if (!fp)
    return -1;
  return (int64_t)fp;
}

char *nova_runtime_file_read(int64_t fd, int64_t size) {
  FILE *fp = (FILE *)fd;
  if (!fp)
    return NULL;

  char *buffer = malloc(size + 1);
  if (!buffer)
    return NULL;

  size_t read = fread(buffer, 1, size, fp);
  buffer[read] = '\0';

  return buffer;
}

char *nova_runtime_file_read_all(int64_t fd) {
  FILE *fp = (FILE *)fd;
  if (!fp)
    return NULL;

  // Get file size
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *buffer = malloc(size + 1);
  if (!buffer)
    return NULL;

  size_t read = fread(buffer, 1, size, fp);
  buffer[read] = '\0';

  return buffer;
}

int64_t nova_runtime_file_write(int64_t fd, const char *ptr, int64_t len) {
  FILE *fp = (FILE *)fd;
  if (!fp)
    return -1;

  size_t written = fwrite(ptr, 1, len, fp);
  fflush(fp);

  return (int64_t)written;
}

void nova_runtime_file_close(int64_t fd) {
  FILE *fp = (FILE *)fd;
  if (fp) {
    fclose(fp);
  }
}

// ============================================
// FORMATTING & PARSING
// ============================================

char *nova_runtime_format_int(int64_t value) {
  char *buffer = malloc(32);
  if (!buffer)
    return NULL;

  snprintf(buffer, 32, "%lld", (long long)value);
  return buffer;
}

char *nova_runtime_format_float(double value) {
  char *buffer = malloc(64);
  if (!buffer)
    return NULL;

  snprintf(buffer, 64, "%g", value);
  return buffer;
}

int64_t nova_runtime_parse_int(const char *ptr, int64_t len) {
  char *null_terminated = strndup(ptr, len);
  if (!null_terminated)
    return 0;

  int64_t value = atoll(null_terminated);
  free(null_terminated);

  return value;
}

double nova_runtime_parse_float(const char *ptr, int64_t len) {
  char *null_terminated = strndup(ptr, len);
  if (!null_terminated)
    return 0.0;

  double value = atof(null_terminated);
  free(null_terminated);

  return value;
}
