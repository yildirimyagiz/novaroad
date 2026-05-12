// File I/O C Implementation
// Real file operations for Nova compiler

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

bool nova_file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

unsigned char* nova_file_read_all(const char* path, size_t* out_len) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        *out_len = 0;
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0) {
        fclose(file);
        *out_len = 0;
        return NULL;
    }

    // Allocate buffer
    unsigned char* buffer = (unsigned char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        *out_len = 0;
        return NULL;
    }

    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        free(buffer);
        *out_len = 0;
        return NULL;
    }

    // Null terminate for safety
    buffer[file_size] = '\0';
    *out_len = file_size;
    return buffer;
}

bool nova_file_write_all(const char* path, const unsigned char* data, size_t len) {
    FILE* file = fopen(path, "wb");
    if (!file) {
        return false;
    }

    size_t bytes_written = fwrite(data, 1, len, file);
    fclose(file);

    return bytes_written == len;
}

void nova_file_free(void* p) {
    free(p);
}
