#include "nova_common.h"

/**
 * Nova Type System Implementation
 */

#include "compiler/nova_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// TYPE SIZE LOOKUP TABLE
// ═══════════════════════════════════════════════════════════════════════════

static size_t primitive_sizes[] = {
  [TYPE_VOID] = 0,
  [TYPE_BOOL] = 1,
  [TYPE_I8] = 1, [TYPE_U8] = 1,
  [TYPE_I16] = 2, [TYPE_U16] = 2,
  [TYPE_I32] = 4, [TYPE_U32] = 4,
  [TYPE_I64] = 8, [TYPE_U64] = 8,
  [TYPE_F16] = 2,
  [TYPE_F32] = 4,
  [TYPE_F64] = 8,
  [TYPE_CHAR] = 4,  // UTF-32
};

static size_t primitive_alignments[] = {
  [TYPE_VOID] = 1,
  [TYPE_BOOL] = 1,
  [TYPE_I8] = 1, [TYPE_U8] = 1,
  [TYPE_I16] = 2, [TYPE_U16] = 2,
  [TYPE_I32] = 4, [TYPE_U32] = 4,
  [TYPE_I64] = 8, [TYPE_U64] = 8,
  [TYPE_F16] = 2,
  [TYPE_F32] = 4,
  [TYPE_F64] = 8,
  [TYPE_CHAR] = 4,
};

// ═══════════════════════════════════════════════════════════════════════════
// TYPE CREATION
// ═══════════════════════════════════════════════════════════════════════════

Type *type_create(TypeKind kind) {
  Type *type = (Type *)calloc(1, sizeof(Type));
  if (!type) return NULL;
  
  type->kind = kind;
  type->name = NULL;
  
  // Set size and alignment for primitives
  if (kind <= TYPE_CHAR) {
    type->size = primitive_sizes[kind];
    type->alignment = primitive_alignments[kind];
  } else if (kind == TYPE_STRING) {
    // String is a fat pointer (ptr + len)
    type->size = sizeof(void*) + sizeof(size_t);
    type->alignment = sizeof(void*);
  } else if (kind == TYPE_REFERENCE || kind == TYPE_MUT_REFERENCE) {
    // References are single pointers
    type->size = sizeof(void*);
    type->alignment = sizeof(void*);
  } else {
    type->size = 0;  // Unknown until resolved
    type->alignment = 1;
  }
  
  return type;
}

Type *type_create_named(TypeKind kind, const char *name) {
  Type *type = type_create(kind);
  if (type && name) {
    type->name = strdup(name);
  }
  return type;
}

Type *type_create_array(Type *element_type, size_t size) {
  Type *type = type_create(TYPE_ARRAY);
  if (!type) return NULL;
  
  type->data.array.element_type = element_type;
  type->data.array.size = size;
  type->data.array.is_dynamic = (size == 0);
  
  if (size > 0 && element_type) {
    type->size = element_type->size * size;
    type->alignment = element_type->alignment;
  }
  
  return type;
}

Type *type_create_slice(Type *element_type, bool is_mutable) {
  Type *type = type_create(TYPE_SLICE);
  if (!type) return NULL;
  
  type->data.slice.element_type = element_type;
  type->data.slice.is_mutable = is_mutable;
  
  // Slice is fat pointer: (ptr, len)
  type->size = sizeof(void*) + sizeof(size_t);
  type->alignment = sizeof(void*);
  
  return type;
}

Type *type_create_tuple(Type **element_types, size_t count) {
  Type *type = type_create(TYPE_TUPLE);
  if (!type) return NULL;
  
  type->data.tuple.element_types = element_types;
  type->data.tuple.element_count = count;
  
  // Calculate size and alignment
  size_t total_size = 0;
  size_t max_align = 1;
  
  for (size_t i = 0; i < count; i++) {
    Type *elem = element_types[i];
    size_t align = elem->alignment;
    
    // Align current position
    total_size = (total_size + align - 1) & ~(align - 1);
    total_size += elem->size;
    
    if (align > max_align) {
      max_align = align;
    }
  }
  
  // Final padding
  total_size = (total_size + max_align - 1) & ~(max_align - 1);
  
  type->size = total_size;
  type->alignment = max_align;
  
  return type;
}

