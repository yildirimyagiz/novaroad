// nova_feval_future.c — Async feval future implementation
// Nova ML / MATLAB Engine Bridge
#include "nova_matlab_engine.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

// Internal feval work struct
typedef struct FevalWork_ {
    NovaMatlabEngine*  eng;
    char               func[256];
    int                nargout;
    NovaMatlabValue**  args;        // caller-owned copies
    int                nargs;
    NovaMatlabValue**  results;     // filled by worker
    NovaMatlabError    error;
    NovaFutureState    state;
    pthread_mutex_t    mu;
    pthread_cond_t     cv;
} FevalWork;

// Worker thread: calls nova_matlab_feval synchronously
static void* feval_worker(void* arg) {
    FevalWork* w = (FevalWork*)arg;

    pthread_mutex_lock(&w->mu);
    w->state = NOVA_FUTURE_RUNNING;
    pthread_mutex_unlock(&w->mu);

    // Allocate result array
    w->results = (NovaMatlabValue**)calloc((size_t)w->nargout, sizeof(NovaMatlabValue*));
    NovaMatlabError rc = NOVA_MATLAB_ERR_INTERNAL;
    if (w->results) {
        rc = nova_matlab_feval(w->eng, w->func, w->nargout,
                               (const NovaMatlabValue**)w->args, w->nargs,
                               w->results);
    }

    pthread_mutex_lock(&w->mu);
    w->error = rc;
    w->state = (rc == NOVA_MATLAB_OK) ? NOVA_FUTURE_DONE : NOVA_FUTURE_ERROR;
    pthread_cond_broadcast(&w->cv);
    pthread_mutex_unlock(&w->mu);
    return NULL;
}

// Launch async feval — returns opaque future handle
NovaFevalFuture* nova_matlab_feval_async(NovaMatlabEngine*       eng,
                                          const char*             func,
                                          int                     nargout,
                                          const NovaMatlabValue** args,
                                          int                     nargs) {
    FevalWork* w = (FevalWork*)calloc(1, sizeof(FevalWork));
    if (!w) return NULL;

    w->eng     = eng;
    w->nargout = nargout;
    w->nargs   = nargs;
    w->state   = NOVA_FUTURE_PENDING;
    w->error   = NOVA_MATLAB_OK;
    strncpy(w->func, func, 255); w->func[255] = '\0';

    // Deep-copy args so caller can free them immediately
    if (nargs > 0 && args) {
        w->args = (NovaMatlabValue**)malloc((size_t)nargs * sizeof(NovaMatlabValue*));
        if (!w->args) { free(w); return NULL; }
        for (int i = 0; i < nargs; i++) {
            // Shallow copy of the value struct — safe for POD types
            w->args[i] = (NovaMatlabValue*)malloc(sizeof(NovaMatlabValue));
            if (w->args[i]) *w->args[i] = *args[i];
        }
    }

    pthread_mutex_init(&w->mu, NULL);
    pthread_cond_init(&w->cv, NULL);

    pthread_t tid;
    if (pthread_create(&tid, NULL, feval_worker, w) != 0) {
        free(w->args); free(w); return NULL;
    }
    pthread_detach(tid);

    return (NovaFevalFuture*)w;
}

// Get current state (lock-free read)
NovaFutureState nova_feval_future_state(NovaFevalFuture* fut) {
    if (!fut) return NOVA_FUTURE_ERROR;
    FevalWork* w = (FevalWork*)fut;
    pthread_mutex_lock(&w->mu);
    NovaFutureState s = w->state;
    pthread_mutex_unlock(&w->mu);
    return s;
}

// Block until done, copy results
NovaMatlabError nova_feval_future_result(NovaFevalFuture* fut,
                                          NovaMatlabValue** out,
                                          int nargout) {
    if (!fut) return NOVA_MATLAB_ERR_INTERNAL;
    FevalWork* w = (FevalWork*)fut;
    pthread_mutex_lock(&w->mu);
    while (w->state == NOVA_FUTURE_PENDING || w->state == NOVA_FUTURE_RUNNING)
        pthread_cond_wait(&w->cv, &w->mu);
    NovaMatlabError rc = w->error;
    if (rc == NOVA_MATLAB_OK && out && w->results) {
        int n = nargout < w->nargout ? nargout : w->nargout;
        for (int i = 0; i < n; i++) out[i] = w->results[i];
    }
    pthread_mutex_unlock(&w->mu);
    return rc;
}

// Block with timeout
NovaMatlabError nova_feval_future_result_timeout(NovaFevalFuture* fut,
                                                   double timeout_secs,
                                                   NovaMatlabValue** out,
                                                   int nargout) {
    if (!fut) return NOVA_MATLAB_ERR_INTERNAL;
    FevalWork* w = (FevalWork*)fut;

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec  += (time_t)timeout_secs;
    deadline.tv_nsec += (long)((timeout_secs - (long)timeout_secs) * 1e9);
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&w->mu);
    while (w->state == NOVA_FUTURE_PENDING || w->state == NOVA_FUTURE_RUNNING) {
        int rc = pthread_cond_timedwait(&w->cv, &w->mu, &deadline);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&w->mu);
            return NOVA_MATLAB_ERR_TIMEOUT;
        }
    }
    NovaMatlabError rc = w->error;
    if (rc == NOVA_MATLAB_OK && out && w->results) {
        int n = nargout < w->nargout ? nargout : w->nargout;
        for (int i = 0; i < n; i++) out[i] = w->results[i];
    }
    pthread_mutex_unlock(&w->mu);
    return rc;
}

// Cancel if still pending
bool nova_feval_future_cancel(NovaFevalFuture* fut) {
    if (!fut) return false;
    FevalWork* w = (FevalWork*)fut;
    pthread_mutex_lock(&w->mu);
    bool cancelled = false;
    if (w->state == NOVA_FUTURE_PENDING) {
        w->state   = NOVA_FUTURE_CANCELLED;
        w->error   = NOVA_MATLAB_ERR_CANCELLED;
        cancelled  = true;
        pthread_cond_broadcast(&w->cv);
    }
    pthread_mutex_unlock(&w->mu);
    return cancelled;
}

// Check if terminal state reached
bool nova_feval_future_done(NovaFevalFuture* fut) {
    if (!fut) return true;
    NovaFutureState s = nova_feval_future_state(fut);
    return s == NOVA_FUTURE_DONE || s == NOVA_FUTURE_CANCELLED || s == NOVA_FUTURE_ERROR;
}

// Free the future and its resources
void nova_feval_future_free(NovaFevalFuture* fut) {
    if (!fut) return;
    FevalWork* w = (FevalWork*)fut;
    pthread_mutex_destroy(&w->mu);
    pthread_cond_destroy(&w->cv);
    if (w->args) {
        for (int i = 0; i < w->nargs; i++) free(w->args[i]);
        free(w->args);
    }
    // Note: results ownership transferred to caller via nova_feval_future_result()
    free(w->results);
    free(w);
}
