// ═══════════════════════════════════════════════════════════════════════════
// Nova Language - Main C/C++ API Header
// ═══════════════════════════════════════════════════════════════════════════

#ifndef NOVA_H
#define NOVA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Version
#define NOVA_VERSION_MAJOR 1
#define NOVA_VERSION_MINOR 0
#define NOVA_VERSION_PATCH 0
#define NOVA_VERSION "1.0.0"

// Forward declarations
typedef struct NovaContext NovaContext;
typedef struct NovaModule NovaModule;
typedef struct NovaFunction NovaFunction;
typedef struct NovaValue NovaValue;

// Result codes
typedef enum {
    NOVA_OK = 0,
    NOVA_ERROR_SYNTAX = 1,
    NOVA_ERROR_TYPE = 2,
    NOVA_ERROR_RUNTIME = 3,
} NovaResultCode;

// Context management
NovaContext* nova_context_new(void);
void nova_context_free(NovaContext* ctx);

// Compilation
NovaModule* nova_compile(NovaContext* ctx, const char* source, size_t len);
NovaModule* nova_compile_file(NovaContext* ctx, const char* filename);
void nova_module_free(NovaModule* module);

// Execution
NovaValue* nova_call(NovaContext* ctx, NovaFunction* func, NovaValue** args, size_t count);
NovaFunction* nova_module_get_function(NovaModule* module, const char* name);

// Value types
typedef enum {
    NOVA_TYPE_NULL,
    NOVA_TYPE_BOOL,
    NOVA_TYPE_INT,
    NOVA_TYPE_FLOAT,
    NOVA_TYPE_STRING,
} NovaValueType;

NovaValue* nova_value_new_int(NovaContext* ctx, int64_t val);
NovaValue* nova_value_new_float(NovaContext* ctx, double val);
NovaValue* nova_value_new_string(NovaContext* ctx, const char* str, size_t len);
NovaValue* nova_value_new_bool(NovaContext* ctx, bool val);

bool nova_value_to_int(const NovaValue* value, int64_t* out);
bool nova_value_to_float(const NovaValue* value, double* out);
bool nova_value_to_string(const NovaValue* value, const char** out, size_t* len);
bool nova_value_to_bool(const NovaValue* value, bool* out);

void nova_value_free(NovaValue* value);

// Utilities
const char* nova_version(void);
NovaResultCode nova_init(void);
void nova_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // NOVA_H
