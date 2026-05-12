/**
 * @file py_interop.c
 * @brief Python language interop using Python C API
 */

#include "runtime/ffi.h"
#include "std/alloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

// Check if Python is available
#ifdef HAVE_PYTHON
#include <Python.h>
#define PYTHON_ENABLED 1
#else
#define PYTHON_ENABLED 0
#warning "Python not available - Python interop will be disabled"
#endif

// Python object wrapper
typedef struct nova_py_object {
    PyObject *py_obj;
    atomic_int ref_count;
} nova_py_object_t;

// Python function wrapper
typedef struct nova_py_function {
    PyObject *py_func;
    char *name;
    nova_py_signature_t signature;
} nova_py_function_t;

// Python module registry
typedef struct py_module_entry {
    char *name;
    PyObject *module;
    struct py_module_entry *next;
} py_module_entry_t;

// Python class registry
typedef struct py_class_entry {
    char *name;
    PyObject *py_class;
    struct py_class_entry *next;
} py_class_entry_t;

// Global Python state
static struct {
    atomic_bool initialized;
    PyThreadState *main_thread_state;
    py_module_entry_t *module_registry;
    py_class_entry_t *class_registry;
    pthread_mutex_t registry_mutex;

    // Statistics
    atomic_size_t total_calls;
    atomic_size_t object_conversions;
    atomic_size_t errors;
} python_state;

static nova_py_object_t *nova_py_object_wrap(PyObject *py_obj)
{
    if (!py_obj) return NULL;

    nova_py_object_t *obj = nova_alloc(sizeof(nova_py_object_t));
    if (!obj) {
        Py_DECREF(py_obj);
        return NULL;
    }

    obj->py_obj = py_obj; // Already has ref from caller
    atomic_init(&obj->ref_count, 1);

    atomic_fetch_add(&python_state.object_conversions, 1);
    return obj;
}

nova_py_object_t *nova_py_object_retain(nova_py_object_t *obj)
{
    if (obj) {
        atomic_fetch_add(&obj->ref_count, 1);
    }
    return obj;
}

void nova_py_object_release(nova_py_object_t *obj)
{
    if (!obj) return;

    if (atomic_fetch_sub(&obj->ref_count, 1) == 1) {
#if PYTHON_ENABLED
        Py_DECREF(obj->py_obj);
#endif
        nova_free(obj);
    }
}

nova_value_t nova_py_object_to_nova_value(nova_py_object_t *obj)
{
    nova_value_t value = {0};
    value.type = NOVA_TYPE_NULL;

    if (!obj || !obj->py_obj) return value;

#if PYTHON_ENABLED
    // Convert Python object to Nova value
    if (PyLong_Check(obj->py_obj)) {
        value.type = NOVA_TYPE_INT;
        value.as.integer = PyLong_AsLongLong(obj->py_obj);
    } else if (PyFloat_Check(obj->py_obj)) {
        value.type = NOVA_TYPE_FLOAT;
        value.as.number = PyFloat_AsDouble(obj->py_obj);
    } else if (PyUnicode_Check(obj->py_obj)) {
        // For strings, we'd need to convert to Nova string
        value.type = NOVA_TYPE_POINTER;
        value.as.pointer = (void *)obj; // Keep as Python object wrapper
    } else if (PyBool_Check(obj->py_obj)) {
        value.type = NOVA_TYPE_INT;
        value.as.integer = PyObject_IsTrue(obj->py_obj);
    } else {
        // For complex objects, keep as Python wrapper
        value.type = NOVA_TYPE_POINTER;
        value.as.pointer = (void *)nova_py_object_retain(obj);
    }
#endif

    return value;
}

