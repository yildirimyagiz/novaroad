/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_wasm.c - WebAssembly Bindings for Nova Engine
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include "nova_bridge.h"
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// MEMORY MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

EMSCRIPTEN_KEEPALIVE
void *nova_wasm_alloc(size_t size) { return malloc(size); }

EMSCRIPTEN_KEEPALIVE
void nova_wasm_free(void *ptr) { free(ptr); }

// ═══════════════════════════════════════════════════════════════════════════
// NOVA BRIDGE / MIRROR IN JS
// ═══════════════════════════════════════════════════════════════════════════

static NovaBridgeClient *g_wasm_client = NULL;

EMSCRIPTEN_KEEPALIVE
void nova_wasm_init_bridge(uint32_t state_count) {
  if (g_wasm_client) {
    free(g_wasm_client->client_state);
    free(g_wasm_client);
  }
  g_wasm_client = nova_bridge_client_create(state_count);
}

EMSCRIPTEN_KEEPALIVE
float *nova_wasm_update_state(const uint8_t *wire_buffer) {
  if (!g_wasm_client)
    return NULL;
  nova_bridge_client_update(g_wasm_client, wire_buffer);
  return g_wasm_client->client_state;
}

EMSCRIPTEN_KEEPALIVE
uint32_t nova_wasm_get_state_size() {
  return g_wasm_client ? g_wasm_client->state_count : 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// NATIVE OPS EXPOSURE
// ═══════════════════════════════════════════════════════════════════════════

#ifdef __wasm_simd128__
#include <wasm_simd128.h>
// Prototype for SIMD accelerated MatMul for WASM
EMSCRIPTEN_KEEPALIVE
void nova_wasm_matmul_simd(const float *a, const float *b, float *c, int n) {
  for (int i = 0; i < n; i += 4) {
    v128_t va = wasm_v128_load(&a[i]);
    v128_t vb = wasm_v128_load(&b[i]);
    v128_t vc = wasm_f32x4_add(va, vb); // Example op
    wasm_v128_store(&c[i], vc);
  }
}
#endif

EMSCRIPTEN_KEEPALIVE
const char *nova_wasm_get_version() {
  return "Nova Sovereignty v1.0 (WASM-Native)";
}
