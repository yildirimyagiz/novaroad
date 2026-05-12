// nova_engine_session.c — Shared MATLAB session management
// Nova ML / MATLAB Engine Bridge
#include "nova_matlab_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define NOVA_MAX_SESSIONS 16

static NovaEngineSession    g_sessions[NOVA_MAX_SESSIONS];
static int                  g_n_sessions = 0;
static pthread_mutex_t      g_session_mutex = PTHREAD_MUTEX_INITIALIZER;

// Internal: register a session
static void __attribute__((unused)) _nova_session_register(const char* name, uint32_t pid, bool is_shared) {
    pthread_mutex_lock(&g_session_mutex);
    // Check if already registered
    for (int i = 0; i < g_n_sessions; i++) {
        if (strncmp(g_sessions[i].name, name, 255) == 0) {
            pthread_mutex_unlock(&g_session_mutex);
            return;
        }
    }
    if (g_n_sessions < NOVA_MAX_SESSIONS) {
        strncpy(g_sessions[g_n_sessions].name, name, 255);
        g_sessions[g_n_sessions].name[255] = '\0';
        g_sessions[g_n_sessions].pid       = pid;
        g_sessions[g_n_sessions].is_shared = is_shared;
        g_n_sessions++;
    }
    pthread_mutex_unlock(&g_session_mutex);
}

// Internal: deregister a session by name
static void __attribute__((unused)) _nova_session_deregister(const char* name) {
    pthread_mutex_lock(&g_session_mutex);
    for (int i = 0; i < g_n_sessions; i++) {
        if (strncmp(g_sessions[i].name, name, 255) == 0) {
            // Shift remaining entries left
            for (int j = i; j < g_n_sessions - 1; j++) {
                g_sessions[j] = g_sessions[j + 1];
            }
            g_n_sessions--;
            break;
        }
    }
    pthread_mutex_unlock(&g_session_mutex);
}

// List all known shared sessions
int nova_matlab_find(NovaEngineSession* out, int max_sessions) {
    pthread_mutex_lock(&g_session_mutex);
    int count = g_n_sessions < max_sessions ? g_n_sessions : max_sessions;
    for (int i = 0; i < count; i++) {
        out[i] = g_sessions[i];
    }
    pthread_mutex_unlock(&g_session_mutex);
    return count;
}

// Connect to a named shared session
NovaMatlabEngine* nova_matlab_connect(const char* name, NovaMatlabError* err) {
#ifdef NOVA_MATLAB_NATIVE
    Engine* ep = engConnectToSession(name);
    if (!ep) {
        if (err) *err = NOVA_MATLAB_ERR_NOT_FOUND;
        return NULL;
    }
    NovaMatlabEngine* eng = (NovaMatlabEngine*)calloc(1, sizeof(*eng));
    if (!eng) { if (err) *err = NOVA_MATLAB_ERR_ALLOC; engClose(ep); return NULL; }
    eng->ep        = ep;
    eng->is_shared = true;
    eng->alive     = true;
    strncpy(eng->name, name, 255);
    _nova_session_register(name, 0, true);
    nova_engine_registry_register(eng);
    if (err) *err = NOVA_MATLAB_OK;
    return eng;
#else
    (void)name;
    if (err) *err = NOVA_MATLAB_ERR_INTERNAL;
    return NULL;
#endif
}

// Connect to any available shared session
NovaMatlabEngine* nova_matlab_connect_any(NovaMatlabError* err) {
    pthread_mutex_lock(&g_session_mutex);
    if (g_n_sessions == 0) {
        pthread_mutex_unlock(&g_session_mutex);
        if (err) *err = NOVA_MATLAB_ERR_NOT_FOUND;
        return NULL;
    }
    char first_name[256];
    strncpy(first_name, g_sessions[0].name, 255);
    first_name[255] = '\0';
    pthread_mutex_unlock(&g_session_mutex);
    return nova_matlab_connect(first_name, err);
}
