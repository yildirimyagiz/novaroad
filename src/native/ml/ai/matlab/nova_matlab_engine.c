#include "nova_matlab_engine.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#ifdef NOVA_MATLAB_NATIVE
#include <engine.h>
#include <matrix.h>
#endif

// ============================================================
// Internal data structures
// ============================================================

struct NovaMatlabEngine_ {
#ifdef NOVA_MATLAB_NATIVE
    Engine*  ep;                    // MATLAB Engine pointer
#else
    void*    ep;                    // stub: unused
#endif
    char     name[256];             // session name
    bool     is_shared;             // shared vs. exclusive
    bool     alive;                 // alive flag
    pthread_mutex_t lock;           // thread safety
};

struct NovaMatlabFuture_ {
    pthread_t       thread;         // startup thread
    NovaFutureState state;          // current state
    NovaMatlabEngine* result;       // result when done
    char            error_msg[512]; // error message if failed
    char            opts[512];      // startup options
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};

struct NovaFevalFuture_ {
    pthread_t       thread;         // feval thread
    NovaFutureState state;          // current state
    NovaMatlabValue** results;      // output array
    int             nargout;        // number of outputs
    char            error_msg[512]; // error message if failed
    NovaMatlabEngine* eng;          // engine reference
    char            func_name[256]; // function name
    NovaMatlabValue** args;         // input args (owned by future)
    int             nargs;          // number of input args
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};

// ============================================================
// Global registry (thread-safe)
// ============================================================

#define NOVA_MAX_ENGINES 16

static struct {
    NovaMatlabEngine* engines[NOVA_MAX_ENGINES];
    int count;
    pthread_mutex_t lock;
    bool initialized;
} g_registry = {{0}, 0, PTHREAD_MUTEX_INITIALIZER, false};

// Forward declarations
static void registry_cleanup(void);
static NovaMatlabValue* __attribute__((unused)) mxarray_to_nova_value(const void* mx);
static void* __attribute__((unused)) nova_value_to_mxarray(const NovaMatlabValue* val);

// ============================================================
// Stub implementations (when NOVA_MATLAB_NATIVE not defined)
// ============================================================

#ifndef NOVA_MATLAB_NATIVE

static NovaMatlabEngine* stub_start(const char* option) {
    (void)option;
    return NULL;
}

static int __attribute__((unused)) stub_eval(void* ep, const char* expr) {
    (void)ep;
    (void)expr;
    return -1;
}

static int __attribute__((unused)) stub_put_variable(void* ep, const char* workspace, const char* name, void* mx) {
    (void)ep;
    (void)workspace;
    (void)name;
    (void)mx;
    return -1;
}

static void* __attribute__((unused)) stub_get_variable(void* ep, const char* workspace, const char* name) {
    (void)ep;
    (void)workspace;
    (void)name;
    return NULL;
}

static void stub_close(void* ep) {
    (void)ep;
}

#endif

// ============================================================
// Engine lifecycle
// ============================================================

NovaMatlabEngine* nova_matlab_start(const char* option, NovaMatlabError* err) {
    if (!err) return NULL;
    
    NovaMatlabEngine* eng = (NovaMatlabEngine*)malloc(sizeof(NovaMatlabEngine));
    if (!eng) {
        *err = NOVA_MATLAB_ERR_ALLOC;
        return NULL;
    }
    
    pthread_mutex_init(&eng->lock, NULL);
    strncpy(eng->name, option ? option : "default", sizeof(eng->name) - 1);
    eng->name[sizeof(eng->name) - 1] = '\0';
    eng->is_shared = (option && strstr(option, "-shared")) ? true : false;
    
#ifdef NOVA_MATLAB_NATIVE
    eng->ep = engOpen(option);
    if (!eng->ep) {
        *err = NOVA_MATLAB_ERR_INTERNAL;
        free(eng);
        return NULL;
    }
#else
    eng->ep = stub_start(option);
    if (!eng->ep) {
        *err = NOVA_MATLAB_ERR_INTERNAL;
        free(eng);
        return NULL;
    }
#endif
    
    eng->alive = true;
    *err = NOVA_MATLAB_OK;
    
    nova_engine_registry_register(eng);
    return eng;
}

