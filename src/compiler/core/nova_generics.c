#include "nova_common.h"

/**
 * Nova Generics Implementation
 */

#include "compiler/nova_generics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// TYPE PARAMETER
// ═══════════════════════════════════════════════════════════════════════════

TypeParam *type_param_create(const char *name) {
  TypeParam *param = (TypeParam *)calloc(1, sizeof(TypeParam));
  if (!param) return NULL;
  
  param->name = name ? strdup(name) : NULL;
  param->bounds = NULL;
  param->bound_count = 0;
  param->default_type = NULL;
  
  return param;
}

void type_param_destroy(TypeParam *param) {
  if (!param) return;
  
  free(param->name);
  
  for (size_t i = 0; i < param->bound_count; i++) {
    type_destroy(param->bounds[i]);
  }
  free(param->bounds);
  
  if (param->default_type) {
    type_destroy(param->default_type);
  }
  
  free(param);
}

void type_param_add_bound(TypeParam *param, Type *bound) {
  if (!param || !bound) return;
  
  param->bounds = (Type **)realloc(param->bounds, 
                                    (param->bound_count + 1) * sizeof(Type*));
  param->bounds[param->bound_count++] = bound;
}

// ═══════════════════════════════════════════════════════════════════════════
// GENERIC CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

GenericContext *generic_context_create(void) {
  GenericContext *ctx = (GenericContext *)calloc(1, sizeof(GenericContext));
  return ctx;
}

void generic_context_destroy(GenericContext *ctx) {
  if (!ctx) return;
  
  for (size_t i = 0; i < ctx->param_count; i++) {
    free(ctx->param_names[i]);
    type_destroy(ctx->param_types[i]);
  }
  
  free(ctx->param_names);
  free(ctx->param_types);
  free(ctx);
}

void generic_context_add_param(GenericContext *ctx, const char *name, Type *type) {
  if (!ctx || !name) return;
  
  ctx->param_names = (char **)realloc(ctx->param_names, 
                                       (ctx->param_count + 1) * sizeof(char*));
  ctx->param_types = (Type **)realloc(ctx->param_types,
                                       (ctx->param_count + 1) * sizeof(Type*));
  
  ctx->param_names[ctx->param_count] = strdup(name);
  ctx->param_types[ctx->param_count] = type;
  ctx->param_count++;
}

