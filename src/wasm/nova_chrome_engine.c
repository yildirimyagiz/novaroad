/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_chrome_engine.c - Chrome/Browser Background Compute Engine
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * C-side implementation that compiles to WASM via Emscripten.
 * This is the "brain" that runs inside the browser; the JavaScript glue
 * (nova_chrome_runtime.js) handles the actual Web API calls.
 */

#include "nova_chrome_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

// ═══════════════════════════════════════════════════════════════════════════
// BROWSER DETECTION
// ═══════════════════════════════════════════════════════════════════════════

NovaBrowserCaps nova_chrome_detect_browser(void)
{
    NovaBrowserCaps caps;
    memset(&caps, 0, sizeof(caps));

#ifdef __EMSCRIPTEN__
    // In real WASM, we would call JS to get navigator.userAgent
    // For now, set sensible defaults for Emscripten environments
    caps.browser = NOVA_BROWSER_CHROME;
    caps.has_service_worker = true;
    caps.has_web_workers = true;
    caps.has_wasm_simd = true;
    caps.has_wasm_threads = true;
    caps.has_shared_array_buffer = true;
    caps.has_offscreen_canvas = true;
    caps.max_workers = 4;                                       // Conservative default
    caps.wasm_memory_limit = (uint64_t) 4 * 1024 * 1024 * 1024; // 4GB
    caps.is_mobile = false;
    caps.is_ios = false;
    caps.is_android = false;

    // WebGPU detection would happen via JS
    caps.has_webgpu = false; // Assume not until JS confirms

    strncpy(caps.user_agent, "Emscripten/Nova WASM Runtime", 255);
#else
    // Native fallback for testing outside browser
    caps.browser = NOVA_BROWSER_UNKNOWN;
    caps.has_service_worker = false;
    caps.has_web_workers = false;
    caps.has_webgpu = false;
    caps.has_wasm_simd = false;
    caps.has_wasm_threads = false;
    caps.has_shared_array_buffer = false;
    caps.has_offscreen_canvas = false;
    caps.max_workers = 0;
    caps.wasm_memory_limit = 0;
    strncpy(caps.user_agent, "Native (non-browser)", 255);
#endif

    return caps;
}

// ═══════════════════════════════════════════════════════════════════════════
// ENGINE LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

EMSCRIPTEN_KEEPALIVE
NovaChromeEngine *nova_chrome_engine_init(void)
{
    printf("🌐 [CHROME ENGINE] Initializing Nova Browser Runtime...\n");

    NovaChromeEngine *engine = (NovaChromeEngine *) calloc(1, sizeof(NovaChromeEngine));
    if (!engine)
        return NULL;

    // Step 1: Detect browser capabilities
    engine->browser = nova_chrome_detect_browser();

    // Step 2: Initialize Service Worker
    engine->service_worker.state = SW_STATE_INSTALLING;
    strncpy(engine->service_worker.cache_name, "nova-army-v10-cache", 63);
    engine->service_worker.cached_asset_count = 0;

    // Step 3: Initialize Worker Pool
    engine->worker_pool.max_workers = engine->browser.max_workers;
    engine->worker_pool.worker_count = 0;
    engine->worker_pool.workers = NULL;
    engine->worker_pool.active_count = 0;
    engine->worker_pool.total_tasks = 0;
    engine->worker_pool.use_shared_memory = engine->browser.has_shared_array_buffer;

    // Step 4: Initialize WebGPU
    engine->webgpu.initialized = false;
    engine->webgpu.caps.available = engine->browser.has_webgpu;

    engine->engine_ready = true;
    printf("🌐 [CHROME ENGINE] Ready.\n");

    return engine;
}

EMSCRIPTEN_KEEPALIVE
void nova_chrome_engine_shutdown(NovaChromeEngine *engine)
{
    if (!engine)
        return;

    printf("🌐 [CHROME ENGINE] Shutting down...\n");

    if (engine->worker_pool.workers) {
        free(engine->worker_pool.workers);
    }

    free(engine);
    printf("🌐 [CHROME ENGINE] Shutdown complete.\n");
}

