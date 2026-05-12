#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================
// Version requirements
// ============================================================
#define NOVA_MATLAB_MIN_VERSION 2501 // R2025a = 25.1
#define NOVA_nova_N_VERSION "1.0.0"

// ============================================================
// Opaque handles
// ============================================================
typedef struct NovaMatlabEngine_ NovaMatlabEngine;   // live session
typedef struct NovaMatlabFuture_ NovaMatlabFuture;   // async startup
typedef struct NovaFevalFuture_ NovaFevalFuture;     // async feval
typedef struct NovaMatlabValue_ NovaMatlabValue;     // boxed return value
typedef struct NovaEngineSession_ NovaEngineSession; // shared-session record

// ============================================================
// Error codes  (mirrors EngineError / EngineTerminatedError)
// ============================================================
typedef enum {
  NOVA_MATLAB_OK = 0,
  NOVA_MATLAB_ERR_TERMINATED,
  NOVA_MATLAB_ERR_REJECTED,
  NOVA_MATLAB_ERR_CANCELLED,
  NOVA_MATLAB_ERR_TIMEOUT,
  NOVA_MATLAB_ERR_VERSION,
  NOVA_MATLAB_ERR_NOT_FOUND,
  NOVA_MATLAB_ERR_ALLOC,
  NOVA_MATLAB_ERR_INTERNAL,
} NovaMatlabError;

// ============================================================
// MatlabValue type tag
// ============================================================
typedef enum {
  NOVA_MVAL_DOUBLE = 0,
  NOVA_MVAL_SINGLE,
  NOVA_MVAL_INT32,
  NOVA_MVAL_INT64,
  NOVA_MVAL_BOOL,
  NOVA_MVAL_CHAR,
  NOVA_MVAL_STRUCT,
  NOVA_MVAL_CELL,
  NOVA_MVAL_MATRIX_F64, // flat row-major double matrix
  NOVA_MVAL_MATRIX_F32,
  NOVA_MVAL_VOID,
} NovaMatlabValueType;

// ============================================================
// MatlabValue — boxed return from feval/getvar
// ============================================================
typedef struct NovaMatlabValue_ {
  NovaMatlabValueType type;
  union {
    double f64;
    float f32;
    int32_t i32;
    int64_t i64;
    bool b;
    char *str; // heap-allocated, free with nova_mval_free
    struct {
      double *data; // row-major
      size_t rows;
      size_t cols;
    } mat_f64;
    struct {
      float *data;
      size_t rows;
      size_t cols;
    } mat_f32;
    void *opaque; // struct / cell — implementation-defined
  } v;
} NovaMatlabValue;

// ============================================================
// FutureState  (mirrors FutureState enum in zn)
// ============================================================
typedef enum {
  NOVA_FUTURE_PENDING = 0,
  NOVA_FUTURE_RUNNING,
  NOVA_FUTURE_DONE,
  NOVA_FUTURE_CANCELLED,
  NOVA_FUTURE_ERROR,
} NovaFutureState;

// ============================================================
// PollStatus
// ============================================================
typedef enum {
  NOVA_POLL_PENDING = 0,
  NOVA_POLL_DONE,
  NOVA_POLL_ERROR,
} NovaPollStatus;

// ============================================================
// Shared session record
// ============================================================
typedef struct NovaEngineSession_ {
  char name[256]; // shared-session name
  uint32_t pid;   // owner process id
  bool is_shared;
} NovaEngineSession;

// ============================================================
// Engine lifecycle
// ============================================================
NovaMatlabEngine *nova_matlab_start(const char *option, NovaMatlabError *err);
NovaMatlabFuture *nova_matlab_start_async(const char *option,
                                          NovaMatlabError *err);
NovaMatlabEngine *nova_matlab_connect(const char *name, NovaMatlabError *err);
NovaMatlabEngine *nova_matlab_connect_any(NovaMatlabError *err);
int nova_matlab_find(NovaEngineSession *out, int max_sessions);
void nova_matlab_quit(NovaMatlabEngine *eng);
void nova_matlab_disconnect(NovaMatlabEngine *eng);
bool nova_matlab_is_alive(NovaMatlabEngine *eng);

