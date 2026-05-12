/**
 * Nova Generics System
 * Generic type handling and monomorphization
 */

#ifndef NOVA_GENERICS_H
#define NOVA_GENERICS_H

#include "compiler/nova_types.h"
#include "compiler/nova_ast.h"
#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// TYPE PARAMETER
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *name;              // e.g., "T"
  Type **bounds;           // Trait bounds
  size_t bound_count;
  Type *default_type;      // Default type argument
} TypeParam;

TypeParam *type_param_create(const char *name);
void type_param_destroy(TypeParam *param);
void type_param_add_bound(TypeParam *param, Type *bound);

// ═══════════════════════════════════════════════════════════════════════════
// GENERIC CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char **param_names;      // Type parameter names
  Type **param_types;      // Actual types (for instantiation)
  size_t param_count;
} GenericContext;

GenericContext *generic_context_create(void);
void generic_context_destroy(GenericContext *ctx);
void generic_context_add_param(GenericContext *ctx, const char *name, Type *type);
Type *generic_context_lookup(const GenericContext *ctx, const char *name);

// ═══════════════════════════════════════════════════════════════════════════
// GENERIC INSTANTIATION
// ═══════════════════════════════════════════════════════════════════════════

// Instantiate generic type with concrete type arguments
Type *generic_instantiate(Type *generic_type, Type **type_args, size_t arg_count);

// Substitute type parameters in a type
Type *generic_substitute(Type *type, const GenericContext *ctx);

// ═══════════════════════════════════════════════════════════════════════════
// MONOMORPHIZATION
// ═══════════════════════════════════════════════════════════════════════════

typedef struct MonomorphInstance {
  ASTNode *original;           // Original generic function/struct
  Type **type_args;            // Concrete type arguments
  size_t type_arg_count;
  ASTNode *instantiated;       // Monomorphized version
  char *mangled_name;          // Mangled name for linker
  struct MonomorphInstance *next;
} MonomorphInstance;

typedef struct {
  MonomorphInstance **instances;
  size_t instance_count;
  size_t instance_capacity;
} MonomorphContext;

MonomorphContext *monomorph_context_create(void);
void monomorph_context_destroy(MonomorphContext *ctx);

// Find or create monomorphized instance
MonomorphInstance *monomorph_get_instance(MonomorphContext *ctx, 
                                           ASTNode *generic_node,
                                           Type **type_args,
                                           size_t arg_count);

// Monomorphize a generic function
ASTNode *monomorph_function(ASTNode *generic_func, Type **type_args, size_t arg_count);

// Monomorphize a generic struct
ASTNode *monomorph_struct(ASTNode *generic_struct, Type **type_args, size_t arg_count);

// ═══════════════════════════════════════════════════════════════════════════
// TYPE INFERENCE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  Type **constraints;      // Type constraints
  size_t constraint_count;
  Type *inferred_type;     // Result of inference
} TypeInference;

TypeInference *type_inference_create(void);
void type_inference_destroy(TypeInference *inf);

// Add constraint: type variable must equal concrete type
void type_inference_add_constraint(TypeInference *inf, Type *constraint);

// Solve constraints and infer type
bool type_inference_solve(TypeInference *inf);

// ═══════════════════════════════════════════════════════════════════════════
// NAME MANGLING
// ═══════════════════════════════════════════════════════════════════════════

// Generate mangled name for generic instance
// e.g., "Vec<i32>" -> "Vec_i32"
char *mangle_generic_name(const char *base_name, Type **type_args, size_t arg_count);

// Demangle name
char *demangle_name(const char *mangled_name);

#endif // NOVA_GENERICS_H