nova_py_object_t *nova_nova_value_to_py_object(nova_value_t *value)
{
    if (!value) return NULL;

#if PYTHON_ENABLED
    PyObject *py_obj = NULL;

    switch (value->type) {
        case NOVA_TYPE_INT:
            py_obj = PyLong_FromLongLong(value->as.integer);
            break;
        case NOVA_TYPE_FLOAT:
            py_obj = PyFloat_FromDouble(value->as.number);
            break;
        case NOVA_TYPE_BOOL:
            py_obj = value->as.boolean ? Py_True : Py_False;
            Py_INCREF(py_obj);
            break;
        case NOVA_TYPE_NULL:
            Py_INCREF(Py_None);
            py_obj = Py_None;
            break;
        default:
            // For complex types, return None
            Py_INCREF(Py_None);
            py_obj = Py_None;
            break;
    }

    return nova_py_object_wrap(py_obj);
#else
    (void)value;
    return NULL;
#endif
}

nova_py_module_t *nova_py_module_import(const char *name)
{
    if (!name) return NULL;

#if PYTHON_ENABLED
    PyObject *module = PyImport_ImportModule(name);
    if (!module) {
        PyErr_Print();
        return NULL;
    }

    nova_py_module_t *nova_module = nova_alloc(sizeof(nova_py_module_t));
    if (!nova_module) {
        Py_DECREF(module);
        return NULL;
    }

    nova_module->name = strdup(name);
    nova_module->py_module = module;

    // Register module
    py_module_entry_t *entry = nova_alloc(sizeof(py_module_entry_t));
    if (entry) {
        entry->name = strdup(name);
        entry->module = module;
        Py_INCREF(module); // Extra ref for registry

        pthread_mutex_lock(&python_state.registry_mutex);
        entry->next = python_state.module_registry;
        python_state.module_registry = entry;
        pthread_mutex_unlock(&python_state.registry_mutex);
    }

    return nova_module;
#else
    (void)name;
    return NULL;
#endif
}

void nova_py_module_destroy(nova_py_module_t *module)
{
    if (!module) return;

#if PYTHON_ENABLED
    Py_DECREF(module->py_module);
#endif

    nova_free(module->name);
    nova_free(module);
}

nova_py_function_t *nova_py_module_get_function(nova_py_module_t *module, const char *name)
{
    if (!module || !name) return NULL;

#if PYTHON_ENABLED
    PyObject *func = PyObject_GetAttrString(module->py_module, name);
    if (!func) {
        PyErr_Print();
        return NULL;
    }

    if (!PyCallable_Check(func)) {
        Py_DECREF(func);
        return NULL;
    }

    nova_py_function_t *nova_func = nova_alloc(sizeof(nova_py_function_t));
    if (!nova_func) {
        Py_DECREF(func);
        return NULL;
    }

    nova_func->name = strdup(name);
    nova_func->py_func = func;

    return nova_func;
#else
    (void)module;
    (void)name;
    return NULL;
#endif
}

nova_py_class_t *nova_py_module_get_class(nova_py_module_t *module, const char *name)
{
    if (!module || !name) return NULL;

#if PYTHON_ENABLED
    PyObject *cls = PyObject_GetAttrString(module->py_module, name);
    if (!cls) {
        PyErr_Print();
        return NULL;
    }

    if (!PyType_Check(cls)) {
        Py_DECREF(cls);
        return NULL;
    }

    nova_py_class_t *nova_class = nova_alloc(sizeof(nova_py_class_t));
    if (!nova_class) {
        Py_DECREF(cls);
        return NULL;
    }

    nova_class->name = strdup(name);
    nova_class->py_class = cls;

    // Register class
    py_class_entry_t *entry = nova_alloc(sizeof(py_class_entry_t));
    if (entry) {
        entry->name = strdup(name);
        entry->py_class = cls;
        Py_INCREF(cls);

        pthread_mutex_lock(&python_state.registry_mutex);
        entry->next = python_state.class_registry;
        python_state.class_registry = entry;
        pthread_mutex_unlock(&python_state.registry_mutex);
    }

    return nova_class;
#else
    (void)module;
    (void)name;
    return NULL;
#endif
}