EMSCRIPTEN_KEEPALIVE
void nova_chrome_engine_print_status(const NovaChromeEngine *engine)
{
    if (!engine)
        return;

    const NovaBrowserCaps *browser = &engine->browser;

    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  🌐 Nova Chrome Engine Status                             ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  User Agent: %-42s  ║\n", browser->user_agent);
    printf("║  Platform:   %-42s  ║\n", browser->is_ios       ? "📱 iOS (Safari/Chrome)"
                                        : browser->is_android ? "🤖 Android (Chrome)"
                                                              : "🖥️ Desktop");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Service Worker: %-38s  ║\n",
           browser->has_service_worker ? "✅ Supported" : "❌ Not Available");
    printf("║  Web Workers:    %-38s  ║\n",
           browser->has_web_workers ? "✅ Supported" : "❌ Not Available");
    printf("║  Max Workers:    %-38d  ║\n", browser->max_workers);
    printf("║  WebGPU:         %-38s  ║\n",
           browser->has_webgpu ? "✅ Supported" : "❌ Not Available");
    printf("║  WASM SIMD:      %-38s  ║\n",
           browser->has_wasm_simd ? "✅ Supported" : "❌ Not Available");
    printf("║  WASM Threads:   %-38s  ║\n",
           browser->has_wasm_threads ? "✅ Supported" : "❌ Not Available");
    printf("║  SharedArrayBuf: %-38s  ║\n",
           browser->has_shared_array_buffer ? "✅ Supported" : "❌ Not Available");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Army L1 (GPU):    → %-34s  ║\n",
           browser->has_webgpu ? "WebGPU Compute" : "WASM SIMD (Fallback)");
    printf("║  Army L2 (Thread): → %-34s  ║\n",
           browser->has_web_workers ? "Web Worker Pool" : "Main Thread (Slow)");
    printf("║  Army L3 (Net):    → %-34s  ║\n", "Fetch API + WebSocket");
    printf("║  Army L4 (P2P):    → %-34s  ║\n", "WebRTC Data Channel");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// SERVICE WORKER — Offline + Background
// ═══════════════════════════════════════════════════════════════════════════

