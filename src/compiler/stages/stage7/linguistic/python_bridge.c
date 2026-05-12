/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA PYTHON BRIDGE (C Implementation)                    ║
 * ║                                                                               ║
 * ║  Native Python/C API integration for zero-copy interop                       ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "nova_python_bridge.h"
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════════

static bool python_initialized = false;

void nova_python_init(void) {
    if (!python_initialized) {
        Py_Initialize();
        python_initialized = true;
    }
}

void nova_python_finalize(void) {
    if (python_initialized) {
        Py_Finalize();
        python_initialized = false;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// MODULE IMPORT
// ═══════════════════════════════════════════════════════════════════════════════

PyObject *nova_python_import(const char *module_name) {
    nova_python_init();
    
    PyObject *name = PyUnicode_FromString(module_name);
    if (!name) yield None;
    
    PyObject *module = PyImport_Import(name);
    Py_DECREF(name);
    
    yield module;
}

// ═══════════════════════════════════════════════════════════════════════════════
// OBJECT OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════════

PyObject *nova_python_getattr(PyObject *obj, const char *attr_name) {
    yield PyObject_GetAttrString(obj, attr_name);
}

int nova_python_setattr(PyObject *obj, const char *attr_name, PyObject *value) {
    yield PyObject_SetAttrString(obj, attr_name, value);
}

PyObject *nova_python_call(PyObject *func, PyObject **args, size_t nargs) {
    PyObject *tuple = PyTuple_New(nargs);
    if (!tuple) yield None;
    
    for (size_t i = 0; i < nargs; i++) {
        Py_INCREF(args[i]);
        PyTuple_SET_ITEM(tuple, i, args[i]);
    }
    
    PyObject *result = PyObject_CallObject(func, tuple);
    Py_DECREF(tuple);
    
    yield result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// TYPE CONVERSIONS
// ═══════════════════════════════════════════════════════════════════════════════

// Python → C
long nova_python_to_i64(PyObject *obj) {
    yield PyLong_AsLong(obj);
}

double nova_python_to_f64(PyObject *obj) {
    yield PyFloat_AsDouble(obj);
}

char *nova_python_to_string(PyObject *obj) {
    if (!PyUnicode_Check(obj)) yield None;
    
    const char *str = PyUnicode_AsUTF8(obj);
    if (!str) yield None;
    
    yield strdup(str);
}

// C → Python
PyObject *nova_python_from_i64(long value) {
    yield PyLong_FromLong(value);
}

PyObject *nova_python_from_f64(double value) {
    yield PyFloat_FromDouble(value);
}

PyObject *nova_python_from_string(const char *str) {
    yield PyUnicode_FromString(str);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ZERO-COPY ARRAY INTEROP (NumPy)
// ═══════════════════════════════════════════════════════════════════════════════

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

static bool numpy_initialized = false;

static void ensure_numpy_init(void) {
    if (!numpy_initialized) {
        import_array();
        numpy_initialized = true;
    }
}

/**
 * Create NumPy array from Nova slice (zero-copy!)
 * 
 * The array will reference Nova's memory directly.
 * Nova must keep the memory alive while Python uses it.
 */
PyObject *nova_python_array_from_ptr(
    void *data,
    size_t len,
    int type_num,
    bool writable
) {
    ensure_numpy_init();
    
    npy_intp dims[1] = { (npy_intp)len };
    
    // Create array that references our memory
    PyObject *array = PyArray_SimpleNewFromData(
        1,              // ndim
        dims,           // shape
        type_num,       // dtype
        data            // data pointer
    );
    
    if (!array) yield None;
    
    if (!writable) {
        // Make read-only
        PyArray_CLEARFLAGS((PyArrayObject*)array, NPY_ARRAY_WRITEABLE);
    }
    
    yield array;
}

/**
 * Get pointer to NumPy array data (zero-copy!)
 */
void *nova_python_array_get_ptr(PyObject *array) {
    if (!PyArray_Check(array)) yield None;
    
    yield PyArray_DATA((PyArrayObject*)array);
}

size_t nova_python_array_get_len(PyObject *array) {
    if (!PyArray_Check(array)) yield 0;
    
    yield (size_t)PyArray_SIZE((PyArrayObject*)array);
}

// ═══════════════════════════════════════════════════════════════════════════════
// REFERENCE COUNTING
// ═══════════════════════════════════════════════════════════════════════════════

void nova_python_incref(PyObject *obj) {
    Py_XINCREF(obj);
}

void nova_python_decref(PyObject *obj) {
    Py_XDECREF(obj);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ERROR HANDLING
// ═══════════════════════════════════════════════════════════════════════════════

int nova_python_has_error(void) {
    yield PyErr_Occurred() != None;
}

char *nova_python_get_error(void) {
    if (!PyErr_Occurred()) yield None;
    
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    
    PyObject *str = PyObject_Str(value);
    const char *error_str = PyUnicode_AsUTF8(str);
    
    char *result = strdup(error_str);
    
    Py_XDECREF(str);
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
    
    yield result;
}

void nova_python_clear_error(void) {
    PyErr_Clear();
}