void nova_py_function_destroy(nova_py_function_t *func)
{
    if (!func) return;

#if PYTHON_ENABLED
    Py_DECREF(func->py_func);
#endif

    nova_free(func->name);
    nova_free(func);
}

nova_py_object_t *nova_py_function_call(nova_py_function_t *func, nova_py_object_t **args, size_t arg_count)
{
    if (!func) return NULL;

    atomic_fetch_add(&python_state.total_calls, 1);

#if PYTHON_ENABLED
    PyObject *py_args = PyTuple_New(arg_count);
    if (!py_args) return NULL;

    // Convert arguments
    for (size_t i = 0; i < arg_count; i++) {
        if (args[i] && args[i]->py_obj) {
            Py_INCREF(args[i]->py_obj);
            PyTuple_SetItem(py_args, i, args[i]->py_obj);
        } else {
            Py_INCREF(Py_None);
            PyTuple_SetItem(py_args, i, Py_None);
        }
    }

    // Call function
    PyObject *result = PyObject_CallObject(func->py_func, py_args);
    Py_DECREF(py_args);

    if (!result) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
        return NULL;
    }

    return nova_py_object_wrap(result);
#else
    (void)func;
    (void)args;
    (void)arg_count;
    return NULL;
#endif
}

nova_py_object_t *nova_py_class_instantiate(nova_py_class_t *cls, nova_py_object_t **args, size_t arg_count)
{
    if (!cls) return NULL;

#if PYTHON_ENABLED
    PyObject *py_args = PyTuple_New(arg_count);
    if (!py_args) return NULL;

    // Convert arguments
    for (size_t i = 0; i < arg_count; i++) {
        if (args[i] && args[i]->py_obj) {
            Py_INCREF(args[i]->py_obj);
            PyTuple_SetItem(py_args, i, args[i]->py_obj);
        } else {
            Py_INCREF(Py_None);
            PyTuple_SetItem(py_args, i, Py_None);
        }
    }

    // Instantiate class
    PyObject *instance = PyObject_CallObject(cls->py_class, py_args);
    Py_DECREF(py_args);

    if (!instance) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
        return NULL;
    }

    return nova_py_object_wrap(instance);
#else
    (void)cls;
    (void)args;
    (void)arg_count;
    return NULL;
#endif
}

void nova_py_class_destroy(nova_py_class_t *cls)
{
    if (!cls) return;

#if PYTHON_ENABLED
    Py_DECREF(cls->py_class);
#endif

    nova_free(cls->name);
    nova_free(cls);
}

int nova_py_object_set_attr(nova_py_object_t *obj, const char *name, nova_py_object_t *value)
{
    if (!obj || !name || !value) return -1;

#if PYTHON_ENABLED
    int result = PyObject_SetAttrString(obj->py_obj, name, value->py_obj);
    if (result == -1) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
    }
    return result;
#else
    (void)obj;
    (void)name;
    (void)value;
    return -1;
#endif
}

nova_py_object_t *nova_py_object_get_attr(nova_py_object_t *obj, const char *name)
{
    if (!obj || !name) return NULL;

#if PYTHON_ENABLED
    PyObject *attr = PyObject_GetAttrString(obj->py_obj, name);
    if (!attr) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
        return NULL;
    }

    return nova_py_object_wrap(attr);
#else
    (void)obj;
    (void)name;
    return NULL;
#endif
}

nova_py_object_t *nova_py_eval_string(const char *code, nova_py_object_t *globals, nova_py_object_t *locals)
{
    if (!code) return NULL;

#if PYTHON_ENABLED
    PyObject *py_globals = globals && globals->py_obj ? globals->py_obj : PyEval_GetGlobals();
    PyObject *py_locals = locals && locals->py_obj ? locals->py_obj : PyEval_GetLocals();

    PyObject *result = PyRun_String(code, Py_eval_input, py_globals, py_locals);
    if (!result) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
        return NULL;
    }

    return nova_py_object_wrap(result);
