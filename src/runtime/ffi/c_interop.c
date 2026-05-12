/**
 * @file c_interop.c
 * @brief C language interop with function calling and callbacks
 */

#include "runtime/ffi.h"
#include "runtime/interop.h"
#include "std/alloc.h"
#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function registry for C functions
typedef struct c_function_entry {
    char *name;
    void *function_ptr;
    nova_ffi_signature_t signature;
    struct c_function_entry *next;
} c_function_entry_t;

// Callback registry for Nova->C callbacks
typedef struct c_callback_entry {
    nova_callback_id_t id;
    nova_callback_function_t function;
    void *user_data;
    nova_ffi_signature_t signature;
    atomic_int ref_count;
    struct c_callback_entry *next;
} c_callback_entry_t;

// Global state
static struct {
    c_function_entry_t *function_registry;
    c_callback_entry_t *callback_registry;
    atomic_size_t next_callback_id;
    pthread_mutex_t registry_mutex;
    atomic_bool initialized;
} c_interop_state;

static nova_callback_id_t generate_callback_id(void)
{
    static atomic_size_t next_id = 1;
    return atomic_fetch_add(&next_id, 1);
}

nova_c_function_t *nova_c_function_load(const char *library_path, const char *function_name)
{
    if (!library_path || !function_name)
        return NULL;

    // Load shared library
    void *handle = dlopen(library_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Failed to load library %s: %s\n", library_path, dlerror());
        return NULL;
    }

    // Clear dlerror
    dlerror();

    // Get function pointer
    void *func_ptr = dlsym(handle, function_name);
    const char *error = dlerror();
    if (error) {
        fprintf(stderr, "Failed to find function %s: %s\n", function_name, error);
        dlclose(handle);
        return NULL;
    }

    // Create function wrapper
    nova_c_function_t *c_func = nova_alloc(sizeof(nova_c_function_t));
    if (!c_func) {
        dlclose(handle);
        return NULL;
    }

    c_func->name = strdup(function_name);
    c_func->library_path = strdup(library_path);
    c_func->function_ptr = func_ptr;
    c_func->library_handle = handle;
    memset(&c_func->signature, 0, sizeof(nova_ffi_signature_t));

    // Register function
    c_function_entry_t *entry = nova_alloc(sizeof(c_function_entry_t));
    if (!entry) {
        nova_free(c_func->name);
        nova_free(c_func->library_path);
        nova_free(c_func);
        dlclose(handle);
        return NULL;
    }

    entry->name = strdup(function_name);
    entry->function_ptr = func_ptr;
    entry->signature = c_func->signature;

    pthread_mutex_lock(&c_interop_state.registry_mutex);
    entry->next = c_interop_state.function_registry;
    c_interop_state.function_registry = entry;
    pthread_mutex_unlock(&c_interop_state.registry_mutex);

    return c_func;
}

void nova_c_function_destroy(nova_c_function_t *c_func)
{
    if (!c_func)
        return;

    // Close library if we loaded it
    if (c_func->library_handle) {
        dlclose(c_func->library_handle);
    }

    nova_free(c_func->name);
    nova_free(c_func->library_path);
    nova_free(c_func);
}

nova_value_t nova_c_function_call(nova_c_function_t *c_func, nova_value_t *args, size_t arg_count)
{
    nova_value_t result = {0};
    result.type = NOVA_VAL_NIL;

    if (!c_func || !c_func->function_ptr)
        return result;

    // For now, we only support simple void functions with no arguments
    // In a real implementation, we'd use libffi for proper calling convention handling
    if (arg_count == 0) {
        void (*func)(void) = c_func->function_ptr;
        func();
        result.type = NOVA_VAL_NIL; // No return value
    }

    return result;
}

int nova_c_function_set_signature(nova_c_function_t *c_func, nova_ffi_signature_t *signature)
{
    if (!c_func || !signature)
        return -1;

    c_func->signature = *signature;
    return 0;
}

nova_callback_id_t nova_callback_register(nova_callback_function_t callback,
                                          nova_ffi_signature_t *signature, void *user_data)
{
    if (!callback)
        return 0;

    c_callback_entry_t *entry = nova_alloc(sizeof(c_callback_entry_t));
    if (!entry)
        return 0;

    entry->id = generate_callback_id();
    entry->function = callback;
    entry->user_data = user_data;
    if (signature) {
        entry->signature = *signature;
    } else {
        memset(&entry->signature, 0, sizeof(nova_ffi_signature_t));
    }
    atomic_init(&entry->ref_count, 1);

    pthread_mutex_lock(&c_interop_state.registry_mutex);
    entry->next = c_interop_state.callback_registry;
    c_interop_state.callback_registry = entry;
    pthread_mutex_unlock(&c_interop_state.registry_mutex);

    return entry->id;
}

