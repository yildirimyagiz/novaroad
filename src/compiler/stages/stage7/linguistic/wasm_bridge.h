/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA WASM BACKEND - WebAssembly Code Generation
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Compiles Nova to WebAssembly for web targets
 */

#ifndef NOVA_WASM_H
#define NOVA_WASM_H

#include "compiler/nova_ast.h"
#include <stdbool.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// WASM TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    WASM_TYPE_I32,
    WASM_TYPE_I64,
    WASM_TYPE_F32,
    WASM_TYPE_F64,
    WASM_TYPE_V128,  // SIMD
    WASM_TYPE_FUNCREF,
    WASM_TYPE_EXTERNREF
} WasmType;

typedef enum {
    WASM_OP_I32_ADD = 0x6A,
    WASM_OP_I32_SUB = 0x6B,
    WASM_OP_I32_MUL = 0x6C,
    WASM_OP_I32_DIV_S = 0x6D,
    WASM_OP_I64_ADD = 0x7C,
    WASM_OP_I64_SUB = 0x7D,
    WASM_OP_I64_MUL = 0x7E,
    WASM_OP_F64_ADD = 0xA0,
    WASM_OP_CALL = 0x10,
    WASM_OP_RETURN = 0x0F,
    WASM_OP_END = 0x0B,
    WASM_OP_LOCAL_GET = 0x20,
    WASM_OP_LOCAL_SET = 0x21
} WasmOpcode;

// ═══════════════════════════════════════════════════════════════════════════
// WASM MODULE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    uint8_t *bytes;
    size_t size;
    size_t capacity;
} WasmBuffer;

typedef struct {
    WasmBuffer code;
    size_t function_count;
    size_t import_count;
    size_t export_count;
} WasmModule;

// ═══════════════════════════════════════════════════════════════════════════
// WASM CODE GENERATOR
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    WasmModule *module;
    WasmBuffer current_function;
    bool had_error;
    char error_message[256];
} WasmCodeGen;

/**
 * Create WASM code generator
 */
WasmCodeGen *wasm_codegen_create(void);

/**
 * Destroy WASM code generator
 */
void wasm_codegen_destroy(WasmCodeGen *cg);

/**
 * Generate WASM code from AST
 */
bool wasm_codegen_generate(WasmCodeGen *cg, ASTNode *program);

/**
 * Emit WASM binary to file
 */
bool wasm_emit_binary(WasmCodeGen *cg, const char *filename);

/**
 * Get WASM bytes
 */
const uint8_t *wasm_get_bytes(WasmCodeGen *cg, size_t *size);

// ═══════════════════════════════════════════════════════════════════════════
// WASM BUFFER OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

WasmBuffer *wasm_buffer_create(void);
void wasm_buffer_destroy(WasmBuffer *buf);
void wasm_buffer_write_byte(WasmBuffer *buf, uint8_t byte);
void wasm_buffer_write_u32_leb128(WasmBuffer *buf, uint32_t value);
void wasm_buffer_write_i32_leb128(WasmBuffer *buf, int32_t value);
void wasm_buffer_write_bytes(WasmBuffer *buf, const uint8_t *bytes, size_t len);

#endif // NOVA_WASM_H
