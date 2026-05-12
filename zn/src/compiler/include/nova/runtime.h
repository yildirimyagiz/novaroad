// ═══════════════════════════════════════════════════════════════════════════
// Nova Runtime C/C++ API
// ═══════════════════════════════════════════════════════════════════════════

#ifndef NOVA_RUNTIME_H
#define NOVA_RUNTIME_H

#include "nova.h"

#ifdef __cplusplus
extern "C" {
#endif

// Memory management
void* nova_alloc(NovaContext* ctx, size_t size);
void nova_free(NovaContext* ctx, void* ptr);
void nova_gc_collect(NovaContext* ctx);
void nova_gc_stats(NovaContext* ctx, size_t* allocated, size_t* used);

// Threading
typedef struct NovaThread NovaThread;

NovaThread* nova_thread_spawn(NovaContext* ctx, NovaFunction* func, NovaValue* arg);
NovaResultCode nova_thread_join(NovaThread* thread, NovaValue** result);
void nova_thread_detach(NovaThread* thread);

// Async/Await
typedef struct NovaFuture NovaFuture;

NovaFuture* nova_async_call(NovaContext* ctx, NovaFunction* func, NovaValue** args, size_t count);
bool nova_future_poll(NovaFuture* future, NovaValue** result);
NovaResultCode nova_future_await(NovaFuture* future, NovaValue** result);
void nova_future_free(NovaFuture* future);

// FFI
typedef NovaValue* (*NovaFFIFunction)(NovaContext* ctx, NovaValue** args, size_t count);

NovaResultCode nova_ffi_register(NovaContext* ctx, NovaModule* module, const char* name, NovaFFIFunction func);
NovaModule* nova_ffi_load_library(NovaContext* ctx, const char* path);

#ifdef __cplusplus
}
#endif

#endif // NOVA_RUNTIME_H
