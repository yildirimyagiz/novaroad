/**
 * @file string.h
 * @brief UTF-8 string type and string manipulation
 * 
 * Nova strings are:
 * - UTF-8 encoded
 * - Immutable by default (use mutable variants for modifications)
 * - Reference counted (copy-on-write)
 * - NUL-terminated for C interop
 */

#ifndef NOVA_STRING_H
#define NOVA_STRING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * String Type
 * ======================================================================== */

typedef struct nova_string nova_string_t;

/* String view (non-owning slice) */
typedef struct {
    const char *data;
    size_t len;
} nova_string_view_t;

/* ========================================================================
 * String Creation
 * ======================================================================== */

/**
 * @brief Create string from C string
 * @param cstr NUL-terminated C string (copied)
 * @return New string, or NULL on failure
 */
nova_string_t *nova_string_new(const char *cstr);

/**
 * @brief Create string from buffer
 * @param data Buffer (may contain NUL bytes)
 * @param len Length in bytes
 * @return New string, or NULL on failure
 */
nova_string_t *nova_string_from_bytes(const char *data, size_t len);

/**
 * @brief Create string with reserved capacity
 * @param capacity Bytes to reserve
 * @return Empty string with capacity, or NULL on failure
 */
nova_string_t *nova_string_with_capacity(size_t capacity);

/**
 * @brief Create string from format
 * @param format Printf-style format string
 * @return Formatted string, or NULL on failure
 */
nova_string_t *nova_string_format(const char *format, ...);

/**
 * @brief Create string from format (va_list version)
 * @param format Printf-style format string
 * @param args Variable arguments
 * @return Formatted string, or NULL on failure
 */
nova_string_t *nova_string_vformat(const char *format, va_list args);

/**
 * @brief Duplicate string
 * @param str Source string
 * @return New string copy, or NULL on failure
 */
nova_string_t *nova_string_dup(const nova_string_t *str);

/* ========================================================================
 * String Properties
 * ======================================================================== */

/**
 * @brief Get string length in bytes
 * @param str String
 * @return Length in bytes (not including NUL terminator)
 */
size_t nova_string_len(const nova_string_t *str);

/**
 * @brief Get string length in UTF-8 characters
 * @param str String
 * @return Number of UTF-8 code points
 */
size_t nova_string_len_utf8(const nova_string_t *str);

/**
 * @brief Get string capacity
 * @param str String
 * @return Reserved capacity in bytes
 */
size_t nova_string_capacity(const nova_string_t *str);

/**
 * @brief Check if string is empty
 * @param str String
 * @return true if empty
 */
bool nova_string_is_empty(const nova_string_t *str);

/**
 * @brief Get C string pointer (NUL-terminated)
 * @param str String
 * @return Pointer to internal buffer (do not modify!)
 */
const char *nova_string_cstr(const nova_string_t *str);

/**
 * @brief Get hash code
 * @param str String
 * @return Hash value
 */
uint64_t nova_string_hash(const nova_string_t *str);

/* ========================================================================
 * String Comparison
 * ======================================================================== */

/**
 * @brief Compare strings
 * @param a First string
 * @param b Second string
 * @return <0 if a<b, 0 if equal, >0 if a>b
 */
int nova_string_cmp(const nova_string_t *a, const nova_string_t *b);

/**
 * @brief Compare string with C string
 * @param str Nova string
 * @param cstr C string
 * @return <0 if str<cstr, 0 if equal, >0 if str>cstr
 */
int nova_string_cmp_cstr(const nova_string_t *str, const char *cstr);

/**
 * @brief Case-insensitive comparison
 * @param a First string
 * @param b Second string
 * @return <0 if a<b, 0 if equal, >0 if a>b
 */
int nova_string_cmp_nocase(const nova_string_t *a, const nova_string_t *b);

/**
 * @brief Check equality
 * @param a First string
 * @param b Second string
 * @return true if equal
 */
bool nova_string_eq(const nova_string_t *a, const nova_string_t *b);

/**
 * @brief Check if string starts with prefix
 * @param str String
 * @param prefix Prefix to check
 * @return true if str starts with prefix
 */
bool nova_string_starts_with(const nova_string_t *str, const char *prefix);

/**
 * @brief Check if string ends with suffix
 * @param str String
 * @param suffix Suffix to check
 * @return true if str ends with suffix
 */
bool nova_string_ends_with(const nova_string_t *str, const char *suffix);

/**
 * @brief Check if string contains substring
 * @param str String
 * @param substr Substring to find
 * @return true if found
 */
bool nova_string_contains(const nova_string_t *str, const char *substr);

/* ========================================================================
 * String Concatenation
 * ======================================================================== */

/**
 * @brief Concatenate two strings
 * @param a First string
 * @param b Second string
 * @return New concatenated string, or NULL on failure
 */
nova_string_t *nova_string_concat(const nova_string_t *a, const nova_string_t *b);

/**
 * @brief Concatenate with C string
 * @param str Nova string
 * @param cstr C string
 * @return New concatenated string, or NULL on failure
 */
