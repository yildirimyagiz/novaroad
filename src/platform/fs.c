/**
 * @file fs.c
 * @brief Cross-platform File System implementation
 */

#include "platform/fs.h"
#include "std/alloc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(NOVA_PLATFORM_POSIX)
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#elif defined(NOVA_PLATFORM_WINDOWS)
#include <windows.h>
#include <direct.h>
#include <io.h>
#endif

/* Internal structures */
struct nova_file_handle {
#if defined(NOVA_PLATFORM_POSIX)
    int fd;
#elif defined(NOVA_PLATFORM_WINDOWS)
    HANDLE handle;
#endif
    nova_file_mode_t mode;
    char *path;
};

struct nova_dir_handle {
#if defined(NOVA_PLATFORM_POSIX)
    DIR *dir;
    char *path;
#elif defined(NOVA_PLATFORM_WINDOWS)
    HANDLE handle;
    WIN32_FIND_DATA find_data;
    char *path;
    bool first_read;
#endif
};

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================== */

static int convert_mode_to_flags(nova_file_mode_t mode) {
    int flags = 0;

#if defined(NOVA_PLATFORM_POSIX)
    if (mode & NOVA_FILE_MODE_READ) {
        flags |= O_RDONLY;
    }
    if (mode & NOVA_FILE_MODE_WRITE) {
        flags |= O_WRONLY | O_CREAT;
        if (mode & NOVA_FILE_MODE_APPEND) {
            flags |= O_APPEND;
        } else {
            flags |= O_TRUNC;
        }
    }
    if (mode & NOVA_FILE_MODE_BINARY) {
        // POSIX doesn't have separate binary mode
    }
#endif

    return flags;
}

static mode_t convert_mode_to_perms(nova_file_mode_t mode) {
    (void)mode; // Unused for now
    return S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644
}

/* ============================================================================
 * File Operations
 * ========================================================================== */

nova_file_handle_t *nova_fs_file_open(const char *path, nova_file_mode_t mode) {
    if (!path) return NULL;

    nova_file_handle_t *file = nova_alloc(sizeof(nova_file_handle_t));
    if (!file) return NULL;

    file->mode = mode;
    file->path = nova_alloc(strlen(path) + 1);
    if (!file->path) {
        nova_free(file);
        return NULL;
    }
    strcpy(file->path, path);

#if defined(NOVA_PLATFORM_POSIX)
    int flags = convert_mode_to_flags(mode);
    mode_t perms = convert_mode_to_perms(mode);
    file->fd = open(path, flags, perms);
    if (file->fd == -1) {
        nova_free(file->path);
        nova_free(file);
        return NULL;
    }
#elif defined(NOVA_PLATFORM_WINDOWS)
    DWORD access = 0;
    DWORD creation = OPEN_EXISTING;

    if (mode & NOVA_FILE_MODE_READ) access |= GENERIC_READ;
    if (mode & NOVA_FILE_MODE_WRITE) {
        access |= GENERIC_WRITE;
        if (mode & NOVA_FILE_MODE_APPEND) {
            creation = OPEN_ALWAYS;
        } else {
            creation = CREATE_ALWAYS;
        }
    }

    file->handle = CreateFileA(path, access, 0, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file->handle == INVALID_HANDLE_VALUE) {
        nova_free(file->path);
        nova_free(file);
        return NULL;
    }
#endif

    return file;
}

void nova_fs_file_close(nova_file_handle_t *file) {
    if (!file) return;

#if defined(NOVA_PLATFORM_POSIX)
    if (file->fd != -1) {
        close(file->fd);
    }
#elif defined(NOVA_PLATFORM_WINDOWS)
    if (file->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(file->handle);
    }
#endif

    if (file->path) nova_free(file->path);
    nova_free(file);
}

size_t nova_fs_file_read(nova_file_handle_t *file, void *buffer, size_t size) {
    if (!file || !buffer || !(file->mode & NOVA_FILE_MODE_READ)) return 0;

#if defined(NOVA_PLATFORM_POSIX)
    ssize_t result = read(file->fd, buffer, size);
    return result >= 0 ? (size_t)result : 0;
#elif defined(NOVA_PLATFORM_WINDOWS)
    DWORD bytes_read;
    if (ReadFile(file->handle, buffer, (DWORD)size, &bytes_read, NULL)) {
        return bytes_read;
    }
    return 0;
#endif
}