int nova_callback_unregister(nova_callback_id_t callback_id)
{
    if (callback_id == 0)
        return -1;

    pthread_mutex_lock(&c_interop_state.registry_mutex);

    c_callback_entry_t *prev = NULL;
    c_callback_entry_t *curr = c_interop_state.callback_registry;

    while (curr) {
        if (curr->id == callback_id) {
            if (atomic_fetch_sub(&curr->ref_count, 1) == 1) {
                // Remove from list
                if (prev) {
                    prev->next = curr->next;
                } else {
                    c_interop_state.callback_registry = curr->next;
                }
                nova_free(curr);
                pthread_mutex_unlock(&c_interop_state.registry_mutex);
                return 0;
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    pthread_mutex_unlock(&c_interop_state.registry_mutex);
    return -1;
}

nova_callback_function_t nova_callback_get_function(nova_callback_id_t callback_id)
{
    if (callback_id == 0)
        return NULL;

    pthread_mutex_lock(&c_interop_state.registry_mutex);

    c_callback_entry_t *curr = c_interop_state.callback_registry;
    while (curr) {
        if (curr->id == callback_id) {
            nova_callback_function_t func = curr->function;
            pthread_mutex_unlock(&c_interop_state.registry_mutex);
            return func;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&c_interop_state.registry_mutex);
    return NULL;
}

void *nova_callback_get_user_data(nova_callback_id_t callback_id)
{
    if (callback_id == 0)
        return NULL;

    pthread_mutex_lock(&c_interop_state.registry_mutex);

    c_callback_entry_t *curr = c_interop_state.callback_registry;
    while (curr) {
        if (curr->id == callback_id) {
            void *data = curr->user_data;
            pthread_mutex_unlock(&c_interop_state.registry_mutex);
            return data;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&c_interop_state.registry_mutex);
    return NULL;
}

nova_c_struct_t *nova_c_struct_define(const char *name, nova_c_field_t *fields, size_t field_count)
{
    nova_c_struct_t *struct_def = nova_alloc(sizeof(nova_c_struct_t));
    if (!struct_def)
        return NULL;

    struct_def->name = strdup(name);
    struct_def->fields = nova_alloc(field_count * sizeof(nova_c_field_t));
    if (!struct_def->fields) {
        nova_free(struct_def->name);
        nova_free(struct_def);
        return NULL;
    }

    memcpy(struct_def->fields, fields, field_count * sizeof(nova_c_field_t));
    struct_def->field_count = field_count;

    // Calculate struct size and offsets
    size_t offset = 0;
    size_t max_align = 1;

    for (size_t i = 0; i < field_count; i++) {
        size_t align = nova_c_type_alignment(fields[i].type);
        max_align = align > max_align ? align : max_align;

        // Align offset
        offset = (offset + align - 1) & ~(align - 1);

        struct_def->fields[i].offset = offset;
        offset += nova_c_type_size(fields[i].type);
    }

    // Align total size
    struct_def->size = (offset + max_align - 1) & ~(max_align - 1);

    return struct_def;
}

void nova_c_struct_destroy(nova_c_struct_t *struct_def)
{
    if (!struct_def)
        return;

    nova_free(struct_def->name);
    nova_free(struct_def->fields);
    nova_free(struct_def);
}

size_t nova_c_type_size(nova_c_type_t type)
{
    switch (type) {
    case NOVA_C_TYPE_VOID:
        return 0;
    case NOVA_C_TYPE_CHAR:
        return sizeof(char);
    case NOVA_C_TYPE_SHORT:
        return sizeof(short);
    case NOVA_C_TYPE_INT:
        return sizeof(int);
    case NOVA_C_TYPE_LONG:
        return sizeof(long);
    case NOVA_C_TYPE_LONG_LONG:
        return sizeof(long long);
    case NOVA_C_TYPE_FLOAT:
        return sizeof(float);
    case NOVA_C_TYPE_DOUBLE:
        return sizeof(double);
    case NOVA_C_TYPE_POINTER:
        return sizeof(void *);
    case NOVA_C_TYPE_STRUCT:
        return 0; // Variable size
    default:
        return 0;
    }
}

size_t nova_c_type_alignment(nova_c_type_t type)
{
    // For simplicity, use size as alignment (correct for most types)
    return nova_c_type_size(type);
}

void *nova_c_struct_create(nova_c_struct_t *struct_def)
{
    if (!struct_def)
        return NULL;

    void *instance = nova_alloc(struct_def->size);
    if (instance) {
        memset(instance, 0, struct_def->size);
    }
    return instance;
}

void nova_c_struct_destroy_instance(void *instance)
{
    nova_free(instance);
}

int nova_c_struct_set_field(nova_c_struct_t *struct_def, void *instance, const char *field_name,
                            const void *value)
{
    if (!struct_def || !instance || !field_name || !value)
        return -1;

    for (size_t i = 0; i < struct_def->field_count; i++) {
        if (strcmp(struct_def->fields[i].name, field_name) == 0) {
            void *field_ptr = (char *) instance + struct_def->fields[i].offset;
            size_t size = nova_c_type_size(struct_def->fields[i].type);
            memcpy(field_ptr, value, size);
            return 0;
        }
    }

    return -1; // Field not found
}

int nova_c_struct_get_field(nova_c_struct_t *struct_def, const void *instance,
                            const char *field_name, void *value)
{
    if (!struct_def || !instance || !field_name || !value)
        return -1;

    for (size_t i = 0; i < struct_def->field_count; i++) {
        if (strcmp(struct_def->fields[i].name, field_name) == 0) {
            const void *field_ptr = (const char *) instance + struct_def->fields[i].offset;
            size_t size = nova_c_type_size(struct_def->fields[i].type);
            memcpy(value, field_ptr, size);
            return 0;
        }
    }

    return -1; // Field not found
}

nova_c_array_t *nova_c_array_create(nova_c_type_t element_type, size_t length)
{
    nova_c_array_t *array = nova_alloc(sizeof(nova_c_array_t));
    if (!array)
        return NULL;

    array->element_type = element_type;
    array->length = length;
    array->element_size = nova_c_type_size(element_type);

    array->data = nova_alloc(array->element_size * length);
    if (!array->data) {
        nova_free(array);
        return NULL;
    }

    memset(array->data, 0, array->element_size * length);
    return array;
}

void nova_c_array_destroy(nova_c_array_t *array)
{
    if (!array)
        return;

    nova_free(array->data);
    nova_free(array);
}

int nova_c_array_set(nova_c_array_t *array, size_t index, const void *value)
{
    if (!array || !value || index >= array->length)
        return -1;

    void *element_ptr = (char *) array->data + (index * array->element_size);
    memcpy(element_ptr, value, array->element_size);
    return 0;
}

int nova_c_array_get(nova_c_array_t *array, size_t index, void *value)
{
    if (!array || !value || index >= array->length)
        return -1;

    const void *element_ptr = (const char *) array->data + (index * array->element_size);
    memcpy(value, element_ptr, array->element_size);
    return 0;
}

void nova_c_interop_init(void)
{
    if (atomic_load(&c_interop_state.initialized))
        return;

    memset(&c_interop_state, 0, sizeof(c_interop_state));
    atomic_init(&c_interop_state.next_callback_id, 1);
    pthread_mutex_init(&c_interop_state.registry_mutex, NULL);

    atomic_store(&c_interop_state.initialized, true);
}

void nova_c_interop_shutdown(void)
{
    if (!atomic_load(&c_interop_state.initialized))
        return;

    pthread_mutex_lock(&c_interop_state.registry_mutex);

    // Clean up function registry
    c_function_entry_t *func_entry = c_interop_state.function_registry;
    while (func_entry) {
        c_function_entry_t *next = func_entry->next;
        nova_free(func_entry->name);
        nova_free(func_entry);
        func_entry = next;
    }

    // Clean up callback registry
    c_callback_entry_t *callback_entry = c_interop_state.callback_registry;
    while (callback_entry) {
        c_callback_entry_t *next = callback_entry->next;
        nova_free(callback_entry);
        callback_entry = next;
    }

    pthread_mutex_unlock(&c_interop_state.registry_mutex);
    pthread_mutex_destroy(&c_interop_state.registry_mutex);

    atomic_store(&c_interop_state.initialized, false);
}

nova_c_interop_stats_t nova_c_interop_stats(void)
{
    nova_c_interop_stats_t stats = {0};

    pthread_mutex_lock(&c_interop_state.registry_mutex);

    // Count functions
    c_function_entry_t *func = c_interop_state.function_registry;
    while (func) {
        stats.function_count++;
        func = func->next;
    }

    // Count callbacks
    c_callback_entry_t *callback = c_interop_state.callback_registry;
    while (callback) {
        stats.callback_count++;
        callback = callback->next;
    }

    pthread_mutex_unlock(&c_interop_state.registry_mutex);

    return stats;
}