Type *generic_context_lookup(const GenericContext *ctx, const char *name) {
  if (!ctx || !name) return NULL;
  
  for (size_t i = 0; i < ctx->param_count; i++) {
    if (strcmp(ctx->param_names[i], name) == 0) {
      return ctx->param_types[i];
    }
  }
  
  return NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// GENERIC INSTANTIATION
// ═══════════════════════════════════════════════════════════════════════════

Type *generic_instantiate(Type *generic_type, Type **type_args, size_t arg_count) {
  if (!generic_type || generic_type->kind != TYPE_GENERIC) {
    return generic_type;
  }
  
  // Create instantiated type based on base type
  // For now, return a copy with substituted type arguments
  Type *instance = type_copy(generic_type);
  
  // Replace type arguments
  if (instance->data.generic.type_args) {
    for (size_t i = 0; i < instance->data.generic.type_arg_count; i++) {
      type_destroy(instance->data.generic.type_args[i]);
    }
    free(instance->data.generic.type_args);
  }
  
  instance->data.generic.type_args = (Type **)malloc(arg_count * sizeof(Type*));
  instance->data.generic.type_arg_count = arg_count;
  
  for (size_t i = 0; i < arg_count; i++) {
    instance->data.generic.type_args[i] = type_copy(type_args[i]);
  }
  
  return instance;
}

Type *generic_substitute(Type *type, const GenericContext *ctx) {
  if (!type || !ctx) return type;
  
  switch (type->kind) {
    case TYPE_PARAMETER: {
      // Look up type parameter in context
      Type *substitution = generic_context_lookup(ctx, type->name);
      return substitution ? type_copy(substitution) : type_copy(type);
    }
    
    case TYPE_ARRAY: {
      Type *elem = generic_substitute(type->data.array.element_type, ctx);
      return type_create_array(elem, type->data.array.size);
    }
    
    case TYPE_FUNCTION: {
      Type **params = (Type **)malloc(type->data.function.param_count * sizeof(Type*));
      for (size_t i = 0; i < type->data.function.param_count; i++) {
        params[i] = generic_substitute(type->data.function.param_types[i], ctx);
      }
      Type *ret = generic_substitute(type->data.function.return_type, ctx);
      return type_create_function(params, type->data.function.param_count, ret);
    }
    
    case TYPE_REFERENCE:
    case TYPE_MUT_REFERENCE: {
      Type *ref = generic_substitute(type->data.reference.referenced_type, ctx);
      return type_create_reference(ref, type->data.reference.is_mutable);
    }
    
    case TYPE_GENERIC: {
      // Substitute type arguments
      Type **args = (Type **)malloc(type->data.generic.type_arg_count * sizeof(Type*));
      for (size_t i = 0; i < type->data.generic.type_arg_count; i++) {
        args[i] = generic_substitute(type->data.generic.type_args[i], ctx);
      }
      return type_create_generic(type->data.generic.base_name, args, 
                                  type->data.generic.type_arg_count);
    }
    
    default:
      return type_copy(type);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// MONOMORPHIZATION
// ═══════════════════════════════════════════════════════════════════════════

MonomorphContext *monomorph_context_create(void) {
  MonomorphContext *ctx = (MonomorphContext *)calloc(1, sizeof(MonomorphContext));
  if (!ctx) return NULL;
  
  ctx->instance_capacity = 16;
  ctx->instances = (MonomorphInstance **)calloc(ctx->instance_capacity, 
                                                 sizeof(MonomorphInstance*));
  
  return ctx;
}

void monomorph_context_destroy(MonomorphContext *ctx) {
  if (!ctx) return;
  
  for (size_t i = 0; i < ctx->instance_count; i++) {
    MonomorphInstance *inst = ctx->instances[i];
    
    for (size_t j = 0; j < inst->type_arg_count; j++) {
      type_destroy(inst->type_args[j]);
    }
    free(inst->type_args);
    free(inst->mangled_name);
    
    // Note: Don't destroy original/instantiated AST nodes - they're owned elsewhere
    free(inst);
  }
  
  free(ctx->instances);
  free(ctx);
}

static bool types_match(Type **a, Type **b, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (!type_equals(a[i], b[i])) {
      return false;
    }
  }
  return true;
}

MonomorphInstance *monomorph_get_instance(MonomorphContext *ctx,
                                           ASTNode *generic_node,
                                           Type **type_args,
                                           size_t arg_count) {
  if (!ctx || !generic_node) return NULL;
  
  // Search for existing instance
  for (size_t i = 0; i < ctx->instance_count; i++) {
    MonomorphInstance *inst = ctx->instances[i];
    
    if (inst->original == generic_node &&
        inst->type_arg_count == arg_count &&
        types_match(inst->type_args, type_args, arg_count)) {
      return inst;
    }
  }
  
  // Create new instance
  MonomorphInstance *inst = (MonomorphInstance *)calloc(1, sizeof(MonomorphInstance));
  inst->original = generic_node;
  inst->type_arg_count = arg_count;
  inst->type_args = (Type **)malloc(arg_count * sizeof(Type*));
  
  for (size_t i = 0; i < arg_count; i++) {
    inst->type_args[i] = type_copy(type_args[i]);
  }
  
  // Generate mangled name
  const char *base_name = NULL;
  if (generic_node->type == AST_FUNCTION) {
    base_name = generic_node->data.function.name;
  } else if (generic_node->type == AST_STRUCT) {
    base_name = generic_node->data.struct_decl.name;
  }
  
  if (base_name) {
    inst->mangled_name = mangle_generic_name(base_name, type_args, arg_count);
  }
  
  // Monomorphize the node
  if (generic_node->type == AST_FUNCTION) {
    inst->instantiated = monomorph_function(generic_node, type_args, arg_count);
  } else if (generic_node->type == AST_STRUCT) {
    inst->instantiated = monomorph_struct(generic_node, type_args, arg_count);
  }
  
  // Add to context
  if (ctx->instance_count >= ctx->instance_capacity) {
    ctx->instance_capacity *= 2;
    ctx->instances = (MonomorphInstance **)realloc(ctx->instances,
                                                     ctx->instance_capacity * sizeof(MonomorphInstance*));
  }
  ctx->instances[ctx->instance_count++] = inst;
  
  return inst;
}

ASTNode *monomorph_function(ASTNode *generic_func, Type **type_args, size_t arg_count) {
  if (!generic_func || generic_func->type != AST_FUNCTION) {
    return NULL;
  }
  
  // Create generic context for substitution
  GenericContext *ctx = generic_context_create();
  
  for (size_t i = 0; i < arg_count && i < generic_func->data.function.type_param_count; i++) {
    generic_context_add_param(ctx, generic_func->data.function.type_params[i], 
                               type_args[i]);
  }
  
  // Clone the function node
  ASTNode *instance = (ASTNode *)malloc(sizeof(ASTNode));
  memcpy(instance, generic_func, sizeof(ASTNode));
  
  // Substitute types in parameters
  if (instance->data.function.param_count > 0) {
    instance->data.function.parameters = (ASTNode **)malloc(
      instance->data.function.param_count * sizeof(ASTNode*));
    
    for (size_t i = 0; i < instance->data.function.param_count; i++) {
      ASTNode *param = (ASTNode *)malloc(sizeof(ASTNode));
      memcpy(param, generic_func->data.function.parameters[i], sizeof(ASTNode));
      
      // Substitute parameter type
      param->data.var_decl.var_type = 
        generic_substitute(generic_func->data.function.parameters[i]->data.var_decl.var_type, ctx);
      
      instance->data.function.parameters[i] = param;
    }
  }
  
  // Substitute return type
  instance->data.function.return_type = 
    generic_substitute(generic_func->data.function.return_type, ctx);
  
  // Update name with mangled version
  instance->data.function.name = mangle_generic_name(generic_func->data.function.name,
                                                      type_args, arg_count);
  
  // Clear type parameters (no longer generic)
  instance->data.function.type_params = NULL;
  instance->data.function.type_param_count = 0;
  
  // TODO: Recursively substitute types in function body
  
  generic_context_destroy(ctx);
  return instance;
}

ASTNode *monomorph_struct(ASTNode *generic_struct, Type **type_args, size_t arg_count) {
  if (!generic_struct || generic_struct->type != AST_STRUCT) {
    return NULL;
  }
  
  GenericContext *ctx = generic_context_create();
  
  for (size_t i = 0; i < arg_count && i < generic_struct->data.struct_decl.type_param_count; i++) {
    generic_context_add_param(ctx, generic_struct->data.struct_decl.type_params[i],
                               type_args[i]);
  }
  
  // Clone struct
  ASTNode *instance = (ASTNode *)malloc(sizeof(ASTNode));
  memcpy(instance, generic_struct, sizeof(ASTNode));
  
  // Substitute field types
  if (instance->data.struct_decl.field_count > 0) {
    instance->data.struct_decl.fields = (ASTNode **)malloc(
      instance->data.struct_decl.field_count * sizeof(ASTNode*));
    
    for (size_t i = 0; i < instance->data.struct_decl.field_count; i++) {
      ASTNode *field = (ASTNode *)malloc(sizeof(ASTNode));
      memcpy(field, generic_struct->data.struct_decl.fields[i], sizeof(ASTNode));
      
      field->data.var_decl.var_type = 
        generic_substitute(generic_struct->data.struct_decl.fields[i]->data.var_decl.var_type, ctx);
      
      instance->data.struct_decl.fields[i] = field;
    }
  }
  
  // Update name
  instance->data.struct_decl.name = mangle_generic_name(generic_struct->data.struct_decl.name,
                                                         type_args, arg_count);
  
  // Clear type parameters
  instance->data.struct_decl.type_params = NULL;
  instance->data.struct_decl.type_param_count = 0;
  
  generic_context_destroy(ctx);
  return instance;
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE INFERENCE
// ═══════════════════════════════════════════════════════════════════════════

TypeInference *type_inference_create(void) {
  TypeInference *inf = (TypeInference *)calloc(1, sizeof(TypeInference));
  return inf;
}

void type_inference_destroy(TypeInference *inf) {
  if (!inf) return;
  
  for (size_t i = 0; i < inf->constraint_count; i++) {
    type_destroy(inf->constraints[i]);
  }
  free(inf->constraints);
  
  if (inf->inferred_type) {
    type_destroy(inf->inferred_type);
  }
  
  free(inf);
}

void type_inference_add_constraint(TypeInference *inf, Type *constraint) {
  if (!inf || !constraint) return;
  
  inf->constraints = (Type **)realloc(inf->constraints,
                                       (inf->constraint_count + 1) * sizeof(Type*));
  inf->constraints[inf->constraint_count++] = constraint;
}

bool type_inference_solve(TypeInference *inf) {
  if (!inf || inf->constraint_count == 0) {
    return false;
  }
  
  // Simple inference: if all constraints are equal, that's the type
  Type *first = inf->constraints[0];
  
  for (size_t i = 1; i < inf->constraint_count; i++) {
    if (!type_equals(first, inf->constraints[i])) {
      return false;  // Conflicting constraints
    }
  }
  
  inf->inferred_type = type_copy(first);
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// NAME MANGLING
// ═══════════════════════════════════════════════════════════════════════════

char *mangle_generic_name(const char *base_name, Type **type_args, size_t arg_count) {
  if (!base_name) return NULL;
  
  // Simple mangling: Base_Type1_Type2_...
  size_t buffer_size = strlen(base_name) + 1;
  
  for (size_t i = 0; i < arg_count; i++) {
    char *type_str = type_to_string(type_args[i]);
    buffer_size += strlen(type_str) + 1;
    free(type_str);
  }
  
  char *mangled = (char *)malloc(buffer_size);
  strcpy(mangled, base_name);
  
  for (size_t i = 0; i < arg_count; i++) {
    strcat(mangled, "_");
    char *type_str = type_to_string(type_args[i]);
    strcat(mangled, type_str);
    free(type_str);
  }
  
  return mangled;
}

char *demangle_name(const char *mangled_name) {
  if (!mangled_name) return NULL;
  
  // Find first underscore (simple demangling)
  const char *underscore = strchr(mangled_name, '_');
  if (!underscore) {
    return strdup(mangled_name);
  }
  
  size_t base_len = underscore - mangled_name;
  char *base = (char *)malloc(base_len + 1);
  memcpy(base, mangled_name, base_len);
  base[base_len] = '\0';
  
  return base;
}