size_t nova_fs_file_write(nova_file_handle_t *file, const void *buffer, size_t size) {
    if (!file || !buffer || !(file->mode & NOVA_FILE_MODE_WRITE)) return 0;

#if defined(NOVA_PLATFORM_POSIX)
    ssize_t result = write(file->fd, buffer, size);
    return result >= 0 ? (size_t)result : 0;
#elif defined(NOVA_PLATFORM_WINDOWS)
    DWORD bytes_written;
    if (WriteFile(file->handle, buffer, (DWORD)size, &bytes_written, NULL)) {
        return bytes_written;
    }
    return 0;
#endif
}

int nova_fs_file_seek(nova_file_handle_t *file, int64_t offset, nova_seek_origin_t origin) {
    if (!file) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    int whence;
    switch (origin) {
        case NOVA_SEEK_SET: whence = SEEK_SET; break;
        case NOVA_SEEK_CUR: whence = SEEK_CUR; break;
        case NOVA_SEEK_END: whence = SEEK_END; break;
        default: return -1;
    }
    return lseek(file->fd, offset, whence) >= 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    DWORD move_method;
    switch (origin) {
        case NOVA_SEEK_SET: move_method = FILE_BEGIN; break;
        case NOVA_SEEK_CUR: move_method = FILE_CURRENT; break;
        case NOVA_SEEK_END: move_method = FILE_END; break;
        default: return -1;
    }
    LARGE_INTEGER li;
    li.QuadPart = offset;
    return SetFilePointerEx(file->handle, li, NULL, move_method) ? 0 : -1;
#endif
}

int64_t nova_fs_file_tell(nova_file_handle_t *file) {
    if (!file) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return lseek(file->fd, 0, SEEK_CUR);
#elif defined(NOVA_PLATFORM_WINDOWS)
    LARGE_INTEGER li = {0};
    LARGE_INTEGER result;
    if (SetFilePointerEx(file->handle, li, &result, FILE_CURRENT)) {
        return result.QuadPart;
    }
    return -1;
#endif
}

int nova_fs_file_flush(nova_file_handle_t *file) {
    if (!file) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return fsync(file->fd) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return FlushFileBuffers(file->handle) ? 0 : -1;
#endif
}

uint64_t nova_fs_file_size(nova_file_handle_t *file) {
    if (!file) return 0;

#if defined(NOVA_PLATFORM_POSIX)
    struct stat st;
    if (fstat(file->fd, &st) == 0) {
        return st.st_size;
    }
    return 0;
#elif defined(NOVA_PLATFORM_WINDOWS)
    LARGE_INTEGER size;
    if (GetFileSizeEx(file->handle, &size)) {
        return size.QuadPart;
    }
    return 0;
#endif
}

bool nova_fs_file_exists(const char *path) {
    if (!path) return false;

#if defined(NOVA_PLATFORM_POSIX)
    return access(path, F_OK) == 0;
#elif defined(NOVA_PLATFORM_WINDOWS)
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#endif
}

int nova_fs_file_delete(const char *path) {
    if (!path) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return unlink(path) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return DeleteFileA(path) ? 0 : -1;
#endif
}

int nova_fs_file_copy(const char *src_path, const char *dst_path) {
    if (!src_path || !dst_path) return -1;

    nova_file_handle_t *src = nova_fs_file_open(src_path, NOVA_FILE_MODE_READ);
    if (!src) return -1;

    nova_file_handle_t *dst = nova_fs_file_open(dst_path, NOVA_FILE_MODE_WRITE);
    if (!dst) {
        nova_fs_file_close(src);
        return -1;
    }

    char buffer[8192];
    size_t bytes_read;
    int result = 0;

    while ((bytes_read = nova_fs_file_read(src, buffer, sizeof(buffer))) > 0) {
        if (nova_fs_file_write(dst, buffer, bytes_read) != bytes_read) {
            result = -1;
            break;
        }
    }

    nova_fs_file_close(src);
    nova_fs_file_close(dst);

    return result;
}

int nova_fs_file_move(const char *src_path, const char *dst_path) {
    if (!src_path || !dst_path) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return rename(src_path, dst_path) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return MoveFileA(src_path, dst_path) ? 0 : -1;
#endif
}

/* ============================================================================
 * Directory Operations
 * ========================================================================== */

int nova_fs_dir_create(const char *path) {
    if (!path) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return mkdir(path, 0755) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return _mkdir(path) == 0 ? 0 : -1;
#endif
}

int nova_fs_dir_remove(const char *path) {
    if (!path) return -1;

#if defined(NOVA_PLATFORM_POSIX)
    return rmdir(path) == 0 ? 0 : -1;
#elif defined(NOVA_PLATFORM_WINDOWS)
    return _rmdir(path) == 0 ? 0 : -1;
#endif
}