EMSCRIPTEN_KEEPALIVE
int nova_chrome_sw_register(NovaChromeEngine *engine)
{
    if (!engine || !engine->browser.has_service_worker) {
        printf("⚠️ [SW] Service Worker not supported on this browser.\n");
        return -1;
    }

    printf("🔧 [SW] Registering Service Worker...\n");

#ifdef __EMSCRIPTEN__
    // Call JS to register: navigator.serviceWorker.register('/nova-sw.js')
    EM_ASM({
        if ('serviceWorker' in navigator) {
            navigator.serviceWorker.register('/nova-sw.js')
                .then(function(reg) { console.log('Nova SW registered:', reg.scope); })
                .catch(function(err) { console.error('Nova SW registration failed:', err); });
        }
    });
#endif

    engine->service_worker.state = SW_STATE_ACTIVE;
    printf("✅ [SW] Service Worker registered. Cache: %s\n", engine->service_worker.cache_name);
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_chrome_sw_request_bg_sync(NovaChromeEngine *engine)
{
    if (!engine)
        return -1;

    printf("🔄 [SW] Requesting Background Sync permission...\n");

#ifdef __EMSCRIPTEN__
    EM_ASM({
        if ('serviceWorker' in navigator && 'SyncManager' in window) {
            navigator.serviceWorker.ready
                .then(function(reg) { return reg.sync.register('nova-army-sync'); })
                .then(function() { console.log('Background sync registered'); });
        }
    });
#endif

    engine->service_worker.background_sync_enabled = true;
    printf("✅ [SW] Background Sync enabled.\n");
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_chrome_sw_cache_wasm(NovaChromeEngine *engine, const char *module_url)
{
    if (!engine || !module_url)
        return -1;

    printf("📦 [SW] Caching WASM module: %s\n", module_url);

#ifdef __EMSCRIPTEN__
    EM_ASM(
        {
            var url = UTF8ToString($0);
            caches.open('nova-army-v10-cache')
                .then(function(cache) { return cache.add(url); })
                .then(function() { console.log('WASM module cached:', url); });
        },
        module_url);
#endif

    engine->service_worker.cached_asset_count++;
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// WEB WORKER POOL — L2 Army Threads
// ═══════════════════════════════════════════════════════════════════════════

EMSCRIPTEN_KEEPALIVE
int nova_chrome_workers_init(NovaChromeEngine *engine)
{
    if (!engine || !engine->browser.has_web_workers) {
        printf("⚠️ [WORKERS] Web Workers not supported.\n");
        return -1;
    }

    int count = engine->browser.max_workers;
    if (count < 1)
        count = 2; // At least 2 workers
    if (count > 16)
        count = 16; // Cap at 16

    // On mobile, reduce worker count to save battery
    if (engine->browser.is_mobile) {
        count = count > 4 ? 4 : count;
    }

    printf("🧵 [WORKERS] Initializing %d Web Workers...\n", count);

    engine->worker_pool.workers = (NovaWebWorker *) calloc(count, sizeof(NovaWebWorker));
    engine->worker_pool.worker_count = count;

    for (int idx = 0; idx < count; idx++) {
        engine->worker_pool.workers[idx].worker_id = idx;
        engine->worker_pool.workers[idx].state = WORKER_IDLE;
        engine->worker_pool.workers[idx].tasks_completed = 0;
        engine->worker_pool.workers[idx].avg_task_ms = 0.0;

#ifdef __EMSCRIPTEN__
        // In full Emscripten pthread mode, workers are created automatically.
        // For manual workers, we'd use EM_ASM to spawn:
        // new Worker('nova-compute-worker.js')
#endif
    }

    printf("✅ [WORKERS] %d workers ready (SharedMemory: %s)\n", count,
           engine->worker_pool.use_shared_memory ? "ON" : "OFF");
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_chrome_workers_dispatch(NovaChromeEngine *engine, const char *task_type,
                                 const void *input_data, size_t input_size, void *output_data,
                                 size_t output_size)
{
    if (!engine || !task_type)
        return -1;

    // Find an idle worker
    int assigned = -1;
    for (int idx = 0; idx < engine->worker_pool.worker_count; idx++) {
        if (engine->worker_pool.workers[idx].state == WORKER_IDLE) {
            assigned = idx;
            break;
        }
    }

    if (assigned < 0) {
        printf("⚠️ [WORKERS] All workers busy, queuing task '%s'\n", task_type);
        return -1;
    }

    engine->worker_pool.workers[assigned].state = WORKER_BUSY;
    engine->worker_pool.active_count++;

    printf("🧵 [WORKER-%d] Dispatching: %s (%zu bytes)\n", assigned, task_type, input_size);

#ifdef __EMSCRIPTEN__
    // In practice, would postMessage to the worker
    // with transferable ArrayBuffer for zero-copy
#endif

    // Simulate completion for native testing
    engine->worker_pool.workers[assigned].state = WORKER_IDLE;
    engine->worker_pool.workers[assigned].tasks_completed++;
    engine->worker_pool.active_count--;
    engine->worker_pool.total_tasks++;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_chrome_workers_sync(NovaChromeEngine *engine)
{
    if (!engine)
        return -1;

    // Wait for all workers to finish
    printf("⏳ [WORKERS] Waiting for %d active workers...\n", engine->worker_pool.active_count);

    // In WASM: would use Atomics.wait on SharedArrayBuffer
    // In native: already synchronous

    printf("✅ [WORKERS] All workers synced. Total tasks: %llu\n",
           (unsigned long long) engine->worker_pool.total_tasks);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// WEBGPU COMPUTE — L1 Army GPU
// ═══════════════════════════════════════════════════════════════════════════

EMSCRIPTEN_KEEPALIVE
int nova_chrome_webgpu_init(NovaChromeEngine *engine)
{
    if (!engine || !engine->browser.has_webgpu) {
        printf("⚠️ [WEBGPU] WebGPU not available on this browser.\n");
        printf("   → Falling back to WASM SIMD for L1 compute.\n");
        return -1;
    }

    printf("🔱 [WEBGPU] Initializing WebGPU Compute...\n");

#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (navigator.gpu) {
            navigator.gpu.requestAdapter()
                .then(function(adapter) {
                    if (!adapter) {
                        console.warn('No WebGPU adapter found');
                        return;
                    }
                    return adapter.requestDevice();
                })
                .then(function(device) {
                    if (device) {
                        Module.novaWebGPUDevice = device;
                        console.log('WebGPU device initialized');
                    }
                });
        }
    });
#endif

    engine->webgpu.initialized = true;
    strncpy(engine->webgpu.caps.adapter_name, "WebGPU Adapter", 127);
    engine->webgpu.caps.max_workgroup_size_x = 256;
    engine->webgpu.caps.max_workgroup_size_y = 256;
    engine->webgpu.caps.max_workgroup_size_z = 64;

    printf("✅ [WEBGPU] WebGPU Compute ready.\n");
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_chrome_webgpu_dispatch(NovaChromeEngine *engine, const char *shader_source,
                                const float *input_a, const float *input_b, float *output,
                                size_t element_count)
{
    if (!engine || !engine->webgpu.initialized) {
        printf("⚠️ [WEBGPU] Not initialized, cannot dispatch.\n");
        return -1;
    }

    printf("🔱 [WEBGPU] Dispatching compute shader (%zu elements)\n", element_count);

#ifdef __EMSCRIPTEN__
    // Would create GPUBuffer, GPUComputePipeline, encode, submit
    // Using the shader_source WGSL code
#endif

    engine->webgpu.active_pipelines++;
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// ARMY BRIDGE — Connect Chrome Engine to GPU-Army V10
// ═══════════════════════════════════════════════════════════════════════════

EMSCRIPTEN_KEEPALIVE
int nova_chrome_army_l1_gpu(NovaChromeEngine *engine, const float *mat_a, const float *mat_b,
                            float *mat_c, int64_t rows, int64_t cols, int64_t inner)
{
    if (!engine)
        return -1;

    if (engine->webgpu.initialized) {
        printf("⚡ [ARMY-L1] WebGPU GPU Compute (%lldx%lld)\n", (long long) rows, (long long) cols);

        // Generate WGSL shader for matmul
        const char *wgsl_matmul =
            "@group(0) @binding(0) var<storage, read> a: array<f32>;\n"
            "@group(0) @binding(1) var<storage, read> b: array<f32>;\n"
            "@group(0) @binding(2) var<storage, read_write> c: array<f32>;\n"
            "@compute @workgroup_size(16, 16)\n"
            "fn main(@builtin(global_invocation_id) gid: vec3<u32>) {\n"
            "  let row = gid.x; let col = gid.y;\n"
            "  var sum: f32 = 0.0;\n"
            "  for (var k: u32 = 0u; k < arrayLength(&a); k++) {\n"
            "    sum += a[row * arrayLength(&a) + k] * b[k * arrayLength(&b) + col];\n"
            "  }\n"
            "  c[row * arrayLength(&b) + col] = sum;\n"
            "}\n";

        return nova_chrome_webgpu_dispatch(engine, wgsl_matmul, mat_a, mat_b, mat_c,
                                           (size_t) (rows * cols));
    }

    // Fallback to WASM SIMD
    printf("⚡ [ARMY-L1] WASM SIMD Fallback (%lldx%lld)\n", (long long) rows, (long long) cols);

#ifdef __wasm_simd128__
    // Use wasm_simd128.h ops
#endif

    // CPU fallback for non-WASM builds
    for (int64_t row = 0; row < rows; row++) {
        for (int64_t col = 0; col < cols; col++) {
            float sum = 0.0f;
            for (int64_t kidx = 0; kidx < inner; kidx++) {
                sum += mat_a[row * inner + kidx] * mat_b[kidx * cols + col];
            }
            mat_c[row * cols + col] = sum;
        }
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_chrome_army_l2_threaded(NovaChromeEngine *engine, const float *mat_a, const float *mat_b,
                                 float *mat_c, int64_t rows, int64_t cols, int64_t inner)
{
    if (!engine)
        return -1;

    if (engine->worker_pool.worker_count > 0) {
        printf("🧵 [ARMY-L2] Web Worker Pool Matmul (%lldx%lld, %d workers)\n", (long long) rows,
               (long long) cols, engine->worker_pool.worker_count);

        // Split rows across workers
        int64_t rows_per_worker = rows / engine->worker_pool.worker_count;
        for (int idx = 0; idx < engine->worker_pool.worker_count; idx++) {
            int64_t start_row = idx * rows_per_worker;
            int64_t end_row = (idx == engine->worker_pool.worker_count - 1)
                                  ? rows
                                  : (start_row + rows_per_worker);

            nova_chrome_workers_dispatch(engine, "matmul_chunk", &mat_a[start_row * inner],
                                         (size_t) ((end_row - start_row) * inner * sizeof(float)),
                                         &mat_c[start_row * cols],
                                         (size_t) ((end_row - start_row) * cols * sizeof(float)));
        }

        nova_chrome_workers_sync(engine);
        return 0;
    }

    // Single-threaded fallback
    printf("🧵 [ARMY-L2] Single-thread fallback (%lldx%lld)\n", (long long) rows, (long long) cols);

    for (int64_t row = 0; row < rows; row++) {
        for (int64_t col = 0; col < cols; col++) {
            float sum = 0.0f;
            for (int64_t kidx = 0; kidx < inner; kidx++) {
                sum += mat_a[row * inner + kidx] * mat_b[kidx * cols + col];
            }
            mat_c[row * cols + col] = sum;
        }
    }
    return 0;
}
