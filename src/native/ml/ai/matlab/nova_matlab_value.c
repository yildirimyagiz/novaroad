#include "nova_matlab_engine.h"
#include <stdlib.h>
#include <string.h>

// ============================================================
// Value constructors — each returns heap-allocated NovaMatlabValue
// ============================================================

NovaMatlabValue* nova_mval_double(double v) {
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    val->type = NOVA_MVAL_DOUBLE;
    val->v.f64 = v;
    return val;
}

NovaMatlabValue* nova_mval_float(float v) {
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    val->type = NOVA_MVAL_SINGLE;
    val->v.f32 = v;
    return val;
}

NovaMatlabValue* nova_mval_int32(int32_t v) {
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    val->type = NOVA_MVAL_INT32;
    val->v.i32 = v;
    return val;
}

NovaMatlabValue* nova_mval_int64(int64_t v) {
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    val->type = NOVA_MVAL_INT64;
    val->v.i64 = v;
    return val;
}

NovaMatlabValue* nova_mval_bool(bool v) {
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    val->type = NOVA_MVAL_BOOL;
    val->v.b = v;
    return val;
}

NovaMatlabValue* nova_mval_string(const char* s) {
    if (!s) return NULL;
    
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    val->type = NOVA_MVAL_CHAR;
    val->v.str = (char*)malloc(strlen(s) + 1);
    if (!val->v.str) {
        free(val);
        return NULL;
    }
    strcpy(val->v.str, s);
    return val;
}

NovaMatlabValue* nova_mval_matrix_f64(const double* data, size_t rows, size_t cols) {
    if (!data || rows == 0 || cols == 0) return NULL;
    
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    size_t nelems = rows * cols;
    double* copy = (double*)malloc(nelems * sizeof(double));
    if (!copy) {
        free(val);
        return NULL;
    }
    
    memcpy(copy, data, nelems * sizeof(double));
    
    val->type = NOVA_MVAL_MATRIX_F64;
    val->v.mat_f64.data = copy;
    val->v.mat_f64.rows = rows;
    val->v.mat_f64.cols = cols;
    return val;
}

NovaMatlabValue* nova_mval_matrix_f32(const float* data, size_t rows, size_t cols) {
    if (!data || rows == 0 || cols == 0) return NULL;
    
    NovaMatlabValue* val = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
    if (!val) return NULL;
    
    size_t nelems = rows * cols;
    float* copy = (float*)malloc(nelems * sizeof(float));
    if (!copy) {
        free(val);
        return NULL;
    }
    
    memcpy(copy, data, nelems * sizeof(float));
    
    val->type = NOVA_MVAL_MATRIX_F32;
    val->v.mat_f32.data = copy;
    val->v.mat_f32.rows = rows;
    val->v.mat_f32.cols = cols;
    return val;
}

// ============================================================
// Value destructor
// ============================================================

void nova_mval_free(NovaMatlabValue* val) {
    if (!val) return;
    
    switch (val->type) {
        case NOVA_MVAL_CHAR:
            if (val->v.str) {
                free(val->v.str);
            }
            break;
        case NOVA_MVAL_MATRIX_F64:
            if (val->v.mat_f64.data) {
                free(val->v.mat_f64.data);
            }
            break;
        case NOVA_MVAL_MATRIX_F32:
            if (val->v.mat_f32.data) {
                free(val->v.mat_f32.data);
            }
            break;
        case NOVA_MVAL_STRUCT:
        case NOVA_MVAL_CELL:
            // TODO: deep free for complex types
            if (val->v.opaque) {
                free(val->v.opaque);
            }
            break;
        default:
            // scalar types have no heap allocation
            break;
    }
    
    free(val);
}
