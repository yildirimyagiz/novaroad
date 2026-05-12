/**
 * @file ffi_runtime.c
 * @brief FFI runtime implementation using libffi
 */

#include "runtime/ffi.h"
#include "std/alloc.h"
#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Check if libffi is available
#ifdef HAVE_LIBFFI
#include <ffi.h>
#define FFI_ENABLED 1
#else
#define FFI_ENABLED 0
#warning "libffi not available - FFI functionality will be limited"
#endif

// Forward declarations of internal helpers
#if FFI_ENABLED
static ffi_type *nova_ffi_type_to_libffi(nova_ffi_type_t type);
static void nova_ffi_arg_to_value(void *arg, ffi_type *type, nova_value_t *value);
static void *nova_value_to_ffi_arg(nova_value_t *value);
static void nova_ffi_value_init(nova_value_t *value, ffi_type *type);
static void nova_ffi_closure_trampoline(ffi_cif *cif, void *ret, void **args, void *user_data);
#endif

// Prepared call cache for performance
typedef struct prepared_call {
    void *function_ptr;
#if FFI_ENABLED
    ffi_cif cif;
    ffi_type **arg_types;
    ffi_type *return_type;
#endif
    size_t arg_count;
    struct prepared_call *next;
} prepared_call_t;

// Global FFI state
static struct {
    prepared_call_t *call_cache;
    pthread_mutex_t cache_mutex;
    atomic_bool initialized;
    atomic_size_t total_calls;
    atomic_size_t cache_hits;
    atomic_size_t cache_misses;
} ffi_state;

static prepared_call_t *find_prepared_call(void *function_ptr, nova_ffi_signature_t *signature)
{
    prepared_call_t *call = ffi_state.call_cache;
    while (call) {
        if (call->function_ptr == function_ptr && call->arg_count == signature->arg_count) {

#if FFI_ENABLED
            // Check if types match
            bool types_match = true;
            for (size_t i = 0; i < signature->arg_count; i++) {
                if (call->arg_types[i] != nova_ffi_type_to_libffi(signature->arg_types[i])) {
                    types_match = false;
                    break;
                }
            }
            if (types_match &&
                call->return_type == nova_ffi_type_to_libffi(signature->return_type)) {
                return call;
            }
#else
            return call; // Fallback matches by count and ptr
#endif
        }
        call = call->next;
    }
    return NULL;
}

static prepared_call_t *create_prepared_call(void *function_ptr, nova_ffi_signature_t *signature)
{
    prepared_call_t *call = nova_alloc(sizeof(prepared_call_t));
    if (!call)
        return NULL;

    memset(call, 0, sizeof(prepared_call_t));
    call->function_ptr = function_ptr;
    call->arg_count = signature->arg_count;

#if FFI_ENABLED
    // Allocate type arrays
    call->arg_types = nova_alloc(signature->arg_count * sizeof(ffi_type *));
    if (!call->arg_types) {
        nova_free(call);
        return NULL;
    }

    // Convert Nova types to libffi types
    for (size_t i = 0; i < signature->arg_count; i++) {
        call->arg_types[i] = nova_ffi_type_to_libffi(signature->arg_types[i]);
    }
    call->return_type = nova_ffi_type_to_libffi(signature->return_type);

    // Prepare CIF
    if (ffi_prep_cif(&call->cif, FFI_DEFAULT_ABI, (unsigned int) signature->arg_count,
                     call->return_type, call->arg_types) != FFI_OK) {
        nova_free(call->arg_types);
        nova_free(call);
        return NULL;
    }
#else
    (void) signature;
#endif

    // Add to cache
    pthread_mutex_lock(&ffi_state.cache_mutex);
    call->next = ffi_state.call_cache;
    ffi_state.call_cache = call;
    pthread_mutex_unlock(&ffi_state.cache_mutex);

    return call;
}

nova_ffi_call_t *nova_ffi_call_prepare(void *function_ptr, nova_ffi_signature_t *signature)
{
    if (!function_ptr || !signature)
        return NULL;

    // Try to find in cache
    prepared_call_t *call = find_prepared_call(function_ptr, signature);
    if (call) {
        atomic_fetch_add(&ffi_state.cache_hits, 1);
    } else {
        atomic_fetch_add(&ffi_state.cache_misses, 1);
        call = create_prepared_call(function_ptr, signature);
        if (!call)
            return NULL;
    }

    nova_ffi_call_t *ffi_call = nova_alloc(sizeof(nova_ffi_call_t));
    if (!ffi_call)
        return NULL;

    ffi_call->function_ptr = function_ptr;
    ffi_call->prepared_call = call;
    memcpy(&ffi_call->signature, signature, sizeof(nova_ffi_signature_t));
    return ffi_call;
}

