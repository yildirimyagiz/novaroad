/**
 * @file ffi.h
 * @brief Foreign Function Interface (FFI) implementation
 */

#ifndef NOVA_FFI_H
#define NOVA_FFI_H

#include "value.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOVA_FFI_TYPE_VOID,
    NOVA_FFI_TYPE_UINT8,
    NOVA_FFI_TYPE_SINT8,
    NOVA_FFI_TYPE_UINT16,
    NOVA_FFI_TYPE_SINT16,
    NOVA_FFI_TYPE_UINT32,
    NOVA_FFI_TYPE_SINT32,
    NOVA_FFI_TYPE_UINT64,
    NOVA_FFI_TYPE_SINT64,
    NOVA_FFI_TYPE_FLOAT,
    NOVA_FFI_TYPE_DOUBLE,
    NOVA_FFI_TYPE_POINTER
} nova_ffi_type_t;

typedef struct {
    nova_ffi_type_t return_type;
    size_t arg_count;
    nova_ffi_type_t *arg_types;
} nova_ffi_signature_t;

typedef struct {
    void *function_ptr;
    void *prepared_call; // internal use
    nova_ffi_signature_t signature;
} nova_ffi_call_t;

typedef nova_value_t (*nova_ffi_callback_t)(nova_value_t *args, int nargs, void *user_data);

typedef struct {
    void *closure;
    void *code;
    nova_ffi_callback_t callback;
    void *user_data;
    void *cif; // internal ffi_cif
} nova_ffi_closure_t;

typedef struct {
    char *path;
    void *handle;
} nova_ffi_library_t;

typedef struct {
    size_t total_calls;
    size_t cache_hits;
    size_t cache_misses;
    double cache_hit_rate;
    size_t cached_calls;
} nova_ffi_stats_t;

/* ========================================================================
 * Library Loading
 * ======================================================================== */

nova_ffi_library_t *nova_ffi_library_load(const char *path);
void nova_ffi_library_destroy(nova_ffi_library_t *library);
void *nova_ffi_library_symbol(nova_ffi_library_t *library, const char *symbol);

/* ========================================================================
 * Signatures & Calls
 * ======================================================================== */

nova_ffi_signature_t *nova_ffi_signature_create(nova_ffi_type_t return_type,
                                                nova_ffi_type_t *arg_types, size_t arg_count);
void nova_ffi_signature_destroy(nova_ffi_signature_t *signature);

nova_ffi_call_t *nova_ffi_call_prepare(void *function_ptr, nova_ffi_signature_t *signature);
void nova_ffi_call_destroy(nova_ffi_call_t *call);
nova_value_t nova_ffi_call_execute(nova_ffi_call_t *call, nova_value_t *args);

/* ========================================================================
 * Closures (Callbacks from C)
 * ======================================================================== */

nova_ffi_closure_t *nova_ffi_closure_create(nova_ffi_signature_t *signature,
                                            nova_ffi_callback_t callback, void *user_data);
void nova_ffi_closure_destroy(nova_ffi_closure_t *closure);
void *nova_ffi_closure_get_code(nova_ffi_closure_t *closure);

/* ========================================================================
 * FFI System
 * ======================================================================== */

void nova_ffi_init(void);
void nova_ffi_shutdown(void);
nova_ffi_stats_t nova_ffi_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_FFI_H */
