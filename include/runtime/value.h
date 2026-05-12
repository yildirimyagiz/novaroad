/**
 * @file value.h
 * @brief Runtime value representation (tagged union for dynamic typing)
 * 
 * Nova values support:
 * - Tagged union representation
 * - Type checking and conversion
 * - Reference counting
 * - String interning
 * - Number coercion
 * - Pretty printing
 */

#ifndef NOVA_VALUE_H
#define NOVA_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Value Type System
 * ======================================================================== */

/**
 * Value types
 */
typedef enum {
    NOVA_VAL_NIL,           /**< Nil/null value */
    NOVA_VAL_BOOL,          /**< Boolean (true/false) */
    NOVA_VAL_INT,           /**< 64-bit signed integer */
    NOVA_VAL_FLOAT,         /**< 64-bit floating point */
    NOVA_VAL_CHAR,          /**< Unicode character (codepoint) */
    NOVA_VAL_STRING,        /**< UTF-8 string (heap-allocated) */
    NOVA_VAL_SYMBOL,        /**< Interned symbol */
    NOVA_VAL_ARRAY,         /**< Dynamic array */
    NOVA_VAL_OBJECT,        /**< Object/struct */
    NOVA_VAL_FUNCTION,      /**< Function/closure */
    NOVA_VAL_NATIVE,        /**< Native C function */
    NOVA_VAL_USERDATA,      /**< Opaque user data */
} nova_value_type_t;

/**
 * Tagged union value representation
 */
typedef struct {
    nova_value_type_t type;
    union {
        bool boolean;       /**< Boolean value */
        int64_t integer;    /**< Integer value */
        double floating;    /**< Float value */
        uint32_t character; /**< Unicode codepoint */
        void *object;       /**< Pointer to heap object */
        void *userdata;     /**< User data pointer */
    } as;
} nova_value_t;

/* ========================================================================
 * Value Construction
 * ======================================================================== */

/**
 * @brief Create nil value
 * @return Nil value
 */
nova_value_t nova_value_nil(void);

/**
 * @brief Create boolean value
 * @param b Boolean value
 * @return Boolean value
 */
nova_value_t nova_value_bool(bool b);

/**
 * @brief Create integer value
 * @param i Integer value
 * @return Integer value
 */
nova_value_t nova_value_int(int64_t i);

/**
 * @brief Create float value
 * @param f Float value
 * @return Float value
 */
nova_value_t nova_value_float(double f);

/**
 * @brief Create character value
 * @param ch Unicode codepoint
 * @return Character value
 */
nova_value_t nova_value_char(uint32_t ch);

/**
 * @brief Create string value
 * @param str C string (copied)
 * @return String value
 */
nova_value_t nova_value_string(const char *str);

/**
 * @brief Create string from buffer
 * @param data Buffer
 * @param len Length in bytes
 * @return String value
 */
nova_value_t nova_value_string_from_bytes(const char *data, size_t len);

/**
 * @brief Create interned symbol
 * @param name Symbol name
 * @return Symbol value
 */
nova_value_t nova_value_symbol(const char *name);

/**
 * @brief Create array value
 * @param capacity Initial capacity
 * @return Array value
 */
nova_value_t nova_value_array(size_t capacity);

/**
 * @brief Create object value
 * @return Object value
 */
nova_value_t nova_value_object(void);

/**
 * @brief Create native function value
 * @param func Function pointer
 * @return Function value
 */
nova_value_t nova_value_native(void *func);

/**
 * @brief Create user data value
 * @param ptr User data pointer
 * @return Userdata value
 */
nova_value_t nova_value_userdata(void *ptr);

/* ========================================================================
 * Type Checking
 * ======================================================================== */

/**
 * @brief Get value type
 * @param val Value
 * @return Value type
 */
nova_value_type_t nova_value_type(nova_value_t val);

/**
 * @brief Check if value is of specific type
 * @param val Value
 * @param type Expected type
 * @return true if matches
 */