void nova_ffi_call_destroy(nova_ffi_call_t *call)
{
    if (!call)
        return;
    nova_free(call);
}

nova_value_t nova_ffi_call_execute(nova_ffi_call_t *call, nova_value_t *args)
{
    nova_value_t result = {0};
    result.type = NOVA_VAL_NIL;

    if (!call)
        return result;

    atomic_fetch_add(&ffi_state.total_calls, 1);

#if FFI_ENABLED
    prepared_call_t *pcall = (prepared_call_t *) call->prepared_call;

    // Prepare arguments
    void **ffi_args = nova_alloc(pcall->arg_count * sizeof(void *));
    if (!ffi_args)
        return result;

    for (size_t i = 0; i < pcall->arg_count; i++) {
        ffi_args[i] = nova_value_to_ffi_arg(&args[i]);
    }

    // Call function
    nova_value_t return_value;
    nova_ffi_value_init(&return_value, pcall->return_type);

    ffi_call(&pcall->cif, pcall->function_ptr, nova_value_to_ffi_arg(&return_value), ffi_args);

    nova_free(ffi_args);
    return return_value;
#else
    // Simple fallback - only support void functions with no args
    if (call->signature.arg_count == 0) {
        void (*func)(void) = call->function_ptr;
        func();
    }
    (void) args;
    return result;
#endif
}

nova_ffi_closure_t *nova_ffi_closure_create(nova_ffi_signature_t *signature,
                                            nova_ffi_callback_t callback, void *user_data)
{
    if (!callback || !signature)
        return NULL;

#if FFI_ENABLED
    nova_ffi_closure_t *closure = nova_alloc(sizeof(nova_ffi_closure_t));
    if (!closure)
        return NULL;

    // Allocate closure code
    closure->closure = ffi_closure_alloc(sizeof(ffi_closure), &closure->code);
    if (!closure->closure) {
        nova_free(closure);
        return NULL;
    }

    // Prepare CIF for callback
    ffi_type **arg_types = nova_alloc(signature->arg_count * sizeof(ffi_type *));
    if (!arg_types) {
        ffi_closure_free(closure->closure);
        nova_free(closure);
        return NULL;
    }

    for (size_t i = 0; i < signature->arg_count; i++) {
        arg_types[i] = nova_ffi_type_to_libffi(signature->arg_types[i]);
    }

    ffi_type *return_type = nova_ffi_type_to_libffi(signature->return_type);

    closure->cif = nova_alloc(sizeof(ffi_cif));
    if (ffi_prep_cif((ffi_cif *) closure->cif, FFI_DEFAULT_ABI, (unsigned int) signature->arg_count,
                     return_type, arg_types) != FFI_OK) {
        nova_free(arg_types);
        ffi_closure_free(closure->closure);
        nova_free(closure->cif);
        nova_free(closure);
        return NULL;
    }

    // Set up closure
    closure->callback = callback;
    closure->user_data = user_data;

    if (ffi_prep_closure_loc(closure->closure, (ffi_cif *) closure->cif,
                             nova_ffi_closure_trampoline, closure, closure->code) != FFI_OK) {
        nova_free(arg_types);
        ffi_closure_free(closure->closure);
        nova_free(closure->cif);
        nova_free(closure);
        return NULL;
    }

    nova_free(arg_types);
    return closure;
#else
    // Fallback without libffi
    (void) signature;
    (void) callback;
    (void) user_data;
    return NULL;
#endif
}

void nova_ffi_closure_destroy(nova_ffi_closure_t *closure)
{
    if (!closure)
        return;

#if FFI_ENABLED
    if (closure->closure) {
        ffi_closure_free(closure->closure);
    }
    if (closure->cif) {
        nova_free(closure->cif);
    }
#endif

    nova_free(closure);
}

void *nova_ffi_closure_get_code(nova_ffi_closure_t *closure)
{
    if (!closure)
        return NULL;

#if FFI_ENABLED
    return closure->code;
#else
    return NULL;
#endif
}

