/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_chrome_engine.h - Chrome/Browser Background Compute Engine
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * This engine enables Nova to run inside Chrome, Safari, Firefox, and Edge
 * as a Progressive Web App with full background compute capability.
 *
 * Architecture:
 *   ┌──────────────────────────────────────────────────────────┐
 *   │                   Nova Chrome Engine                     │
 *   ├──────────┬──────────┬──────────────┬────────────────────┤
 *   │ Service  │ Web      │ WebGPU       │ SharedArray        │
 *   │ Worker   │ Workers  │ Compute      │ Buffer             │
 *   │ (Offline │ (L2 Army │ (L1 Army     │ (Zero-Copy         │
 *   │  + BG)   │  Threads)│  GPU)        │  IPC)              │
 *   └──────────┴──────────┴──────────────┴────────────────────┘
 *
 * iPhone Support: Uses Safari's Service Worker + WebAssembly.
 * Android Support: Chrome Web Worker + WebGPU (where available).
 * Desktop: Full WebGPU + SharedArrayBuffer + WASM Threads.
 */

#ifndef NOVA_CHROME_ENGINE_H
#define NOVA_CHROME_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// BROWSER CAPABILITIES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    NOVA_BROWSER_CHROME,
    NOVA_BROWSER_SAFARI, // iPhone
    NOVA_BROWSER_FIREFOX,
    NOVA_BROWSER_EDGE,
    NOVA_BROWSER_UNKNOWN
} NovaBrowserType;

typedef struct {
    NovaBrowserType browser;
    bool has_service_worker;
    bool has_web_workers;
    bool has_webgpu;
    bool has_wasm_simd;
    bool has_wasm_threads;
    bool has_shared_array_buffer;
    bool has_offscreen_canvas;
    bool is_mobile;
    bool is_ios;
    bool is_android;
    int max_workers;            // Navigator.hardwareConcurrency
    uint64_t wasm_memory_limit; // bytes
    char user_agent[256];
} NovaBrowserCaps;

// ═══════════════════════════════════════════════════════════════════════════
// SERVICE WORKER — Background & Offline Engine
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    SW_STATE_INSTALLING,
    SW_STATE_WAITING,
    SW_STATE_ACTIVE,
    SW_STATE_REDUNDANT
} NovaServiceWorkerState;

typedef struct {
    NovaServiceWorkerState state;
    bool background_sync_enabled;
    bool push_notifications;
    bool periodic_sync;
    char cache_name[64];
    uint32_t cached_asset_count;
} NovaServiceWorker;

// ═══════════════════════════════════════════════════════════════════════════
// WEB WORKER POOL — L2 Army Compute Threads
// ═══════════════════════════════════════════════════════════════════════════

typedef enum { WORKER_IDLE, WORKER_BUSY, WORKER_ERROR } NovaWorkerState;

typedef struct {
    int worker_id;
    NovaWorkerState state;
    uint64_t tasks_completed;
    double avg_task_ms;
} NovaWebWorker;

typedef struct {
    NovaWebWorker *workers;
    int worker_count;
    int max_workers;
    int active_count;
    uint64_t total_tasks;
    bool use_shared_memory; // SharedArrayBuffer for zero-copy
} NovaWorkerPool;

// ═══════════════════════════════════════════════════════════════════════════
// WEBGPU COMPUTE — L1 Army GPU Dispatch
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    bool available;
    char adapter_name[128];
    uint64_t max_buffer_size;
    uint32_t max_workgroup_size_x;
    uint32_t max_workgroup_size_y;
    uint32_t max_workgroup_size_z;
    uint32_t max_compute_invocations;
    bool has_float16;
    bool has_timestamp_query;
} NovaWebGPUCaps;

typedef struct {
    NovaWebGPUCaps caps;
    bool initialized;
    uint32_t active_pipelines;
} NovaWebGPUContext;

// ═══════════════════════════════════════════════════════════════════════════
// CHROME ENGINE — Main Controller
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    NovaBrowserCaps browser;
    NovaServiceWorker service_worker;
    NovaWorkerPool worker_pool;
    NovaWebGPUContext webgpu;
    bool engine_ready;
} NovaChromeEngine;

// ═══════════════════════════════════════════════════════════════════════════
// LIFECYCLE API
// ═══════════════════════════════════════════════════════════════════════════

/** Detect browser capabilities */
NovaBrowserCaps nova_chrome_detect_browser(void);

/** Initialize the Chrome Engine */
NovaChromeEngine *nova_chrome_engine_init(void);

/** Shutdown the Chrome Engine */
void nova_chrome_engine_shutdown(NovaChromeEngine *engine);

/** Print engine status */
void nova_chrome_engine_print_status(const NovaChromeEngine *engine);

// ═══════════════════════════════════════════════════════════════════════════
// SERVICE WORKER API
// ═══════════════════════════════════════════════════════════════════════════

/** Register the Service Worker for offline + background */
int nova_chrome_sw_register(NovaChromeEngine *engine);

/** Request background sync permission */
int nova_chrome_sw_request_bg_sync(NovaChromeEngine *engine);

/** Cache Nova WASM modules for offline */
int nova_chrome_sw_cache_wasm(NovaChromeEngine *engine, const char *module_url);

// ═══════════════════════════════════════════════════════════════════════════
// WEB WORKER POOL API (L2 Army Threads)
// ═══════════════════════════════════════════════════════════════════════════

/** Create a Web Worker pool sized for the device */
int nova_chrome_workers_init(NovaChromeEngine *engine);

/** Dispatch a compute task to the worker pool */
int nova_chrome_workers_dispatch(NovaChromeEngine *engine, const char *task_type,
                                 const void *input_data, size_t input_size, void *output_data,
                                 size_t output_size);

/** Wait for all workers to complete */
int nova_chrome_workers_sync(NovaChromeEngine *engine);

// ═══════════════════════════════════════════════════════════════════════════
// WEBGPU COMPUTE API (L1 Army GPU)
// ═══════════════════════════════════════════════════════════════════════════

/** Initialize WebGPU compute context */
int nova_chrome_webgpu_init(NovaChromeEngine *engine);

/** Dispatch a GPU compute shader */
int nova_chrome_webgpu_dispatch(NovaChromeEngine *engine, const char *shader_source,
                                const float *input_a, const float *input_b, float *output,
                                size_t element_count);

// ═══════════════════════════════════════════════════════════════════════════
// ARMY BRIDGE — Connect Chrome Engine to GPU-Army V10
// ═══════════════════════════════════════════════════════════════════════════

/** Route Army L1 through WebGPU (if available) */
int nova_chrome_army_l1_gpu(NovaChromeEngine *engine, const float *mat_a, const float *mat_b,
                            float *mat_c, int64_t rows, int64_t cols, int64_t inner);

/** Route Army L2 through Web Workers */
int nova_chrome_army_l2_threaded(NovaChromeEngine *engine, const float *mat_a, const float *mat_b,
                                 float *mat_c, int64_t rows, int64_t cols, int64_t inner);

#endif // NOVA_CHROME_ENGINE_H