bool nova_fs_dir_exists(const char *path) {
    if (!path) return false;

    // Simple implementation - check if we can open it
    nova_dir_handle_t *dir = nova_fs_dir_open(path);
    if (dir) {
        nova_fs_dir_close(dir);
        return true;
    }
    return false;
}

nova_dir_handle_t *nova_fs_dir_open(const char *path) {
    if (!path) return NULL;

    nova_dir_handle_t *dir = nova_alloc(sizeof(nova_dir_handle_t));
    if (!dir) return NULL;

    dir->path = nova_alloc(strlen(path) + 1);
    if (!dir->path) {
        nova_free(dir);
        return NULL;
    }
    strcpy(dir->path, path);

#if defined(NOVA_PLATFORM_POSIX)
    dir->dir = opendir(path);
    if (!dir->dir) {
        nova_free(dir->path);
        nova_free(dir);
        return NULL;
    }
#elif defined(NOVA_PLATFORM_WINDOWS)
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    dir->handle = FindFirstFileA(search_path, &dir->find_data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        nova_free(dir->path);
        nova_free(dir);
        return NULL;
    }
    dir->first_read = true;
#endif

    return dir;
}

void nova_fs_dir_close(nova_dir_handle_t *dir) {
    if (!dir) return;

#if defined(NOVA_PLATFORM_POSIX)
    if (dir->dir) closedir(dir->dir);
#elif defined(NOVA_PLATFORM_WINDOWS)
    if (dir->handle != INVALID_HANDLE_VALUE) FindClose(dir->handle);
#endif

    if (dir->path) nova_free(dir->path);
    nova_free(dir);
}

const nova_dir_entry_t *nova_fs_dir_read(nova_dir_handle_t *dir) {
    if (!dir) return NULL;

    static nova_dir_entry_t entry;

#if defined(NOVA_PLATFORM_POSIX)
    struct dirent *dent = readdir(dir->dir);
    if (!dent) return NULL;

    entry.name = dent->d_name;
    // Basic attribute detection - can be expanded
    entry.attrs.is_directory = (dent->d_type == DT_DIR);
    entry.attrs.is_file = (dent->d_type == DT_REG);
    entry.attrs.size = 0; // Would need stat() for full info

    return &entry;

#elif defined(NOVA_PLATFORM_WINDOWS)
    if (dir->first_read) {
        dir->first_read = false;
    } else {
        if (!FindNextFileA(dir->handle, &dir->find_data)) {
            return NULL;
        }
    }

    entry.name = dir->find_data.cFileName;
    entry.attrs.is_directory = (dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    entry.attrs.is_file = !entry.attrs.is_directory;
    entry.attrs.size = ((uint64_t)dir->find_data.nFileSizeHigh << 32) | dir->find_data.nFileSizeLow;

    return &entry;
#endif

    return NULL;
}

/* ============================================================================
 * Path Operations (Simplified)
 * ========================================================================== */

char *nova_fs_path_join(const char *path1, const char *path2) {
    if (!path1 || !path2) return NULL;

    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);
    char *result = nova_alloc(len1 + len2 + 2); // +2 for separator and null

    if (!result) return NULL;

    strcpy(result, path1);

    // Add separator if needed
#if defined(NOVA_PLATFORM_WINDOWS)
    if (len1 > 0 && result[len1-1] != '\\') {
        strcat(result, "\\");
    }
#else
    if (len1 > 0 && result[len1-1] != '/') {
        strcat(result, "/");
    }
#endif

    strcat(result, path2);
    return result;
}

const char *nova_fs_path_extension(const char *path) {
    if (!path) return NULL;

    const char *dot = strrchr(path, '.');
    return dot ? dot + 1 : "";
}

/* ============================================================================
 * Temporary Files (Basic Implementation)
 * ========================================================================== */

nova_file_handle_t *nova_fs_temp_file_create(void) {
    // Simple implementation - creates file in current directory
    static int counter = 0;
    char temp_path[256];

    snprintf(temp_path, sizeof(temp_path), "nova_temp_%d.tmp", counter++);
    return nova_fs_file_open(temp_path, NOVA_FILE_MODE_WRITE | NOVA_FILE_MODE_BINARY);
}

const char *nova_fs_temp_dir(void) {
#if defined(NOVA_PLATFORM_POSIX)
    return "/tmp";
#elif defined(NOVA_PLATFORM_WINDOWS)
    return getenv("TEMP");
#else
    return ".";
#endif
}
