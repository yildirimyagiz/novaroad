/**
 * Nova Multi-Language Lowering Header
 * Unified interface for all language backends
 */

#ifndef NOVA_LOWERING_H
#define NOVA_LOWERING_H

#include "compiler/nova_ir.h"
#include <stdbool.h>

// C++ Backend
bool nova_lower_to_cpp(NovaIRModule *module, const char *output_path);

// Rust Backend
bool nova_lower_to_rust(NovaIRModule *module, const char *output_path);

// Go Backend
bool nova_lower_to_go(NovaIRModule *module, const char *output_path);

// Swift Backend
bool nova_lower_to_swift(NovaIRModule *module, const char *output_path);

// Kotlin Backend
bool nova_lower_to_kotlin(NovaIRModule *module, const char *output_path);

// TypeScript Backend
bool nova_lower_to_typescript(NovaIRModule *module, const char *output_path);

// C Backend
bool nova_lower_to_c(NovaIRModule *module, const char *output_path);

#endif // NOVA_LOWERING_H