#if FFI_ENABLED
static void nova_ffi_closure_trampoline(ffi_cif *cif, void *ret, void **args, void *user_data)
{
    nova_ffi_closure_t *closure = user_data;

    // Convert FFI args to Nova values
    nova_value_t *nova_args = nova_alloc(cif->nargs * sizeof(nova_value_t));
    if (!nova_args)
        return;

    for (size_t i = 0; i < cif->nargs; i++) {
        nova_ffi_arg_to_value(args[i], cif->arg_types[i], &nova_args[i]);
    }

    // Call callback
    nova_value_t result = closure->callback(nova_args, (int) cif->nargs, closure->user_data);

    // Convert result back
    // Use the result if ret is available
    if (ret) {
        void *res_ptr = nova_value_to_ffi_arg(&result);
        if (res_ptr) {
            memcpy(ret, res_ptr, cif->rtype->size);
        }
    }

    nova_free(nova_args);
}

static ffi_type *nova_ffi_type_to_libffi(nova_ffi_type_t type)
{
    switch (type) {
    case NOVA_FFI_TYPE_VOID:
        return &ffi_type_void;
    case NOVA_FFI_TYPE_UINT8:
        return &ffi_type_uint8;
    case NOVA_FFI_TYPE_SINT8:
        return &ffi_type_sint8;
    case NOVA_FFI_TYPE_UINT16:
        return &ffi_type_uint16;
    case NOVA_FFI_TYPE_SINT16:
        return &ffi_type_sint16;
    case NOVA_FFI_TYPE_UINT32:
        return &ffi_type_uint32;
    case NOVA_FFI_TYPE_SINT32:
        return &ffi_type_sint32;
    case NOVA_FFI_TYPE_UINT64:
        return &ffi_type_uint64;
    case NOVA_FFI_TYPE_SINT64:
        return &ffi_type_sint64;
    case NOVA_FFI_TYPE_FLOAT:
        return &ffi_type_float;
    case NOVA_FFI_TYPE_DOUBLE:
        return &ffi_type_double;
    case NOVA_FFI_TYPE_POINTER:
        return &ffi_type_pointer;
    default:
        return &ffi_type_void;
    }
}

static void nova_ffi_arg_to_value(void *arg, ffi_type *type, nova_value_t *value)
{
    if (type == &ffi_type_uint8) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(uint8_t *) arg;
    } else if (type == &ffi_type_sint8) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(int8_t *) arg;
    } else if (type == &ffi_type_uint16) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(uint16_t *) arg;
    } else if (type == &ffi_type_sint16) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(int16_t *) arg;
    } else if (type == &ffi_type_uint32) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(uint32_t *) arg;
    } else if (type == &ffi_type_sint32) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(int32_t *) arg;
    } else if (type == &ffi_type_uint64) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(uint64_t *) arg;
    } else if (type == &ffi_type_sint64) {
        value->type = NOVA_VAL_INT;
        value->as.integer = *(int64_t *) arg;
    } else if (type == &ffi_type_float) {
        value->type = NOVA_VAL_FLOAT;
        value->as.floating = *(float *) arg;
    } else if (type == &ffi_type_double) {
        value->type = NOVA_VAL_FLOAT;
        value->as.floating = *(double *) arg;
    } else if (type == &ffi_type_pointer) {
        value->type = NOVA_VAL_USERDATA;
        value->as.userdata = *(void **) arg;
    } else {
        value->type = NOVA_VAL_NIL;
    }
}

static void *nova_value_to_ffi_arg(nova_value_t *value)
{
    switch (value->type) {
    case NOVA_VAL_INT:
        return &value->as.integer;
    case NOVA_VAL_FLOAT:
        return &value->as.floating;
    case NOVA_VAL_USERDATA:
        return &value->as.userdata;
    case NOVA_VAL_OBJECT:
        return &value->as.object;
    default:
        return NULL;
    }
}

static void nova_ffi_value_init(nova_value_t *value, ffi_type *type)
{
    if (type == &ffi_type_uint8 || type == &ffi_type_sint8 || type == &ffi_type_uint16 ||
        type == &ffi_type_sint16 || type == &ffi_type_uint32 || type == &ffi_type_sint32 ||
        type == &ffi_type_uint64 || type == &ffi_type_sint64) {
        value->type = NOVA_VAL_INT;
    } else if (type == &ffi_type_float || type == &ffi_type_double) {
        value->type = NOVA_VAL_FLOAT;
    } else if (type == &ffi_type_pointer) {
        value->type = NOVA_VAL_USERDATA;
    } else {
        value->type = NOVA_VAL_NIL;
    }
}
#endif

