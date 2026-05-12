/**
 * Nova WASM Backend
 * Compiles Nova to WebAssembly
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* module_name;
    FILE* output;
    int function_count;
} NovaWASMContext;

// Create WASM context
NovaWASMContext* nova_wasm_create_context(const char* module_name) {
    NovaWASMContext* ctx = malloc(sizeof(NovaWASMContext));
    ctx->module_name = strdup(module_name);
    ctx->output = NULL;
    ctx->function_count = 0;
    return ctx;
}

// Destroy WASM context
void nova_wasm_destroy_context(NovaWASMContext* ctx) {
    if (ctx) {
        free(ctx->module_name);
        if (ctx->output) fclose(ctx->output);
        free(ctx);
    }
}

// Emit WASM module header
void nova_wasm_emit_header(NovaWASMContext* ctx) {
    fprintf(ctx->output, ";; WASM module: %s\n", ctx->module_name);
    fprintf(ctx->output, "(module\n");
}

// Emit WASM module footer
void nova_wasm_emit_footer(NovaWASMContext* ctx) {
    fprintf(ctx->output, ")\n");
}

// Emit WASM function
void nova_wasm_emit_function(NovaWASMContext* ctx, const char* name, 
                              const char* params, const char* result) {
    fprintf(ctx->output, "  (func $%s ", name);
    
    if (params && strlen(params) > 0) {
        fprintf(ctx->output, "(param %s) ", params);
    }
    
    if (result && strlen(result) > 0) {
        fprintf(ctx->output, "(result %s) ", result);
    }
    
    fprintf(ctx->output, "\n");
    ctx->function_count++;
}

// Emit WASM instruction
void nova_wasm_emit_instr(NovaWASMContext* ctx, const char* instr) {
    fprintf(ctx->output, "    %s\n", instr);
}

// Close function
void nova_wasm_close_function(NovaWASMContext* ctx) {
    fprintf(ctx->output, "  )\n");
}

// Export function
void nova_wasm_export_function(NovaWASMContext* ctx, const char* name) {
    fprintf(ctx->output, "  (export \"%s\" (func $%s))\n", name, name);
}

// Generate WASM file
int nova_wasm_generate_file(NovaWASMContext* ctx, const char* output_path) {
    ctx->output = fopen(output_path, "w");
    if (!ctx->output) {
        return 0;
    }
    
    nova_wasm_emit_header(ctx);
    
    // Example: simple add function
    nova_wasm_emit_function(ctx, "add", "i32 i32", "i32");
    nova_wasm_emit_instr(ctx, "local.get 0");
    nova_wasm_emit_instr(ctx, "local.get 1");
    nova_wasm_emit_instr(ctx, "i32.add");
    nova_wasm_close_function(ctx);
    nova_wasm_export_function(ctx, "add");
    
    nova_wasm_emit_footer(ctx);
    
    fclose(ctx->output);
    ctx->output = NULL;
    
    printf("✅ WASM module generated: %s\n", output_path);
    printf("   Functions: %d\n", ctx->function_count);
    
    return 1;
}

// Simple test
int main() {
    NovaWASMContext* ctx = nova_wasm_create_context("test");
    nova_wasm_generate_file(ctx, "test.wat");
    nova_wasm_destroy_context(ctx);
    return 0;
}