nova_string_t *nova_string_concat_cstr(const nova_string_t *str, const char *cstr);

/**
 * @brief Join strings with separator
 * @param strings Array of strings
 * @param count Number of strings
 * @param sep Separator string
 * @return Joined string, or NULL on failure
 */
nova_string_t *nova_string_join(nova_string_t **strings, size_t count, const char *sep);

/**
 * @brief Repeat string n times
 * @param str String to repeat
 * @param count Number of repetitions
 * @return Repeated string, or NULL on failure
 */
nova_string_t *nova_string_repeat(const nova_string_t *str, size_t count);

/* ========================================================================
 * String Searching
 * ======================================================================== */

/**
 * @brief Find substring
 * @param str String to search in
 * @param substr Substring to find
 * @return Index of first occurrence, or -1 if not found
 */
ssize_t nova_string_find(const nova_string_t *str, const char *substr);

/**
 * @brief Find substring from end
 * @param str String to search in
 * @param substr Substring to find
 * @return Index of last occurrence, or -1 if not found
 */
ssize_t nova_string_rfind(const nova_string_t *str, const char *substr);

/**
 * @brief Find character
 * @param str String to search in
 * @param ch Character to find
 * @return Index of first occurrence, or -1 if not found
 */
ssize_t nova_string_find_char(const nova_string_t *str, char ch);

/**
 * @brief Count occurrences of substring
 * @param str String to search in
 * @param substr Substring to count
 * @return Number of occurrences
 */
size_t nova_string_count(const nova_string_t *str, const char *substr);

/* ========================================================================
 * String Modification (returns new string)
 * ======================================================================== */

/**
 * @brief Get substring
 * @param str Source string
 * @param start Start index
 * @param len Length in bytes
 * @return New substring, or NULL on failure
 */
nova_string_t *nova_string_substr(const nova_string_t *str, size_t start, size_t len);

/**
 * @brief Convert to lowercase
 * @param str Source string
 * @return New lowercase string, or NULL on failure
 */
nova_string_t *nova_string_tolower(const nova_string_t *str);

/**
 * @brief Convert to uppercase
 * @param str Source string
 * @return New uppercase string, or NULL on failure
 */
nova_string_t *nova_string_toupper(const nova_string_t *str);

/**
 * @brief Trim whitespace from both ends
 * @param str Source string
 * @return New trimmed string, or NULL on failure
 */
nova_string_t *nova_string_trim(const nova_string_t *str);

/**
 * @brief Trim whitespace from start
 * @param str Source string
 * @return New trimmed string, or NULL on failure
 */
nova_string_t *nova_string_ltrim(const nova_string_t *str);

/**
 * @brief Trim whitespace from end
 * @param str Source string
 * @return New trimmed string, or NULL on failure
 */
nova_string_t *nova_string_rtrim(const nova_string_t *str);

/**
 * @brief Replace all occurrences
 * @param str Source string
 * @param from String to replace
 * @param to Replacement string
 * @return New string with replacements, or NULL on failure
 */
nova_string_t *nova_string_replace(const nova_string_t *str, const char *from, const char *to);

/**
 * @brief Split string by delimiter
 * @param str String to split
 * @param delim Delimiter string
 * @param count Output: number of parts
 * @return Array of strings (caller must free), or NULL on failure
 */
nova_string_t **nova_string_split(const nova_string_t *str, const char *delim, size_t *count);

/* ========================================================================
 * String Views (Zero-copy slices)
 * ======================================================================== */

/**
 * @brief Create string view
 * @param data Pointer to data
 * @param len Length in bytes
 * @return String view
 */
nova_string_view_t nova_string_view(const char *data, size_t len);

/**
 * @brief Create view from string
 * @param str Source string
 * @return String view
 */
nova_string_view_t nova_string_as_view(const nova_string_t *str);

/**
 * @brief Create string from view
 * @param view String view
 * @return New string (copy), or NULL on failure
 */
nova_string_t *nova_string_from_view(nova_string_view_t view);

/* ========================================================================
 * Memory Management
 * ======================================================================== */

/**
 * @brief Destroy string and free memory
 * @param str String to destroy
 */
void nova_string_destroy(nova_string_t *str);

/**
 * @brief Increment reference count
 * @param str String
 * @return Same string pointer
 */
nova_string_t *nova_string_retain(nova_string_t *str);

/**
 * @brief Decrement reference count (destroys if reaches 0)
 * @param str String
 */
void nova_string_release(nova_string_t *str);

/* ========================================================================
 * UTF-8 Utilities
 * ======================================================================== */

/**
 * @brief Validate UTF-8 encoding
 * @param str String to validate
 * @return true if valid UTF-8
 */
bool nova_string_is_valid_utf8(const nova_string_t *str);

/**
 * @brief Get UTF-8 character at index
 * @param str String
 * @param index Character index (not byte index)
 * @param codepoint Output: Unicode codepoint
 * @return Byte length of character, or 0 on error
 */
size_t nova_string_char_at(const nova_string_t *str, size_t index, uint32_t *codepoint);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_STRING_H */