NovaMatlabFuture* nova_matlab_start_async(const char* option, NovaMatlabError* err) {
    if (!err) return NULL;
    
    NovaMatlabFuture* fut = (NovaMatlabFuture*)malloc(sizeof(NovaMatlabFuture));
    if (!fut) {
        *err = NOVA_MATLAB_ERR_ALLOC;
        return NULL;
    }
    
    pthread_mutex_init(&fut->lock, NULL);
    pthread_cond_init(&fut->cond, NULL);
    fut->state = NOVA_FUTURE_PENDING;
    fut->result = NULL;
    strncpy(fut->opts, option ? option : "", sizeof(fut->opts) - 1);
    fut->opts[sizeof(fut->opts) - 1] = '\0';
    fut->error_msg[0] = '\0';
    
    // TODO: spawn thread to call nova_matlab_start
    *err = NOVA_MATLAB_OK;
    return fut;
}

void nova_matlab_quit(NovaMatlabEngine* eng) {
    if (!eng) return;
    
    pthread_mutex_lock(&eng->lock);
    eng->alive = false;
    
#ifdef NOVA_MATLAB_NATIVE
    if (eng->ep) {
        engClose(eng->ep);
        eng->ep = NULL;
    }
#else
    stub_close(eng->ep);
#endif
    
    pthread_mutex_unlock(&eng->lock);
    nova_engine_registry_deregister(eng);
    
    pthread_mutex_destroy(&eng->lock);
    free(eng);
}

void nova_matlab_disconnect(NovaMatlabEngine* eng) {
    if (!eng) return;
    nova_matlab_quit(eng);
}

bool nova_matlab_is_alive(NovaMatlabEngine* eng) {
    if (!eng) return false;
    pthread_mutex_lock(&eng->lock);
    bool alive = eng->alive;
    pthread_mutex_unlock(&eng->lock);
    return alive;
}

// ============================================================
// Eval
// ============================================================

NovaMatlabError nova_matlab_eval(NovaMatlabEngine* eng, const char* expr, bool use_feval_exc) {
    if (!eng || !expr) return NOVA_MATLAB_ERR_INTERNAL;
    
    (void)use_feval_exc;
    
    if (!nova_matlab_is_alive(eng)) {
        return NOVA_MATLAB_ERR_TERMINATED;
    }
    
#ifdef NOVA_MATLAB_NATIVE
    pthread_mutex_lock(&eng->lock);
    int ret = engEvalString(eng->ep, expr);
    pthread_mutex_unlock(&eng->lock);
    return (ret == 0) ? NOVA_MATLAB_OK : NOVA_MATLAB_ERR_INTERNAL;
#else
    return NOVA_MATLAB_ERR_INTERNAL;
#endif
}

NovaFevalFuture* nova_matlab_eval_async(NovaMatlabEngine* eng, const char* expr) {
    if (!eng || !expr) return NULL;
    
    // TODO: implement async eval
    return NULL;
}

// ============================================================
// feval (function evaluation)
// ============================================================

NovaMatlabError nova_matlab_feval(NovaMatlabEngine* eng,
                                  const char* func_name,
                                  int nargout,
                                  const NovaMatlabValue** args, // NOLINT
                                  int nargs,
                                  NovaMatlabValue** out) {
    (void)args; (void)nargs;
    if (!eng || !func_name || !out) return NOVA_MATLAB_ERR_INTERNAL;
    
    if (!nova_matlab_is_alive(eng)) {
        return NOVA_MATLAB_ERR_TERMINATED;
    }
    
#ifdef NOVA_MATLAB_NATIVE
    // Serialize args to MATLAB workspace
    for (int i = 0; i < nargs; i++) {
        char varname[64];
        snprintf(varname, sizeof(varname), "__nova_arg_%d", i);
        // TODO: convert args[i] to mxArray and put variable
    }
    
    // Build feval call string
    char call[512];
    snprintf(call, sizeof(call), "[");
    for (int i = 0; i < nargout; i++) {
        if (i > 0) strcat(call, ", ");
        strcat(call, "__nova_out_");
    }
    snprintf(call + strlen(call), sizeof(call) - strlen(call),
             "] = feval('%s'", func_name);
    
    for (int i = 0; i < nargs; i++) {
        sprintf(call + strlen(call), ", __nova_arg_%d", i);
    }
    strcat(call, ");");
    
    // Evaluate
    int ret = engEvalString(eng->ep, call);
    if (ret != 0) return NOVA_MATLAB_ERR_INTERNAL;
    
    // Retrieve results
    for (int i = 0; i < nargout; i++) {
        char varname[64];
        snprintf(varname, sizeof(varname), "__nova_out_%d", i);
        // TODO: get variable and convert to NovaMatlabValue
        out[i] = NULL;
    }
    
    return NOVA_MATLAB_OK;
#else
    (void)nargout;
    (void)args;
    (void)nargs;
    return NOVA_MATLAB_ERR_INTERNAL;
#endif
}

