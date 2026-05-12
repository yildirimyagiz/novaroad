/**
 * @file file_io.h
 * @brief Nova Standard Library - File I/O Operations
 * 
 * Cross-platform file operations with error handling and buffering.
 */

#ifndef NOVA_STD_FILE_IO_H
#define NOVA_STD_FILE_IO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * File Operations
 * ======================================================================== */

/**
 * Check if a file exists
 * @param path File path
 * @return true if file exists
 */
bool nova_file_exists(const char* path);

/**
 * Read entire file into memory
 * @param path File path
 * @param out_len Output: number of bytes read
 * @return Allocated buffer (caller must free) or NULL on error
 */
unsigned char* nova_file_read_all(const char* path, size_t* out_len);

/**
 * Write data to file (overwrites existing)
 * @param path File path
 * @param data Data to write
 * @param len Number of bytes to write
 * @return true on success
 */
bool nova_file_write_all(const char* path, const unsigned char* data, size_t len);

/**
 * Append data to file
 * @param path File path
 * @param data Data to append
 * @param len Number of bytes to append
 * @return true on success
 */
bool nova_file_append_all(const char* path, const unsigned char* data, size_t len);

/**
 * Free memory allocated by file operations
 * @param ptr Pointer returned by nova_file_read_all()
 */
void nova_file_free(void* ptr);

/**
 * Get file size in bytes
 * @param path File path
 * @return File size or -1 on error
 */
int64_t nova_file_size(const char* path);

/**
 * Delete a file
 * @param path File path
 * @return true on success
 */
bool nova_file_delete(const char* path);

/**
 * Copy a file
 * @param src Source file path
 * @param dest Destination file path
 * @return true on success
 */
bool nova_file_copy(const char* src, const char* dest);

/**
 * Move/rename a file
 * @param src Source file path
 * @param dest Destination file path
 * @return true on success
 */
bool nova_file_move(const char* src, const char* dest);

/* ========================================================================
 * Directory Operations
 * ======================================================================== */

/**
 * Check if a directory exists
 * @param path Directory path
 * @return true if directory exists
 */
bool nova_dir_exists(const char* path);

/**
 * Create a directory
 * @param path Directory path
 * @param recursive If true, create parent directories
 * @return true on success
 */
bool nova_dir_create(const char* path, bool recursive);

/**
 * Remove a directory
 * @param path Directory path
 * @param recursive If true, remove contents recursively
 * @return true on success
 */
bool nova_dir_remove(const char* path, bool recursive);

/* ========================================================================
 * Path Operations
 * ======================================================================== */

/**
 * Join path components
 * @param base Base path
 * @param component Component to append
 * @return Allocated string (caller must free) or NULL on error
 */
char* nova_path_join(const char* base, const char* component);

/**
 * Get file name from path
 * @param path Full path
 * @return Allocated string (caller must free) or NULL on error
 */
char* nova_path_basename(const char* path);

/**
 * Get directory from path
 * @param path Full path
 * @return Allocated string (caller must free) or NULL on error
 */
char* nova_path_dirname(const char* path);

/**
 * Get file extension
 * @param path File path
 * @return Allocated string (caller must free, includes dot) or NULL
 */
char* nova_path_extension(const char* path);

/**
 * Normalize path (resolve .., ., remove duplicate slashes)
 * @param path Path to normalize
 * @return Allocated string (caller must free) or NULL on error
 */
char* nova_path_normalize(const char* path);

/**
 * Check if path is absolute
 * @param path Path to check
 * @return true if absolute
 */
bool nova_path_is_absolute(const char* path);

/* ========================================================================
 * Buffered I/O (Advanced)
 * ======================================================================== */

typedef struct nova_file_handle nova_file_handle_t;

typedef enum {
    NOVA_FILE_READ,
    NOVA_FILE_WRITE,
    NOVA_FILE_APPEND,
    NOVA_FILE_READ_WRITE,
} nova_file_mode_t;

/**
 * Open a file with buffering
 * @param path File path
 * @param mode File mode
 * @return File handle or NULL on error
 */
nova_file_handle_t* nova_file_open(const char* path, nova_file_mode_t mode);

/**
 * Close a file handle
 * @param handle File handle
 */
void nova_file_close(nova_file_handle_t* handle);

/**
 * Read from file
 * @param handle File handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes actually read
 */
size_t nova_file_read(nova_file_handle_t* handle, void* buffer, size_t size);

/**
 * Write to file
 * @param handle File handle
 * @param data Data to write
 * @param size Number of bytes to write
 * @return Number of bytes actually written
 */
size_t nova_file_write(nova_file_handle_t* handle, const void* data, size_t size);

/**
 * Seek to position in file
 * @param handle File handle
 * @param offset Offset from whence
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * @return New position or -1 on error
 */
int64_t nova_file_seek(nova_file_handle_t* handle, int64_t offset, int whence);

/**
 * Get current file position
 * @param handle File handle
 * @return Current position or -1 on error
 */
int64_t nova_file_tell(nova_file_handle_t* handle);

/**
 * Flush buffered data to disk
 * @param handle File handle
 * @return true on success
 */
bool nova_file_flush(nova_file_handle_t* handle);

/**
 * Check if end of file reached
 * @param handle File handle
 * @return true if EOF
 */
bool nova_file_eof(nova_file_handle_t* handle);

/* ========================================================================
 * Error Handling
 * ======================================================================== */

/**
 * Get last file I/O error message
 * @return Error string or NULL if no error
 */
const char* nova_file_error(void);

/**
 * Clear last error
 */
void nova_file_clear_error(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_STD_FILE_IO_H */
