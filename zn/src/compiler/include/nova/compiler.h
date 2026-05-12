// ═══════════════════════════════════════════════════════════════════════════
// Nova Compiler C/C++ API
// ═══════════════════════════════════════════════════════════════════════════

#ifndef NOVA_COMPILER_H
#define NOVA_COMPILER_H

#include "nova.h"

#ifdef __cplusplus
extern "C" {
#endif

// Optimization levels
typedef enum {
    NOVA_OPT_NONE = 0,
    NOVA_OPT_BASIC = 1,
    NOVA_OPT_AGGRESSIVE = 2,
    NOVA_OPT_MAX = 3,
} NovaOptLevel;

// Compilation targets
typedef enum {
    NOVA_TARGET_NATIVE,
    NOVA_TARGET_WASM,
    NOVA_TARGET_JIT,
} NovaTarget;

// Compiler configuration
typedef struct {
    NovaOptLevel opt_level;
    NovaTarget target;
    bool debug_info;
    bool verbose;
    const char* output_path;
} NovaCompilerConfig;

// API
NovaCompilerConfig nova_compiler_config_default(void);

NovaResultCode nova_compiler_compile(
    const NovaCompilerConfig* config,
    const char* source,
    size_t source_len,
    uint8_t** output,
    size_t* output_len
);

NovaResultCode nova_compiler_compile_file(
    const NovaCompilerConfig* config,
    const char* input_path,
    const char* output_path
);

void nova_compiler_free_output(uint8_t* buffer);

#ifdef __cplusplus
}
#endif

#endif // NOVA_COMPILER_H