bool nova_value_is_type(nova_value_t val, nova_value_type_t type);

/**
 * @brief Check if value is nil
 * @param val Value
 * @return true if nil
 */
bool nova_value_is_nil(nova_value_t val);

/**
 * @brief Check if value is boolean
 * @param val Value
 * @return true if boolean
 */
bool nova_value_is_bool(nova_value_t val);

/**
 * @brief Check if value is number (int or float)
 * @param val Value
 * @return true if number
 */
bool nova_value_is_number(nova_value_t val);

/**
 * @brief Check if value is integer
 * @param val Value
 * @return true if integer
 */
bool nova_value_is_int(nova_value_t val);

/**
 * @brief Check if value is float
 * @param val Value
 * @return true if float
 */
bool nova_value_is_float(nova_value_t val);

/**
 * @brief Check if value is string
 * @param val Value
 * @return true if string
 */
bool nova_value_is_string(nova_value_t val);

/**
 * @brief Check if value is callable (function or native)
 * @param val Value
 * @return true if callable
 */
bool nova_value_is_callable(nova_value_t val);

/* ========================================================================
 * Value Extraction
 * ======================================================================== */

/**
 * @brief Get boolean value
 * @param val Value
 * @return Boolean value (false if not boolean)
 */
bool nova_value_as_bool(nova_value_t val);

/**
 * @brief Get integer value
 * @param val Value
 * @return Integer value (0 if not integer)
 */
int64_t nova_value_as_int(nova_value_t val);

/**
 * @brief Get float value
 * @param val Value
 * @return Float value (0.0 if not float)
 */
double nova_value_as_float(nova_value_t val);

/**
 * @brief Get number as float (coerces int to float)
 * @param val Value
 * @return Float value
 */
double nova_value_as_number(nova_value_t val);

/**
 * @brief Get character value
 * @param val Value
 * @return Unicode codepoint
 */
uint32_t nova_value_as_char(nova_value_t val);

/**
 * @brief Get string pointer
 * @param val Value
 * @return C string pointer (do not modify!)
 */
const char *nova_value_as_string(nova_value_t val);

/**
 * @brief Get object pointer
 * @param val Value
 * @return Object pointer
 */
void *nova_value_as_object(nova_value_t val);

/**
 * @brief Get userdata pointer
 * @param val Value
 * @return Userdata pointer
 */
void *nova_value_as_userdata(nova_value_t val);

/* ========================================================================
 * Value Comparison
 * ======================================================================== */

/**
 * @brief Check equality
 * @param a First value
 * @param b Second value
 * @return true if equal
 */
bool nova_value_equals(nova_value_t a, nova_value_t b);

/**
 * @brief Compare values
 * @param a First value
 * @param b Second value
 * @return <0 if a<b, 0 if equal, >0 if a>b
 */
int nova_value_compare(nova_value_t a, nova_value_t b);

/**
 * @brief Hash value
 * @param val Value
 * @return Hash code
 */
uint64_t nova_value_hash(nova_value_t val);

/* ========================================================================
 * Type Conversion
 * ======================================================================== */

/**
 * @brief Convert to boolean (truthiness)
 * @param val Value
 * @return Boolean value (nil/false -> false, others -> true)
 */
bool nova_value_to_bool(nova_value_t val);

/**
 * @brief Convert to integer
 * @param val Value
 * @param result Output: integer value
 * @return true on success, false if conversion failed
 */
bool nova_value_to_int(nova_value_t val, int64_t *result);

/**
 * @brief Convert to float
 * @param val Value
 * @param result Output: float value
 * @return true on success, false if conversion failed
 */
bool nova_value_to_float(nova_value_t val, double *result);

/**
 * @brief Convert to string (allocates new string)
 * @param val Value
 * @return String representation (caller must free), or NULL on failure
 */
char *nova_value_to_string(nova_value_t val);