Type *type_create_struct(const char *name, StructField *fields, size_t field_count) {
  Type *type = type_create(TYPE_STRUCT);
  if (!type) return NULL;
  
  type->name = name ? strdup(name) : NULL;
  type->data.struct_data.name = type->name;
  type->data.struct_data.fields = fields;
  type->data.struct_data.field_count = field_count;
  type->data.struct_data.is_packed = false;
  
  // Calculate layout
  size_t offset = 0;
  size_t max_align = 1;
  
  for (size_t i = 0; i < field_count; i++) {
    Type *field_type = fields[i].type;
    size_t align = field_type->alignment;
    
    // Align field
    offset = (offset + align - 1) & ~(align - 1);
    fields[i].offset = offset;
    offset += field_type->size;
    
    if (align > max_align) {
      max_align = align;
    }
  }
  
  // Final struct padding
  offset = (offset + max_align - 1) & ~(max_align - 1);
  
  type->size = offset;
  type->alignment = max_align;
  type->data.struct_data.size_bytes = offset;
  type->data.struct_data.alignment = max_align;
  
  return type;
}

Type *type_create_enum(const char *name, EnumVariant *variants, size_t variant_count) {
  Type *type = type_create(TYPE_ENUM);
  if (!type) return NULL;
  
  type->name = name ? strdup(name) : NULL;
  type->data.enum_data.name = type->name;
  type->data.enum_data.variants = variants;
  type->data.enum_data.variant_count = variant_count;
  type->data.enum_data.discriminant_type = type_create(TYPE_U32);
  
  // Find largest variant
  size_t max_variant_size = 0;
  for (size_t i = 0; i < variant_count; i++) {
    size_t variant_size = 0;
    for (size_t j = 0; j < variants[i].data_count; j++) {
      variant_size += variants[i].data_types[j]->size;
    }
    if (variant_size > max_variant_size) {
      max_variant_size = variant_size;
    }
  }
  
  // Size = discriminant + largest variant
  type->size = type->data.enum_data.discriminant_type->size + max_variant_size;
  type->alignment = 4;  // At least 4 for discriminant
  
  return type;
}

Type *type_create_function(Type **param_types, size_t param_count, Type *return_type) {
  Type *type = type_create(TYPE_FUNCTION);
  if (!type) return NULL;
  
  type->data.function.param_types = param_types;
  type->data.function.param_count = param_count;
  type->data.function.return_type = return_type;
  type->data.function.is_variadic = false;
  type->data.function.is_unsafe = false;
  type->data.function.is_extern = false;
  type->data.function.abi = NULL;
  
  // Function pointers are pointer-sized
  type->size = sizeof(void*);
  type->alignment = sizeof(void*);
  
  return type;
}

Type *type_create_reference(Type *referenced_type, bool is_mutable) {
  Type *type = type_create(is_mutable ? TYPE_MUT_REFERENCE : TYPE_REFERENCE);
  if (!type) return NULL;
  
  type->data.reference.referenced_type = referenced_type;
  type->data.reference.is_mutable = is_mutable;
  type->data.reference.lifetime_id = 0;  // Default to 'static
  
  type->size = sizeof(void*);
  type->alignment = sizeof(void*);
  
  return type;
}