nova_ffi_library_t *nova_ffi_library_load(const char *path)
{
    if (!path)
        return NULL;

    void *handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to load FFI library %s: %s\n", path, dlerror());
        return NULL;
    }

    nova_ffi_library_t *library = nova_alloc(sizeof(nova_ffi_library_t));
    if (!library) {
        dlclose(handle);
        return NULL;
    }

    library->path = strdup(path);
    library->handle = handle;

    return library;
}

void nova_ffi_library_destroy(nova_ffi_library_t *library)
{
    if (!library)
        return;

    if (library->handle) {
        dlclose(library->handle);
    }

    nova_free(library->path);
    nova_free(library);
}

void *nova_ffi_library_symbol(nova_ffi_library_t *library, const char *symbol)
{
    if (!library || !symbol)
        return NULL;

    dlerror(); // Clear error
    void *ptr = dlsym(library->handle, symbol);
    const char *error = dlerror();
    if (error) {
        fprintf(stderr, "Failed to find symbol %s: %s\n", symbol, error);
        return NULL;
    }

    return ptr;
}

nova_ffi_signature_t *nova_ffi_signature_create(nova_ffi_type_t return_type,
                                                nova_ffi_type_t *arg_types, size_t arg_count)
{
    nova_ffi_signature_t *signature = nova_alloc(sizeof(nova_ffi_signature_t));
    if (!signature)
        return NULL;

    signature->return_type = return_type;
    signature->arg_count = arg_count;

    if (arg_count > 0) {
        signature->arg_types = nova_alloc(arg_count * sizeof(nova_ffi_type_t));
        if (!signature->arg_types) {
            nova_free(signature);
            return NULL;
        }
        memcpy(signature->arg_types, arg_types, arg_count * sizeof(nova_ffi_type_t));
    } else {
        signature->arg_types = NULL;
    }

    return signature;
}

void nova_ffi_signature_destroy(nova_ffi_signature_t *signature)
{
    if (!signature)
        return;

    nova_free(signature->arg_types);
    nova_free(signature);
}

void nova_ffi_init(void)
{
    if (atomic_load(&ffi_state.initialized))
        return;

    memset(&ffi_state, 0, sizeof(ffi_state));
    pthread_mutex_init(&ffi_state.cache_mutex, NULL);

    atomic_init(&ffi_state.total_calls, 0);
    atomic_init(&ffi_state.cache_hits, 0);
    atomic_init(&ffi_state.cache_misses, 0);

    atomic_store(&ffi_state.initialized, true);
}

void nova_ffi_shutdown(void)
{
    if (!atomic_load(&ffi_state.initialized))
        return;

    pthread_mutex_lock(&ffi_state.cache_mutex);

    // Clean up call cache
    prepared_call_t *call = ffi_state.call_cache;
    while (call) {
        prepared_call_t *next = call->next;
#if FFI_ENABLED
        nova_free(call->arg_types);
#endif
        nova_free(call);
        call = next;
    }

    pthread_mutex_unlock(&ffi_state.cache_mutex);
    pthread_mutex_destroy(&ffi_state.cache_mutex);

    atomic_store(&ffi_state.initialized, false);
}

nova_ffi_stats_t nova_ffi_stats(void)
{
    nova_ffi_stats_t stats = {0};

    stats.total_calls = atomic_load(&ffi_state.total_calls);
    stats.cache_hits = atomic_load(&ffi_state.cache_hits);
    stats.cache_misses = atomic_load(&ffi_state.cache_misses);

    if (stats.total_calls > 0) {
        stats.cache_hit_rate = (double) stats.cache_hits / (double) stats.total_calls * 100.0;
    }

    // Count cached calls
    pthread_mutex_lock(&ffi_state.cache_mutex);
    prepared_call_t *call = ffi_state.call_cache;
    while (call) {
        stats.cached_calls++;
        call = call->next;
    }
    pthread_mutex_unlock(&ffi_state.cache_mutex);

    return stats;
}
