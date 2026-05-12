// nova_engine_helper.c — Version helpers and mxArray conversion utilities
// Nova ML / MATLAB Engine Bridge
#include "nova_matlab_engine.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Internal struct definitions (needed for helper access)
// These match the internal structs in nova_matlab_engine.c
typedef struct {
  void *ep; // Engine* (opaque without MATLAB headers)
  char name[256];
  bool is_shared;
  bool alive;
} _EngInternal;

typedef struct {
  pthread_t thread;
  NovaFutureState state;
  void *result_ep; // Engine* opaque
  char error_msg[512];
  char opts[512];
  pthread_mutex_t mu;
  pthread_cond_t cv;
} _FutInternal;

// Return monotonic time in seconds
double nova_engine_monotonic_time(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// Get MATLAB version as integer (e.g. 2501 for R2025a)
int nova_matlab_get_version(void) {
#ifdef NOVA_MATLAB_NATIVE
  // Would use engEvalString + engGetVariable to get version info
  // Parsing "9.14.0" → 2501 (R2025a)
  return 2501; // stub for now even in native mode
#else
  return 0;
#endif
}

// Validate minimum version requirement
bool nova_matlab_validate_version(int ver) {
  return ver >= NOVA_MATLAB_MIN_VERSION;
}

// Return version as human-readable string
const char *nova_matlab_version_string(void) {
#ifdef NOVA_MATLAB_NATIVE
  return "R2025a (9.14)";
#else
  return "MATLAB not available (stub mode)";
#endif
}

// Return Nova Novaon
const char *nova_matlab_get_nova_version(void) { return NOVA_nova_N_VERSION; }

// Cancel a pending startup future
bool nova_matlab_cancel_startup(NovaMatlabFuture *fut) {
  if (!fut)
    return false;
  _FutInternal *f = (_FutInternal *)fut;
  pthread_mutex_lock(&f->mu);
  if (f->state == NOVA_FUTURE_PENDING || f->state == NOVA_FUTURE_RUNNING) {
    f->state = NOVA_FUTURE_CANCELLED;
    pthread_cond_signal(&f->cv);
    pthread_mutex_unlock(&f->mu);
    return true;
  }
  pthread_mutex_unlock(&f->mu);
  return false;
}

// Non-blocking check if startup future is done
NovaPollStatus nova_matlab_poll_startup(NovaMatlabFuture *fut) {
  if (!fut)
    return NOVA_POLL_ERROR;
  _FutInternal *f = (_FutInternal *)fut;
  pthread_mutex_lock(&f->mu);
  NovaFutureState s = f->state;
  pthread_mutex_unlock(&f->mu);
  if (s == NOVA_FUTURE_DONE)
    return NOVA_POLL_DONE;
  if (s == NOVA_FUTURE_ERROR || s == NOVA_FUTURE_CANCELLED)
    return NOVA_POLL_ERROR;
  return NOVA_POLL_PENDING;
}

// --- mxArray conversion (only available with MATLAB headers) ---
#ifdef NOVA_MATLAB_NATIVE
#include <matrix.h>

NovaMatlabValue *nova_mxarray_to_value(mxArray *mx) {
  if (!mx)
    return NULL;
  NovaMatlabValue *val = (NovaMatlabValue *)calloc(1, sizeof(NovaMatlabValue));
  if (!val)
    return NULL;

  if (mxIsDouble(mx) && !mxIsComplex(mx)) {
    size_t rows = mxGetM(mx);
    size_t cols = mxGetN(mx);
    if (rows == 1 && cols == 1) {
      val->type = NOVA_MVAL_DOUBLE;
      val->v.f64 = mxGetScalar(mx);
    } else {
      val->type = NOVA_MVAL_MATRIX_F64;
      val->v.mat_f64.rows = rows;
      val->v.mat_f64.cols = cols;
      size_t n = rows * cols;
      val->v.mat_f64.data = (double *)malloc(n * sizeof(double));
      if (val->v.mat_f64.data)
        memcpy(val->v.mat_f64.data, mxGetDoubles(mx), n * sizeof(double));
    }
  } else if (mxIsSingle(mx)) {
    size_t rows = mxGetM(mx);
    size_t cols = mxGetN(mx);
    val->type = NOVA_MVAL_MATRIX_F32;
    val->v.mat_f32.rows = rows;
    val->v.mat_f32.cols = cols;
    size_t n = rows * cols;
    val->v.mat_f32.data = (float *)malloc(n * sizeof(float));
    if (val->v.mat_f32.data)
      memcpy(val->v.mat_f32.data, mxGetSingles(mx), n * sizeof(float));
  } else if (mxIsInt32(mx)) {
    val->type = NOVA_MVAL_INT32;
    val->v.i32 = (int32_t)mxGetScalar(mx);
  } else if (mxIsInt64(mx)) {
    val->type = NOVA_MVAL_INT64;
    val->v.i64 = (int64_t)mxGetScalar(mx);
  } else if (mxIsLogical(mx)) {
    val->type = NOVA_MVAL_BOOL;
    val->v.b = (bool)mxGetScalar(mx);
  } else if (mxIsChar(mx)) {
    val->type = NOVA_MVAL_CHAR;
    char *buf = mxArrayToString(mx);
    val->v.str = buf ? strdup(buf) : strdup("");
    mxFree(buf);
  } else {
    val->type = NOVA_MVAL_VOID;
    val->v.opaque = NULL;
  }
  return val;
}

mxArray *nova_value_to_mxarray(const NovaMatlabValue *val) {
  if (!val)
    return NULL;
  switch (val->type) {
  case NOVA_MVAL_DOUBLE:
    return mxCreateDoubleScalar(val->v.f64);
  case NOVA_MVAL_SINGLE: {
    mxArray *mx = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
    *((float *)mxGetData(mx)) = val->v.f32;
    return mx;
  }
  case NOVA_MVAL_INT32:
    return mxCreateDoubleScalar((double)val->v.i32);
  case NOVA_MVAL_INT64:
    return mxCreateDoubleScalar((double)val->v.i64);
  case NOVA_MVAL_BOOL:
    return mxCreateLogicalScalar(val->v.b);
  case NOVA_MVAL_CHAR:
    return mxCreateString(val->v.str ? val->v.str : "");
  case NOVA_MVAL_MATRIX_F64: {
    size_t r = val->v.mat_f64.rows, c = val->v.mat_f64.cols;
    mxArray *mx = mxCreateDoubleMatrix(r, c, mxREAL);
    memcpy(mxGetDoubles(mx), val->v.mat_f64.data, r * c * sizeof(double));
    return mx;
  }
  case NOVA_MVAL_MATRIX_F32: {
    size_t r = val->v.mat_f32.rows, c = val->v.mat_f32.cols;
    mxArray *mx = mxCreateNumericMatrix(r, c, mxSINGLE_CLASS, mxREAL);
    memcpy(mxGetSingles(mx), val->v.mat_f32.data, r * c * sizeof(float));
    return mx;
  }
  default:
    return mxCreateDoubleScalar(0.0);
  }
}
#endif // NOVA_MATLAB_NATIVE