/* ========================================================================
 * Array Operations
 * ======================================================================== */

/**
 * @brief Get array length
 * @param val Array value
 * @return Length, or 0 if not array
 */
size_t nova_value_array_len(nova_value_t val);

/**
 * @brief Get array element
 * @param val Array value
 * @param index Index
 * @return Element value, or nil if out of bounds
 */
nova_value_t nova_value_array_get(nova_value_t val, size_t index);

/**
 * @brief Set array element
 * @param val Array value
 * @param index Index
 * @param element Element value
 * @return true on success, false if not array or out of bounds
 */
bool nova_value_array_set(nova_value_t val, size_t index, nova_value_t element);

/**
 * @brief Push element to array
 * @param val Array value
 * @param element Element to push
 * @return true on success, false if not array
 */
bool nova_value_array_push(nova_value_t val, nova_value_t element);

/**
 * @brief Pop element from array
 * @param val Array value
 * @return Popped element, or nil if empty or not array
 */
nova_value_t nova_value_array_pop(nova_value_t val);

/* ========================================================================
 * Object Operations
 * ======================================================================== */

/**
 * @brief Get object field
 * @param val Object value
 * @param key Field name
 * @return Field value, or nil if not found
 */
nova_value_t nova_value_object_get(nova_value_t val, const char *key);

/**
 * @brief Set object field
 * @param val Object value
 * @param key Field name
 * @param value Field value
 * @return true on success, false if not object
 */
bool nova_value_object_set(nova_value_t val, const char *key, nova_value_t value);

/**
 * @brief Check if object has field
 * @param val Object value
 * @param key Field name
 * @return true if field exists
 */
bool nova_value_object_has(nova_value_t val, const char *key);

/**
 * @brief Delete object field
 * @param val Object value
 * @param key Field name
 * @return true on success
 */
bool nova_value_object_delete(nova_value_t val, const char *key);

/**
 * @brief Get object keys
 * @param val Object value
 * @param count Output: number of keys
 * @return Array of key strings (caller must free), or NULL
 */
char **nova_value_object_keys(nova_value_t val, size_t *count);

/* ========================================================================
 * Memory Management
 * ======================================================================== */

/**
 * @brief Increment reference count
 * @param val Value to retain
 * @return Same value
 */
nova_value_t nova_value_retain(nova_value_t val);

/**
 * @brief Decrement reference count (may free)
 * @param val Value to release
 */
void nova_value_release(nova_value_t val);

/**
 * @brief Deep copy value
 * @param val Value to copy
 * @return Copied value
 */
nova_value_t nova_value_copy(nova_value_t val);

/* ========================================================================
 * Pretty Printing
 * ======================================================================== */

/**
 * @brief Print value to stdout
 * @param val Value to print
 */
void nova_value_print(nova_value_t val);

/**
 * @brief Print value with newline
 * @param val Value to print
 */
void nova_value_println(nova_value_t val);

/**
 * @brief Format value to string
 * @param val Value
 * @return Formatted string (caller must free), or NULL
 */
char *nova_value_format(nova_value_t val);

/**
 * @brief Debug print (shows type and internal representation)
 * @param val Value
 */
void nova_value_debug_print(nova_value_t val);

/* ========================================================================
 * Convenience Macros
 * ======================================================================== */

/**
 * @brief Create true value
 */
#define NOVA_TRUE nova_value_bool(true)

/**
 * @brief Create false value
 */
#define NOVA_FALSE nova_value_bool(false)

/**
 * @brief Create nil value
 */
#define NOVA_NIL nova_value_nil()

/**
 * @brief Check if value is truthy
 */
#define NOVA_IS_TRUTHY(val) nova_value_to_bool(val)

/**
 * @brief Check if value is falsy
 */
#define NOVA_IS_FALSY(val) (!nova_value_to_bool(val))

#ifdef __cplusplus
}
#endif

#endif /* NOVA_VALUE_H */
