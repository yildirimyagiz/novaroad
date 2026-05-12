/**
 * @file fs.h
 * @brief Cross-platform File System Abstraction
 */

#ifndef NOVA_FS_H
#define NOVA_FS_H

#include "../config/config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* File access modes */
typedef enum {
    NOVA_FILE_MODE_READ = 1,
    NOVA_FILE_MODE_WRITE = 2,
    NOVA_FILE_MODE_APPEND = 4,
    NOVA_FILE_MODE_BINARY = 8,
    NOVA_FILE_MODE_TEXT = 16,
} nova_file_mode_t;

/* File seek origins */
#ifndef NOVA_SEEK_ENUM_DEFINED
#define NOVA_SEEK_ENUM_DEFINED
typedef enum {
    NOVA_SEEK_SET,
    NOVA_SEEK_CUR,
    NOVA_SEEK_END,
} nova_seek_t;
#endif

/* File attributes */
typedef struct {
    bool is_directory;
    bool is_file;
    bool is_symlink;
    bool is_hidden;
    bool is_readonly;
    uint64_t size;
    uint64_t created_time;
    uint64_t modified_time;
    uint64_t accessed_time;
} nova_file_attrs_t;

/* Directory entry */
typedef struct nova_dir_entry {
    const char *name;
    nova_file_attrs_t attrs;
} nova_dir_entry_t;

/* File handle */
typedef struct nova_file_handle nova_file_handle_t;

/* Directory handle */
typedef struct nova_dir_handle nova_dir_handle_t;

/* ============================================================================
 * File Operations
 * ========================================================================== */

/**
 * Open file
 */
nova_file_handle_t *nova_fs_file_open(const char *path, nova_file_mode_t mode);

/**
 * Close file
 */
void nova_fs_file_close(nova_file_handle_t *file);

/**
 * Read from file
 */
size_t nova_fs_file_read(nova_file_handle_t *file, void *buffer, size_t size);

/**
 * Write to file
 */
size_t nova_fs_file_write(nova_file_handle_t *file, const void *buffer, size_t size);

/**
 * Seek in file
 */
int nova_fs_file_seek(nova_file_handle_t *file, int64_t offset, nova_seek_t origin);

/**
 * Get current file position
 */
int64_t nova_fs_file_tell(nova_file_handle_t *file);

/**
 * Flush file buffers
 */
int nova_fs_file_flush(nova_file_handle_t *file);

/**
 * Get file size
 */
uint64_t nova_fs_file_size(nova_file_handle_t *file);

/**
 * Check if file exists
 */
bool nova_fs_file_exists(const char *path);

/**
 * Delete file
 */
int nova_fs_file_delete(const char *path);

/**
 * Copy file
 */
int nova_fs_file_copy(const char *src_path, const char *dst_path);

/**
 * Move/rename file
 */
int nova_fs_file_move(const char *src_path, const char *dst_path);

/* ============================================================================
 * Directory Operations
 * ========================================================================== */

/**
 * Create directory
 */
int nova_fs_dir_create(const char *path);

/**
 * Remove directory
 */
int nova_fs_dir_remove(const char *path);

/**
 * Check if directory exists
 */
bool nova_fs_dir_exists(const char *path);

/**
 * Get current working directory
 */
const char *nova_fs_dir_current(void);

/**
 * Set current working directory
 */
int nova_fs_dir_set_current(const char *path);

/**
 * Open directory for iteration
 */
nova_dir_handle_t *nova_fs_dir_open(const char *path);

/**
 * Close directory
 */
void nova_fs_dir_close(nova_dir_handle_t *dir);

/**
 * Read next directory entry
 */
const nova_dir_entry_t *nova_fs_dir_read(nova_dir_handle_t *dir);

/* ============================================================================
 * Path Operations
 * ========================================================================== */

/**
 * Get absolute path
 */
char *nova_fs_path_absolute(const char *path);

/**
 * Get directory name from path
 */
const char *nova_fs_path_dirname(const char *path);

/**
 * Get filename from path
 */
const char *nova_fs_path_basename(const char *path);

/**
 * Join path components
 */
char *nova_fs_path_join(const char *path1, const char *path2);

/**
 * Get file extension
 */
const char *nova_fs_path_extension(const char *path);

/**
 * Normalize path
 */
char *nova_fs_path_normalize(const char *path);

/* ============================================================================
 * File Attributes
 * ========================================================================== */

/**
 * Get file attributes
 */
int nova_fs_get_attrs(const char *path, nova_file_attrs_t *attrs);

/**
 * Set file attributes
 */
int nova_fs_set_attrs(const char *path, const nova_file_attrs_t *attrs);

/* ============================================================================
 * Temporary Files
 * ========================================================================== */

/**
 * Create temporary file
 */
nova_file_handle_t *nova_fs_temp_file_create(void);

/**
 * Get temporary directory path
 */
const char *nova_fs_temp_dir(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_FS_H */
