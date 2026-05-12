/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA PYTHON BRIDGE (Header)                              ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_PYTHON_BRIDGE_H
#define NOVA_PYTHON_BRIDGE_H

#include <stddef.h>
#include <stdbool.h>

// Forward declare Python types
typedef struct _object PyObject;

// ═══════════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════════

void nova_python_init(void);
void nova_python_finalize(void);

// ═══════════════════════════════════════════════════════════════════════════════
// MODULE OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════════

PyObject *nova_python_import(const char *module_name);

// ═══════════════════════════════════════════════════════════════════════════════
// OBJECT OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════════

PyObject *nova_python_getattr(PyObject *obj, const char *attr_name);
int nova_python_setattr(PyObject *obj, const char *attr_name, PyObject *value);
PyObject *nova_python_call(PyObject *func, PyObject **args, size_t nargs);

// ═══════════════════════════════════════════════════════════════════════════════
// TYPE CONVERSIONS
// ═══════════════════════════════════════════════════════════════════════════════

long nova_python_to_i64(PyObject *obj);
double nova_python_to_f64(PyObject *obj);
char *nova_python_to_string(PyObject *obj);

PyObject *nova_python_from_i64(long value);
PyObject *nova_python_from_f64(double value);
PyObject *nova_python_from_string(const char *str);

// ═══════════════════════════════════════════════════════════════════════════════
// ARRAY INTEROP (NumPy)
// ═══════════════════════════════════════════════════════════════════════════════

PyObject *nova_python_array_from_ptr(void *data, size_t len, int type_num, bool writable);
void *nova_python_array_get_ptr(PyObject *array);
size_t nova_python_array_get_len(PyObject *array);

// ═══════════════════════════════════════════════════════════════════════════════
// REFERENCE COUNTING
// ═══════════════════════════════════════════════════════════════════════════════

void nova_python_incref(PyObject *obj);
void nova_python_decref(PyObject *obj);

// ═══════════════════════════════════════════════════════════════════════════════
// ERROR HANDLING
// ═══════════════════════════════════════════════════════════════════════════════

int nova_python_has_error(void);
char *nova_python_get_error(void);
void nova_python_clear_error(void);

#endif // NOVA_PYTHON_BRIDGE_H