// ============================================================
// Workspace variables
// ============================================================

NovaMatlabError nova_matlab_put_variable(NovaMatlabEngine* eng,
                                         const char* workspace,
                                         const char* name,
                                         const NovaMatlabValue* val) {
    if (!eng || !name || !val) return NOVA_MATLAB_ERR_INTERNAL;
    
    if (!nova_matlab_is_alive(eng)) {
        return NOVA_MATLAB_ERR_TERMINATED;
    }
    
#ifdef NOVA_MATLAB_NATIVE
    void* mx = nova_value_to_mxarray(val);
    if (!mx) return NOVA_MATLAB_ERR_INTERNAL;
    
    pthread_mutex_lock(&eng->lock);
    int ret = engPutVariable(eng->ep, name, (mxArray*)mx);
    pthread_mutex_unlock(&eng->lock);
    
    // TODO: free mxArray
    
    return (ret == 0) ? NOVA_MATLAB_OK : NOVA_MATLAB_ERR_INTERNAL;
#else
    (void)workspace;
    return NOVA_MATLAB_ERR_INTERNAL;
#endif
}

NovaMatlabValue* nova_matlab_get_variable(NovaMatlabEngine* eng,
                                          const char* workspace,
                                          const char* name,
                                          NovaMatlabError* err) {
    if (!eng || !name) {
        if (err) *err = NOVA_MATLAB_ERR_INTERNAL;
        return NULL;
    }
    
    if (!nova_matlab_is_alive(eng)) {
        if (err) *err = NOVA_MATLAB_ERR_TERMINATED;
        return NULL;
    }
    
#ifdef NOVA_MATLAB_NATIVE
    pthread_mutex_lock(&eng->lock);
    void* mx = (void*)engGetVariable(eng->ep, name);
    pthread_mutex_unlock(&eng->lock);
    
    if (!mx) {
        if (err) *err = NOVA_MATLAB_ERR_NOT_FOUND;
        return NULL;
    }
    
    NovaMatlabValue* val = mxarray_to_nova_value(mx);
    if (err) *err = val ? NOVA_MATLAB_OK : NOVA_MATLAB_ERR_INTERNAL;
    
    // TODO: free mxArray
    
    return val;
#else
    (void)workspace;
    if (err) *err = NOVA_MATLAB_ERR_INTERNAL;
    return NULL;
#endif
}

// ============================================================
// Future — startup (MatlabFuture)
// ============================================================

NovaFutureState nova_future_state(NovaMatlabFuture* fut) {
    if (!fut) return NOVA_FUTURE_ERROR;
    pthread_mutex_lock(&fut->lock);
    NovaFutureState state = fut->state;
    pthread_mutex_unlock(&fut->lock);
    return state;
}

NovaMatlabEngine* nova_future_result(NovaMatlabFuture* fut, NovaMatlabError* err) {
    return nova_future_result_timeout(fut, -1.0, err);
}