#else
    (void)code;
    (void)globals;
    (void)locals;
    return NULL;
#endif
}

nova_py_object_t *nova_py_exec_file(const char *filename)
{
    if (!filename) return NULL;

#if PYTHON_ENABLED
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;

    PyObject *result = PyRun_File(fp, filename, Py_file_input, PyEval_GetGlobals(), PyEval_GetLocals());
    fclose(fp);

    if (!result) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
        return NULL;
    }

    return nova_py_object_wrap(result);
#else
    (void)filename;
    return NULL;
#endif
}

int nova_py_add_module_path(const char *path)
{
    if (!path) return -1;

#if PYTHON_ENABLED
    PyObject *sys_path = PySys_GetObject("path");
    if (!sys_path) return -1;

    PyObject *py_path = PyUnicode_FromString(path);
    if (!py_path) return -1;

    int result = PyList_Append(sys_path, py_path);
    Py_DECREF(py_path);

    if (result == -1) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
    }

    return result;
#else
    (void)path;
    return -1;
#endif
}

void nova_py_interop_init(void)
{
    if (atomic_load(&python_state.initialized)) return;

    memset(&python_state, 0, sizeof(python_state));
    pthread_mutex_init(&python_state.registry_mutex, NULL);

    atomic_init(&python_state.total_calls, 0);
    atomic_init(&python_state.object_conversions, 0);
    atomic_init(&python_state.errors, 0);

#if PYTHON_ENABLED
    // Initialize Python
    Py_Initialize();

    // Get main thread state
    python_state.main_thread_state = PyThreadState_Get();

    // Add current directory to path
    nova_py_add_module_path(".");
#endif

    atomic_store(&python_state.initialized, true);
}

void nova_py_interop_shutdown(void)
{
    if (!atomic_load(&python_state.initialized)) return;

#if PYTHON_ENABLED
    // Clean up Python objects
    py_module_entry_t *module_entry = python_state.module_registry;
    while (module_entry) {
        py_module_entry_t *next = module_entry->next;
        Py_DECREF(module_entry->module);
        nova_free(module_entry->name);
        nova_free(module_entry);
        module_entry = next;
    }

    py_class_entry_t *class_entry = python_state.class_registry;
    while (class_entry) {
        py_class_entry_t *next = class_entry->next;
        Py_DECREF(class_entry->py_class);
        nova_free(class_entry->name);
        nova_free(class_entry);
        class_entry = next;
    }

    // Finalize Python
    Py_Finalize();
#endif

    pthread_mutex_destroy(&python_state.registry_mutex);
    atomic_store(&python_state.initialized, false);
}

nova_py_stats_t nova_py_stats(void)
{
    nova_py_stats_t stats = {0};

    stats.total_calls = atomic_load(&python_state.total_calls);
    stats.object_conversions = atomic_load(&python_state.object_conversions);
    stats.errors = atomic_load(&python_state.errors);

    // Count registered modules and classes
    pthread_mutex_lock(&python_state.registry_mutex);

    py_module_entry_t *module = python_state.module_registry;
    while (module) {
        stats.loaded_modules++;
        module = module->next;
    }

    py_class_entry_t *cls = python_state.class_registry;
    while (cls) {
        stats.loaded_classes++;
        cls = cls->next;
    }

    pthread_mutex_unlock(&python_state.registry_mutex);

    return stats;
}

int nova_py_run_simple_string(const char *code)
{
    if (!code) return -1;

#if PYTHON_ENABLED
    int result = PyRun_SimpleString(code);
    if (result == -1) {
        PyErr_Print();
        atomic_fetch_add(&python_state.errors, 1);
    }
    return result;
#else
    (void)code;
    return -1;
#endif
}
