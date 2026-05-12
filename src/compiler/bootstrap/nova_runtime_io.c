// Nova Runtime I/O - Minimal Bootstrap Stub
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Nova Result Type
typedef struct {
  int ok;
  void *value;
  char *err;
} ln_result_t;

// File I/O stubs
void *nova_file_open(const char *path, const char *mode) {
  return fopen(path, mode);
}

void nova_file_close(void *fp) {
  if (fp)
    fclose((FILE *)fp);
}

char *nova_file_read_line(void *fp) {
  if (!fp)
    return NULL;

  char *line = NULL;
  size_t len = 0;
  ssize_t read = getline(&line, &len, (FILE *)fp);

  if (read == -1) {
    free(line);
    return NULL;
  }

  return line;
}

long nova_file_read(void *fp, char *buffer, long size) {
  if (!fp || !buffer)
    return -1;
  return (long)fread(buffer, 1, (size_t)size, (FILE *)fp);
}

long nova_file_read_fixed(void *fp, char *buffer, long size) {
  return nova_file_read(fp, buffer, size);
}

long nova_file_write(void *fp, const char *data, long size) {
  if (!fp || !data)
    return -1;
  return (long)fwrite(data, 1, (size_t)size, (FILE *)fp);
}

// Console I/O stubs
void nova_print(const char *str) {
  if (str)
    printf("%s", str);
}

void nova_println(const char *str) {
  if (str)
    printf("%s\n", str);
}

char *nova_read_line(void) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read = getline(&line, &len, stdin);

  if (read == -1) {
    free(line);
    return NULL;
  }

  return line;
}
// Aliases for Nova compiler (without nova_ prefix)
void println(const char *str) { nova_println(str); }

void print(const char *str) { nova_print(str); }

// File content read/write
char *fs__read_to_string(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);
  string[fsize] = 0;
  return string;
}

int fs__write(const char *path, const char *content) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return 0;
  fputs(content, f);
  fclose(f);
  return 1;
}

// Bootstrap stub: ignores the error-mapping callback, passes result through
// as-is. Stage 1 should implement proper callback invocation.
char *fs__read_to_string__map_err(char *res, void *cb) { return res; }

ln_result_t Err(const char *msg) {
  ln_result_t res = {0, NULL, (char *)msg};
  return res;
}

ln_result_t Ok(void *val) {
  ln_result_t res = {1, val, NULL};
  return res;
}

// Result mapping helper for bootstrap
ln_result_t map_err(ln_result_t res, void *cb) { return res; }

// ── Stage 0 Uyumluluk Ekleri ─────────────────────────────────────────────

// Stage 0 compiler maps Nova's print() -> print_ln() (no newline variant)
void print_ln(const char *str) { nova_print(str); }

// Stage 0 header'ı bu imzaları extern olarak bekliyor
// nova_runtime_println(ptr, len) — ptr+len bazlı println
void nova_runtime_println(const char *ptr, int64_t len) {
  if (!ptr) {
    puts("");
    return;
  }
  fwrite(ptr, 1, (size_t)len, stdout);
  putchar('\n');
}

void nova_runtime_print(const char *ptr, int64_t len) {
  if (!ptr)
    return;
  fwrite(ptr, 1, (size_t)len, stdout);
}

void nova_runtime_print_int(int64_t value) { printf("%lld", (long long)value); }

void nova_runtime_print_float(double value) { printf("%g", value); }

char *nova_runtime_read_line(void) { return nova_read_line(); }