// ============================================================
// Eval
// ============================================================
NovaMatlabError nova_matlab_eval(NovaMatlabEngine *eng, const char *expr,
                                 bool use_feval_exc);
NovaFevalFuture *nova_matlab_eval_async(NovaMatlabEngine *eng,
                                        const char *expr);

// ============================================================
// feval  (function evaluation)
// ============================================================
// nargout: number of return values requested
// args:    array of NovaMatlabValue* (caller owns)
// out:     caller-provided array of NovaMatlabValue* (nargout elements)
NovaMatlabError nova_matlab_feval(NovaMatlabEngine *eng, const char *func_name,
                                  int nargout, const NovaMatlabValue **args,
                                  int nargs, NovaMatlabValue **out);

NovaFevalFuture *nova_matlab_feval_async(NovaMatlabEngine *eng,
                                         const char *func_name, int nargout,
                                         const NovaMatlabValue **args,
                                         int nargs);

// ============================================================
// Workspace variables
// ============================================================
NovaMatlabError nova_matlab_put_variable(NovaMatlabEngine *eng,
                                         const char *workspace,
                                         const char *name,
                                         const NovaMatlabValue *val);
NovaMatlabValue *nova_matlab_get_variable(NovaMatlabEngine *eng,
                                          const char *workspace,
                                          const char *name,
                                          NovaMatlabError *err);

// ============================================================
// Value constructors / destructors
// ============================================================
NovaMatlabValue *nova_mval_double(double v);
NovaMatlabValue *nova_mval_float(float v);
NovaMatlabValue *nova_mval_int32(int32_t v);
NovaMatlabValue *nova_mval_int64(int64_t v);
NovaMatlabValue *nova_mval_bool(bool v);
NovaMatlabValue *nova_mval_string(const char *s);
NovaMatlabValue *nova_mval_matrix_f64(const double *data, size_t rows,
                                      size_t cols);
NovaMatlabValue *nova_mval_matrix_f32(const float *data, size_t rows,
                                      size_t cols);
void nova_mval_free(NovaMatlabValue *val);

// ============================================================
// Future — startup (MatlabFuture)
// ============================================================
NovaFutureState nova_future_state(NovaMatlabFuture *fut);
NovaMatlabEngine *nova_future_result(NovaMatlabFuture *fut,
                                     NovaMatlabError *err);
NovaMatlabEngine *nova_future_result_timeout(NovaMatlabFuture *fut,
                                             double timeout_secs,
                                             NovaMatlabError *err);
bool nova_future_cancel(NovaMatlabFuture *fut);
bool nova_future_done(NovaMatlabFuture *fut);
bool nova_future_cancelled(NovaMatlabFuture *fut);
void nova_future_free(NovaMatlabFuture *fut);

// ============================================================
// Future — feval (FevalFuture)
// ============================================================
NovaFutureState nova_feval_future_state(NovaFevalFuture *fut);
NovaMatlabError nova_feval_future_result(NovaFevalFuture *fut,
                                         NovaMatlabValue **out, int nargout);
NovaMatlabError nova_feval_future_result_timeout(NovaFevalFuture *fut,
                                                 double timeout_secs,
                                                 NovaMatlabValue **out,
                                                 int nargout);
bool nova_feval_future_cancel(NovaFevalFuture *fut);
bool nova_feval_future_done(NovaFevalFuture *fut);
void nova_feval_future_free(NovaFevalFuture *fut);

// ============================================================
// Engine registry  (global singleton)
// ============================================================
void nova_engine_registry_init(void);
void nova_engine_registry_shutdown(void); // called at process exit
void nova_engine_registry_register(NovaMatlabEngine *eng);
void nova_engine_registry_deregister(NovaMatlabEngine *eng);

// ============================================================
// Version helpers
// ============================================================
int nova_matlab_get_version(void); // returns e.g. 2501 for R2025a
bool nova_matlab_validate_version(int ver);
const char *nova_matlab_version_string(void);
