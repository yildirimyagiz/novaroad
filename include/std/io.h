/**
 * @file io.h
 * @brief I/O operations (files, streams, formatting)
 *
 * Nova I/O provides:
 * - Buffered and unbuffered file I/O
 * - Stream abstraction (stdin, stdout, stderr)
 * - Formatted I/O (printf-style)
 * - Binary and text mode
 * - Async I/O support
 */

#ifndef NOVA_IO_H
#define NOVA_IO_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * File Handle
 * ======================================================================== */

typedef struct nova_file nova_file_t;

/* File open modes */
#define NOVA_FILE_READ (1 << 0)     /**< Read mode */
#define NOVA_FILE_WRITE (1 << 1)    /**< Write mode */
#define NOVA_FILE_APPEND (1 << 2)   /**< Append mode */
#define NOVA_FILE_CREATE (1 << 3)   /**< Create if not exists */
#define NOVA_FILE_TRUNCATE (1 << 4) /**< Truncate existing file */
#define NOVA_FILE_BINARY (1 << 5)   /**< Binary mode */

/* Seek modes */
#ifndef NOVA_SEEK_ENUM_DEFINED
#define NOVA_SEEK_ENUM_DEFINED
typedef enum {
  NOVA_SEEK_SET, /**< Seek from beginning */
  NOVA_SEEK_CUR, /**< Seek from current position */
  NOVA_SEEK_END  /**< Seek from end */
} nova_seek_t;
#endif

/* ========================================================================
 * File Operations
 * ======================================================================== */

/**
 * @brief Open file
 * @param path File path
 * @param mode Mode string ("r", "w", "a", "rb", "wb", etc.)
 * @return File handle, or NULL on failure
 */
nova_file_t *nova_file_open(const char *path, const char *mode);

/**
 * @brief Open file with flags
 * @param path File path
 * @param flags Combination of NOVA_FILE_* flags
 * @return File handle, or NULL on failure
 */
nova_file_t *nova_file_open_flags(const char *path, int flags);

/**
 * @brief Create temporary file
 * @return File handle, or NULL on failure
 */
nova_file_t *nova_file_tmpfile(void);

/**
 * @brief Read from file
 * @param file File handle
 * @param buffer Output buffer
 * @param size Bytes to read
 * @return Bytes actually read
 */
size_t nova_file_read(nova_file_t *file, void *buffer, size_t size);

/**
 * @brief Write to file
 * @param file File handle
 * @param buffer Data to write
 * @param size Bytes to write
 * @return Bytes actually written
 */
size_t nova_file_write(nova_file_t *file, const void *buffer, size_t size);

/**
 * @brief Read entire file into buffer
 * @param path File path
 * @param size Output: file size
 * @return Buffer (caller must free), or NULL on failure
 */
void *nova_file_read_all(const char *path, size_t *size);

/**
 * @brief Write entire buffer to file
 * @param path File path
 * @param data Data to write
 * @param size Data size
 * @return Bytes written, or -1 on failure
 */
size_t nova_file_write_all(const char *path, const void *data, size_t size);

/**
 * @brief Read line from file
 * @param file File handle
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Number of bytes read (including newline), or 0 on EOF
 */
size_t nova_file_read_line(nova_file_t *file, char *buffer, size_t size);

/**
 * @brief Seek to position
 * @param file File handle
 * @param offset Offset in bytes
 * @param whence Seek mode (NOVA_SEEK_*)
 * @return New position, or -1 on failure
 */
size_t nova_file_seek(nova_file_t *file, size_t offset, nova_seek_t whence);

/**
 * @brief Get current position
 * @param file File handle
 * @return Current position, or -1 on failure
 */
size_t nova_file_tell(nova_file_t *file);

/**
 * @brief Flush buffered data
 * @param file File handle
 * @return 0 on success, -1 on failure
 */
int nova_file_flush(nova_file_t *file);

/**
 * @brief Check if at end of file
 * @param file File handle
 * @return true if at EOF
 */
bool nova_file_eof(nova_file_t *file);

/**
 * @brief Get file size
 * @param file File handle
 * @return File size in bytes, or -1 on failure
 */
size_t nova_file_size(nova_file_t *file);

/**
 * @brief Close file
 * @param file File handle
 */
void nova_file_close(nova_file_t *file);

/* ========================================================================
 * Standard Streams
 * ======================================================================== */

/**
 * @brief Get stdin handle
 * @return stdin file handle
 */
nova_file_t *nova_stdin(void);

/**
 * @brief Get stdout handle
 * @return stdout file handle
 */
nova_file_t *nova_stdout(void);

/**
 * @brief Get stderr handle
 * @return stderr file handle
 */
nova_file_t *nova_stderr(void);

/* ========================================================================
 * Formatted I/O
 * ======================================================================== */

/**
 * @brief Print formatted string to stdout
 * @param format Printf-style format string
 * @return Number of characters printed
 */
int nova_print(const char *format, ...);

/**
 * @brief Print formatted string to stdout (va_list)
 * @param format Printf-style format string
 * @param args Variable arguments
 * @return Number of characters printed
 */
int nova_vprint(const char *format, va_list args);

/**
 * @brief Print formatted string to stderr
 * @param format Printf-style format string
 * @return Number of characters printed
 */
int nova_eprint(const char *format, ...);

/**
 * @brief Print formatted string to file
 * @param file File handle
 * @param format Printf-style format string
 * @return Number of characters printed
 */
int nova_fprint(nova_file_t *file, const char *format, ...);