Type *type_create_generic(const char *base_name, Type **type_args, size_t arg_count) {
  Type *type = type_create(TYPE_GENERIC);
  if (!type) return NULL;
  
  type->data.generic.base_name = base_name ? strdup(base_name) : NULL;
  type->data.generic.base_type = NULL;
  type->data.generic.type_args = type_args;
  type->data.generic.type_arg_count = arg_count;
  
  return type;
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

Type *type_copy(const Type *type) {
  if (!type) return NULL;
  
  Type *copy = (Type *)malloc(sizeof(Type));
  memcpy(copy, type, sizeof(Type));
  
  if (type->name) {
    copy->name = strdup(type->name);
  }
  
  // Deep copy based on kind
  switch (type->kind) {
    case TYPE_ARRAY:
      copy->data.array.element_type = type_copy(type->data.array.element_type);
      break;
    case TYPE_FUNCTION:
      copy->data.function.return_type = type_copy(type->data.function.return_type);
      if (type->data.function.param_count > 0) {
        copy->data.function.param_types = (Type **)malloc(
          type->data.function.param_count * sizeof(Type*));
        for (size_t i = 0; i < type->data.function.param_count; i++) {
          copy->data.function.param_types[i] = type_copy(type->data.function.param_types[i]);
        }
      }
      break;
    case TYPE_REFERENCE:
    case TYPE_MUT_REFERENCE:
      copy->data.reference.referenced_type = type_copy(type->data.reference.referenced_type);
      break;
    default:
      break;
  }
  
  return copy;
}

void type_destroy(Type *type) {
  if (!type) return;
  
  if (type->name) {
    free(type->name);
  }
  
  switch (type->kind) {
    case TYPE_ARRAY:
      type_destroy(type->data.array.element_type);
      break;
    case TYPE_FUNCTION:
      type_destroy(type->data.function.return_type);
      for (size_t i = 0; i < type->data.function.param_count; i++) {
        type_destroy(type->data.function.param_types[i]);
      }
      free(type->data.function.param_types);
      break;
    case TYPE_REFERENCE:
    case TYPE_MUT_REFERENCE:
      type_destroy(type->data.reference.referenced_type);
      break;
    default:
      break;
  }
  
  free(type);
}

bool type_equals(const Type *a, const Type *b) {
  if (!a || !b) return false;
  if (a == b) return true;
  if (a->kind != b->kind) return false;
  
  switch (a->kind) {
    case TYPE_ARRAY:
      return type_equals(a->data.array.element_type, b->data.array.element_type) &&
             a->data.array.size == b->data.array.size;
    
    case TYPE_FUNCTION:
      if (!type_equals(a->data.function.return_type, b->data.function.return_type))
        return false;
      if (a->data.function.param_count != b->data.function.param_count)
        return false;
      for (size_t i = 0; i < a->data.function.param_count; i++) {
        if (!type_equals(a->data.function.param_types[i], b->data.function.param_types[i]))
          return false;
      }
      return true;
    
    case TYPE_STRUCT:
    case TYPE_ENUM:
      return a->name && b->name && strcmp(a->name, b->name) == 0;
    
    case TYPE_REFERENCE:
    case TYPE_MUT_REFERENCE:
      return type_equals(a->data.reference.referenced_type, b->data.reference.referenced_type);
    
    default:
      return true;  // Primitives are equal if kinds match
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE QUERIES
// ═══════════════════════════════════════════════════════════════════════════

bool type_is_primitive(const Type *type) {
  return type && type->kind >= TYPE_VOID && type->kind <= TYPE_STRING;
}

bool type_is_numeric(const Type *type) {
  return type_is_integer(type) || type_is_float(type);
}

bool type_is_integer(const Type *type) {
  if (!type) return false;
  return type->kind >= TYPE_I8 && type->kind <= TYPE_U64;
}

bool type_is_float(const Type *type) {
  if (!type) return false;
  return type->kind >= TYPE_F16 && type->kind <= TYPE_F64;
}

bool type_is_signed(const Type *type) {
  if (!type) return false;
  return type->kind >= TYPE_I8 && type->kind <= TYPE_I64;
}

bool type_is_unsigned(const Type *type) {
  if (!type) return false;
  return type->kind >= TYPE_U8 && type->kind <= TYPE_U64;
}

bool type_is_reference(const Type *type) {
  if (!type) return false;
  return type->kind == TYPE_REFERENCE || type->kind == TYPE_MUT_REFERENCE;
}

bool type_is_pointer(const Type *type) {
  if (!type) return false;
  return type->kind == TYPE_RAW_POINTER || type_is_reference(type);
}

bool type_is_aggregate(const Type *type) {
  if (!type) return false;
  return type->kind == TYPE_STRUCT || type->kind == TYPE_TUPLE || 
         type->kind == TYPE_ARRAY || type->kind == TYPE_ENUM;
}

bool type_is_copy(const Type *type) {
  if (!type) return false;
  // Primitives and references can be copied
  return type_is_primitive(type) || type_is_reference(type);
}

bool type_is_move(const Type *type) {
  if (!type) return false;
  // Aggregates require move semantics
  return type_is_aggregate(type);
}

bool type_is_zero_sized(const Type *type) {
  return type && type->size == 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE LAYOUT
// ═══════════════════════════════════════════════════════════════════════════

size_t type_sizeof(const Type *type) {
  return type ? type->size : 0;
}

size_t type_alignof(const Type *type) {
  return type ? type->alignment : 1;
}

size_t type_field_offset(const Type *struct_type, const char *field_name) {
  if (!struct_type || struct_type->kind != TYPE_STRUCT || !field_name) {
    return 0;
  }
  
  for (size_t i = 0; i < struct_type->data.struct_data.field_count; i++) {
    if (strcmp(struct_type->data.struct_data.fields[i].name, field_name) == 0) {
      return struct_type->data.struct_data.fields[i].offset;
    }
  }
  
  return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE PRINTING
// ═══════════════════════════════════════════════════════════════════════════

static const char *type_kind_names[] = {
  [TYPE_VOID] = "void",
  [TYPE_BOOL] = "bool",
  [TYPE_I8] = "i8", [TYPE_I16] = "i16", [TYPE_I32] = "i32", [TYPE_I64] = "i64",
  [TYPE_U8] = "u8", [TYPE_U16] = "u16", [TYPE_U32] = "u32", [TYPE_U64] = "u64",
  [TYPE_F16] = "f16", [TYPE_F32] = "f32", [TYPE_F64] = "f64",
  [TYPE_CHAR] = "char",
  [TYPE_STRING] = "String",
};

char *type_to_string(const Type *type) {
  if (!type) return strdup("(null)");
  
  char buffer[256];
  
  if (type->kind <= TYPE_STRING && type_kind_names[type->kind]) {
    snprintf(buffer, sizeof(buffer), "%s", type_kind_names[type->kind]);
  } else if (type->name) {
    snprintf(buffer, sizeof(buffer), "%s", type->name);
  } else {
    snprintf(buffer, sizeof(buffer), "<type:%d>", type->kind);
  }
  
  return strdup(buffer);
}

void type_print(const Type *type) {
  char *str = type_to_string(type);
  printf("%s", str);
  free(str);
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

TypeContext *type_context_create(void) {
  TypeContext *ctx = (TypeContext *)calloc(1, sizeof(TypeContext));
  if (!ctx) return NULL;
  
  ctx->type_capacity = 32;
  ctx->registered_types = (Type **)calloc(ctx->type_capacity, sizeof(Type*));
  ctx->type_count = 0;
  
  return ctx;
}

void type_context_destroy(TypeContext *ctx) {
  if (!ctx) return;
  
  for (size_t i = 0; i < ctx->type_count; i++) {
    type_destroy(ctx->registered_types[i]);
  }
  
  free(ctx->registered_types);
  free(ctx);
}

bool type_context_register(TypeContext *ctx, Type *type) {
  if (!ctx || !type) return false;
  
  if (ctx->type_count >= ctx->type_capacity) {
    ctx->type_capacity *= 2;
    ctx->registered_types = (Type **)realloc(ctx->registered_types, 
                                              ctx->type_capacity * sizeof(Type*));
  }
  
  ctx->registered_types[ctx->type_count++] = type;
  return true;
}

Type *type_context_lookup(TypeContext *ctx, const char *name) {
  if (!ctx || !name) return NULL;
  
  for (size_t i = 0; i < ctx->type_count; i++) {
    Type *type = ctx->registered_types[i];
    if (type->name && strcmp(type->name, name) == 0) {
      return type;
    }
  }
  
  return NULL;
}

Type *type_context_intern(TypeContext *ctx, Type *type) {
  if (!ctx || !type) return NULL;
  
  // Simple interning: check if equivalent type exists
  for (size_t i = 0; i < ctx->type_count; i++) {
    if (type_equals(ctx->registered_types[i], type)) {
      return ctx->registered_types[i];
    }
  }
  
  // Not found, register it
  type_context_register(ctx, type);
  return type;
}
