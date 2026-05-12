/**
 * @file interop.h
 * @brief Higher-level C interop layer
 */

#ifndef NOVA_INTEROP_H
#define NOVA_INTEROP_H

#include "ffi.h"
#include "value.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t nova_callback_id_t;
typedef nova_value_t (*nova_callback_function_t)(nova_value_t *args, int nargs, void *user_data);

typedef enum {
    NOVA_C_TYPE_VOID,
    NOVA_C_TYPE_CHAR,
    NOVA_C_TYPE_SHORT,
    NOVA_C_TYPE_INT,
    NOVA_C_TYPE_LONG,
    NOVA_C_TYPE_LONG_LONG,
    NOVA_C_TYPE_FLOAT,
    NOVA_C_TYPE_DOUBLE,
    NOVA_C_TYPE_POINTER,
    NOVA_C_TYPE_STRUCT
} nova_c_type_t;

typedef struct {
    char *name;
    nova_c_type_t type;
    size_t offset;
} nova_c_field_t;

typedef struct {
    char *name;
    nova_c_field_t *fields;
    size_t field_count;
    size_t size;
} nova_c_struct_t;

typedef struct {
    void *data;
    size_t length;
    nova_c_type_t element_type;
    size_t element_size;
} nova_c_array_t;

typedef struct {
    char *name;
    char *library_path;
    void *function_ptr;
    void *library_handle;
    nova_ffi_signature_t signature;
} nova_c_function_t;

typedef struct {
    size_t function_count;
    size_t callback_count;
} nova_c_interop_stats_t;

/* ========================================================================
 * Function Interop
 * ======================================================================== */

nova_c_function_t *nova_c_function_load(const char *library_path, const char *function_name);
void nova_c_function_destroy(nova_c_function_t *c_func);
nova_value_t nova_c_function_call(nova_c_function_t *c_func, nova_value_t *args, size_t arg_count);
int nova_c_function_set_signature(nova_c_function_t *c_func, nova_ffi_signature_t *signature);

/* ========================================================================
 * Callbacks
 * ======================================================================== */

nova_callback_id_t nova_callback_register(nova_callback_function_t callback,
                                          nova_ffi_signature_t *signature, void *user_data);
int nova_callback_unregister(nova_callback_id_t callback_id);
nova_callback_function_t nova_callback_get_function(nova_callback_id_t callback_id);
void *nova_callback_get_user_data(nova_callback_id_t callback_id);

/* ========================================================================
 * Struct & Array Interop
 * ======================================================================== */

nova_c_struct_t *nova_c_struct_define(const char *name, nova_c_field_t *fields, size_t field_count);
void nova_c_struct_destroy(nova_c_struct_t *struct_def);
size_t nova_c_type_size(nova_c_type_t type);
size_t nova_c_type_alignment(nova_c_type_t type);

void *nova_c_struct_create(nova_c_struct_t *struct_def);
void nova_c_struct_destroy_instance(void *instance);
int nova_c_struct_set_field(nova_c_struct_t *struct_def, void *instance, const char *field_name,
                            const void *value);
int nova_c_struct_get_field(nova_c_struct_t *struct_def, const void *instance,
                            const char *field_name, void *value);

nova_c_array_t *nova_c_array_create(nova_c_type_t element_type, size_t length);
void nova_c_array_destroy(nova_c_array_t *array);
int nova_c_array_set(nova_c_array_t *array, size_t index, const void *value);
int nova_c_array_get(nova_c_array_t *array, size_t index, void *value);

/* ========================================================================
 * Interop System
 * ======================================================================== */

void nova_c_interop_init(void);
void nova_c_interop_shutdown(void);
nova_c_interop_stats_t nova_c_interop_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_INTEROP_H */