/**
 * @brief Print formatted string to buffer
 * @param buffer Output buffer
 * @param size Buffer size
 * @param format Printf-style format string
 * @return Number of characters written (excluding null terminator)
 */
int nova_snprint(char *buffer, size_t size, const char *format, ...);

/**
 * @brief Print line to stdout (adds newline)
 * @param format Printf-style format string
 * @return Number of characters printed
 */
int nova_println(const char *format, ...);

/**
 * @brief Print line to stderr (adds newline)
 * @param format Printf-style format string
 * @return Number of characters printed
 */
int nova_eprintln(const char *format, ...);

/**
 * @brief Read formatted input from stdin
 * @param format Scanf-style format string
 * @return Number of items read
 */
int nova_scan(const char *format, ...);

/**
 * @brief Read formatted input from file
 * @param file File handle
 * @param format Scanf-style format string
 * @return Number of items read
 */
int nova_fscan(nova_file_t *file, const char *format, ...);

/* ========================================================================
 * Buffered I/O
 * ======================================================================== */

typedef struct nova_bufio nova_bufio_t;

/**
 * @brief Create buffered reader
 * @param file Underlying file
 * @param buffer_size Buffer size (0 for default)
 * @return Buffered reader, or NULL on failure
 */
nova_bufio_t *nova_bufio_reader(nova_file_t *file, size_t buffer_size);

/**
 * @brief Create buffered writer
 * @param file Underlying file
 * @param buffer_size Buffer size (0 for default)
 * @return Buffered writer, or NULL on failure
 */
nova_bufio_t *nova_bufio_writer(nova_file_t *file, size_t buffer_size);

/**
 * @brief Read from buffered reader
 * @param bufio Buffered reader
 * @param buffer Output buffer
 * @param size Bytes to read
 * @return Bytes actually read
 */
size_t nova_bufio_read(nova_bufio_t *bufio, void *buffer, size_t size);

/**
 * @brief Write to buffered writer
 * @param bufio Buffered writer
 * @param data Data to write
 * @param size Bytes to write
 * @return Bytes actually written
 */
size_t nova_bufio_write(nova_bufio_t *bufio, const void *data, size_t size);

/**
 * @brief Read line from buffered reader
 * @param bufio Buffered reader
 * @return Line as string (caller must free), or NULL on EOF
 */
char *nova_bufio_read_line(nova_bufio_t *bufio);

/**
 * @brief Flush buffered writer
 * @param bufio Buffered writer
 * @return 0 on success, -1 on failure
 */
int nova_bufio_flush(nova_bufio_t *bufio);

/**
 * @brief Close buffered I/O
 * @param bufio Buffered I/O handle
 */
void nova_bufio_close(nova_bufio_t *bufio);

/* ========================================================================
 * Path Operations
 * ======================================================================== */

/**
 * @brief Check if file exists
 * @param path File path
 * @return true if exists
 */
bool nova_path_exists(const char *path);

/**
 * @brief Check if path is directory
 * @param path Path
 * @return true if directory
 */
bool nova_path_is_dir(const char *path);

/**
 * @brief Check if path is regular file
 * @param path Path
 * @return true if regular file
 */
bool nova_path_is_file(const char *path);

/**
 * @brief Get file size
 * @param path File path
 * @return File size in bytes, or -1 on failure
 */
size_t nova_path_size(const char *path);

/**
 * @brief Create directory
 * @param path Directory path
 * @param recursive Create parent directories if needed
 * @return 0 on success, -1 on failure
 */
int nova_path_mkdir(const char *path, bool recursive);

/**
 * @brief Remove file or empty directory
 * @param path Path
 * @return 0 on success, -1 on failure
 */
int nova_path_remove(const char *path);

/**
 * @brief Copy file
 * @param src Source path
 * @param dst Destination path
 * @return 0 on success, -1 on failure
 */
int nova_path_copy(const char *src, const char *dst);

/**
 * @brief Move/rename file
 * @param src Source path
 * @param dst Destination path
 * @return 0 on success, -1 on failure
 */
int nova_path_move(const char *src, const char *dst);

/**
 * @brief Get absolute path
 * @param path Relative or absolute path
 * @return Absolute path (caller must free), or NULL on failure
 */
char *nova_path_absolute(const char *path);

/**
 * @brief Join path components
 * @param base Base path
 * @param ... Additional path components (NULL-terminated)
 * @return Joined path (caller must free), or NULL on failure
 */
char *nova_path_join(const char *base, ...);

/* ========================================================================
 * Directory Iteration
 * ======================================================================== */

typedef struct nova_dir nova_dir_t;
typedef struct nova_dir_entry nova_dir_entry_t;

/**
 * @brief Open directory
 * @param path Directory path
 * @return Directory handle, or NULL on failure
 */
nova_dir_t *nova_dir_open(const char *path);

/**
 * @brief Read next directory entry
 * @param dir Directory handle
 * @return Directory entry, or NULL at end
 */
const nova_dir_entry_t *nova_dir_read(nova_dir_t *dir);

/**
 * @brief Get entry name
 * @param entry Directory entry
 * @return Entry name
 */
const char *nova_dir_entry_name(const nova_dir_entry_t *entry);

/**
 * @brief Check if entry is directory
 * @param entry Directory entry
 * @return true if directory
 */
bool nova_dir_entry_is_dir(const nova_dir_entry_t *entry);

/**
 * @brief Close directory
 * @param dir Directory handle
 */
void nova_dir_close(nova_dir_t *dir);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_IO_H */