NovaMatlabEngine* nova_future_result_timeout(NovaMatlabFuture* fut, double timeout_secs, NovaMatlabError* err) {
    if (!fut || !err) return NULL;
    
    pthread_mutex_lock(&fut->lock);
    
    // TODO: wait on condition variable with optional timeout
    
    NovaMatlabEngine* result = fut->result;
    if (fut->state == NOVA_FUTURE_ERROR) {
    (void)timeout_secs;
        *err = NOVA_MATLAB_ERR_INTERNAL;
    } else if (fut->state == NOVA_FUTURE_CANCELLED) {
        *err = NOVA_MATLAB_ERR_CANCELLED;
    } else if (fut->state == NOVA_FUTURE_DONE) {
        *err = NOVA_MATLAB_OK;
    } else {
        *err = NOVA_MATLAB_ERR_TIMEOUT;
    }
    
    pthread_mutex_unlock(&fut->lock);
    return result;
}

bool nova_future_cancel(NovaMatlabFuture* fut) {
    if (!fut) return false;
    pthread_mutex_lock(&fut->lock);
    if (fut->state == NOVA_FUTURE_PENDING) {
        fut->state = NOVA_FUTURE_CANCELLED;
        pthread_cond_signal(&fut->cond);
        pthread_mutex_unlock(&fut->lock);
        return true;
    }
    pthread_mutex_unlock(&fut->lock);
    return false;
}

bool nova_future_done(NovaMatlabFuture* fut) {
    if (!fut) return false;
    return nova_future_state(fut) == NOVA_FUTURE_DONE;
}

bool nova_future_cancelled(NovaMatlabFuture* fut) {
    if (!fut) return false;
    return nova_future_state(fut) == NOVA_FUTURE_CANCELLED;
}

void nova_future_free(NovaMatlabFuture* fut) {
    if (!fut) return;
    pthread_mutex_destroy(&fut->lock);
    pthread_cond_destroy(&fut->cond);
    free(fut);
}

// ============================================================
// Future — feval (FevalFuture)
// ============================================================

// ============================================================
// Engine registry (thread-safe global singleton)
// ============================================================

void nova_engine_registry_init(void) {
    if (g_registry.initialized) return;
    
    pthread_mutex_lock(&g_registry.lock);
    if (!g_registry.initialized) {
        g_registry.count = 0;
        memset(g_registry.engines, 0, sizeof(g_registry.engines));
        g_registry.initialized = true;
        atexit(registry_cleanup);
    }
    pthread_mutex_unlock(&g_registry.lock);
}

void nova_engine_registry_shutdown(void) {
    registry_cleanup();
}

void nova_engine_registry_register(NovaMatlabEngine* eng) {
    if (!eng) return;
    
    pthread_mutex_lock(&g_registry.lock);
    if (g_registry.count < NOVA_MAX_ENGINES) {
        g_registry.engines[g_registry.count++] = eng;
    }
    pthread_mutex_unlock(&g_registry.lock);
}

void nova_engine_registry_deregister(NovaMatlabEngine* eng) {
    if (!eng) return;
    
    pthread_mutex_lock(&g_registry.lock);
    for (int i = 0; i < g_registry.count; i++) {
        if (g_registry.engines[i] == eng) {
            // Swap with last element
            g_registry.engines[i] = g_registry.engines[--g_registry.count];
            break;
        }
    }
    pthread_mutex_unlock(&g_registry.lock);
}

static void registry_cleanup(void) {
    pthread_mutex_lock(&g_registry.lock);
    for (int i = 0; i < g_registry.count; i++) {
        if (g_registry.engines[i]) {
            g_registry.engines[i]->alive = false;
#ifdef NOVA_MATLAB_NATIVE
            if (g_registry.engines[i]->ep) {
                engClose(g_registry.engines[i]->ep);
            }
#endif
            pthread_mutex_destroy(&g_registry.engines[i]->lock);
            free(g_registry.engines[i]);
        }
    }
    g_registry.count = 0;
    pthread_mutex_unlock(&g_registry.lock);
}

// ============================================================
// Version helpers
// ============================================================

// ============================================================
// Value conversion helpers (stubs)
// ============================================================

static NovaMatlabValue* __attribute__((unused)) mxarray_to_nova_value(const void* mx) {
    (void)mx;
    // TODO: implement conversion from MATLAB mxArray
    return NULL;
}

static void* __attribute__((unused)) nova_value_to_mxarray(const NovaMatlabValue* val) {
    (void)val;
    // TODO: implement conversion to MATLAB mxArray
    return NULL;
}
