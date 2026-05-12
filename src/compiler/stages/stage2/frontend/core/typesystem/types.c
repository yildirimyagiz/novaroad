#include "../../../../../../../include/system/nova_common.h"

/**
 * Nova Type System Implementation
 */

#include "../../../../../../../include/compiler/nova_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef nova_type_t Type;
typedef nova_type_kind_t TypeKind;

// Forward declarations
bool type_is_integer(const Type *type);
bool type_is_float(const Type *type);

// ═══════════════════════════════════════════════════════════════════════════
// TYPE SIZE LOOKUP TABLE
// ═══════════════════════════════════════════════════════════════════════════

static size_t primitive_sizes[] = {
  [SEM_TYPE_VOID] = 0,
  [SEM_TYPE_BOOL] = 1,
  [SEM_TYPE_I8] = 1, [SEM_TYPE_U8] = 1,
  [SEM_TYPE_I16] = 2, [SEM_TYPE_U16] = 2,
  [SEM_TYPE_I32] = 4, [SEM_TYPE_U32] = 4,
  [SEM_TYPE_I64] = 8, [SEM_TYPE_U64] = 8,
  [SEM_TYPE_F16] = 2,
  [SEM_TYPE_F32] = 4,
  [SEM_TYPE_F64] = 8,
  [SEM_TYPE_CHAR] = 4,  // UTF-32
};

static size_t primitive_alignments[] = {
  [SEM_TYPE_VOID] = 1,
  [SEM_TYPE_BOOL] = 1,
  [SEM_TYPE_I8] = 1, [SEM_TYPE_U8] = 1,
  [SEM_TYPE_I16] = 2, [SEM_TYPE_U16] = 2,
  [SEM_TYPE_I32] = 4, [SEM_TYPE_U32] = 4,
  [SEM_TYPE_I64] = 8, [SEM_TYPE_U64] = 8,
  [SEM_TYPE_F16] = 2,
  [SEM_TYPE_F32] = 4,
  [SEM_TYPE_F64] = 8,
  [SEM_TYPE_CHAR] = 4,
};

// ═══════════════════════════════════════════════════════════════════════════
// TYPE CREATION
// ═══════════════════════════════════════════════════════════════════════════

Type *type_create(TypeKind kind) {
  Type *type = (Type *)calloc(1, sizeof(Type));
  if (!type) return NULL;
  
  type->kind = kind;
  type->data.name = NULL;
  
  // Set size and alignment for primitives
  if (kind <= SEM_TYPE_CHAR) {
    type->size = primitive_sizes[kind];
    type->alignment = primitive_alignments[kind];
  } else if (kind == SEM_TYPE_STRING) {
    // String is a fat pointer (ptr + len)
    type->size = sizeof(void*) + sizeof(size_t);
    type->alignment = sizeof(void*);
  } else if (kind == SEM_TYPE_REFERENCE || kind == SEM_TYPE_MUT_REFERENCE) {
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
    type->data.name = strdup(name);
  }
  return type;
}

Type *type_create_array(Type *element_type, size_t size) {
  Type *type = type_create(SEM_TYPE_ARRAY);
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
  Type *type = type_create(SEM_TYPE_SLICE);
  if (!type) return NULL;
  
  type->data.slice.element_type = element_type;
  type->data.slice.is_mutable = is_mutable;
  
  // Slice is fat pointer: (ptr, len)
  type->size = sizeof(void*) + sizeof(size_t);
  type->alignment = sizeof(void*);
  
  return type;
}

Type *type_create_tuple(Type **element_types, size_t count) {
  Type *type = type_create(SEM_TYPE_TUPLE);
  if (!type) return NULL;
  
  type->data.tuple.elements = element_types;
  type->data.tuple.count = count;
  
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
  Type *type = type_create(SEM_TYPE_STRUCT);
  if (!type) return NULL;
  
  type->data.name = name ? strdup(name) : NULL;
  type->data.struct_data.name = type->data.name;
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
  Type *type = type_create(SEM_TYPE_ENUM);
  if (!type) return NULL;
  
  type->data.name = name ? strdup(name) : NULL;
  type->data.enum_data.name = type->data.name;
  type->data.enum_data.variants = variants;
  type->data.enum_data.variant_count = variant_count;
  type->data.enum_data.discriminant_type = type_create(SEM_TYPE_U32);
  
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
  Type *type = type_create(SEM_TYPE_FUNCTION);
  if (!type) return NULL;
  
  type->data.fn.params = param_types;
  type->data.fn.param_count = param_count;
  type->data.fn.return_type = return_type;
  type->data.fn.is_variadic = false;
  type->data.fn.is_unsafe = false;
  type->data.fn.is_extern = false;
  type->data.fn.abi = NULL;
  
  // Function pointers are pointer-sized
  type->size = sizeof(void*);
  type->alignment = sizeof(void*);
  
  return type;
}

Type *type_create_reference(Type *referenced_type, bool is_mutable) {
  Type *type = type_create(is_mutable ? SEM_TYPE_MUT_REFERENCE : SEM_TYPE_REFERENCE);
  if (!type) return NULL;
  
  type->data.reference.referenced_type = referenced_type;
  type->data.reference.is_mutable = is_mutable;
  type->data.reference.lifetime_id = 0;  // Default to 'static
  
  type->size = sizeof(void*);
  type->alignment = sizeof(void*);
  
  return type;
}

Type *type_create_generic(const char *base_name, Type **type_args, size_t arg_count) {
  Type *type = type_create(SEM_TYPE_GENERIC);
  if (!type) return NULL;
  
  type->data.generic.name = base_name ? strdup(base_name) : NULL;
  type->data.generic.base_type = NULL;
  type->data.generic.args = type_args;
  type->data.generic.arg_count = arg_count;
  
  return type;
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

Type *type_copy(const Type *type) {
  if (!type) return NULL;
  
  Type *copy = (Type *)malloc(sizeof(Type));
  memcpy(copy, type, sizeof(Type));
  
  if (type->data.name) {
    copy->data.name = strdup(type->data.name);
  }
  
  // Deep copy based on kind
  switch (type->kind) {
    case SEM_TYPE_ARRAY:
      copy->data.array.element_type = type_copy(type->data.array.element_type);
      break;
    case SEM_TYPE_FUNCTION:
      copy->data.fn.return_type = type_copy(type->data.fn.return_type);
      if (type->data.fn.param_count > 0) {
        copy->data.fn.params = (Type **)malloc(
          type->data.fn.param_count * sizeof(Type*));
        for (size_t i = 0; i < type->data.fn.param_count; i++) {
          copy->data.fn.params[i] = type_copy(type->data.fn.params[i]);
        }
      }
      break;
    case SEM_TYPE_REFERENCE:
    case SEM_TYPE_MUT_REFERENCE:
      copy->data.reference.referenced_type = type_copy(type->data.reference.referenced_type);
      break;
    default:
      break;
  }
  
  return copy;
}

void type_destroy(Type *type) {
  if (!type) return;
  
  if (type->data.name) {
    free(type->data.name);
  }
  
  switch (type->kind) {
    case SEM_TYPE_ARRAY:
      type_destroy(type->data.array.element_type);
      break;
    case SEM_TYPE_FUNCTION:
      type_destroy(type->data.fn.return_type);
      for (size_t i = 0; i < type->data.fn.param_count; i++) {
        type_destroy(type->data.fn.params[i]);
      }
      free(type->data.fn.params);
      break;
    case SEM_TYPE_REFERENCE:
    case SEM_TYPE_MUT_REFERENCE:
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
    case SEM_TYPE_ARRAY:
      return type_equals(a->data.array.element_type, b->data.array.element_type) &&
             a->data.array.size == b->data.array.size;
    
    case SEM_TYPE_FUNCTION:
      if (!type_equals(a->data.fn.return_type, b->data.fn.return_type))
        return false;
      if (a->data.fn.param_count != b->data.fn.param_count)
        return false;
      for (size_t i = 0; i < a->data.fn.param_count; i++) {
        if (!type_equals(a->data.fn.params[i], b->data.fn.params[i]))
          return false;
      }
      return true;
    
    case SEM_TYPE_STRUCT:
    case SEM_TYPE_ENUM:
      return a->data.name && b->data.name && strcmp(a->data.name, b->data.name) == 0;
    
    case SEM_TYPE_REFERENCE:
    case SEM_TYPE_MUT_REFERENCE:
      return type_equals(a->data.reference.referenced_type, b->data.reference.referenced_type);
    
    default:
      return true;  // Primitives are equal if kinds match
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// TYPE QUERIES
// ═══════════════════════════════════════════════════════════════════════════

bool type_is_primitive(const Type *type) {
  return type && type->kind >= SEM_TYPE_VOID && type->kind <= SEM_TYPE_STRING;
}

bool type_is_numeric(const Type *type) {
  return type_is_integer(type) || type_is_float(type);
}

bool type_is_integer(const Type *type) {
  if (!type) return false;
  return type->kind >= SEM_TYPE_I8 && type->kind <= SEM_TYPE_U64;
}

bool type_is_float(const Type *type) {
  if (!type) return false;
  return type->kind >= SEM_TYPE_F16 && type->kind <= SEM_TYPE_F64;
}

bool type_is_signed(const Type *type) {
  if (!type) return false;
  return type->kind >= SEM_TYPE_I8 && type->kind <= SEM_TYPE_I64;
}

bool type_is_unsigned(const Type *type) {
  if (!type) return false;
  return type->kind >= SEM_TYPE_U8 && type->kind <= SEM_TYPE_U64;
}

bool type_is_reference(const Type *type) {
  if (!type) return false;
  return type->kind == SEM_TYPE_REFERENCE || type->kind == SEM_TYPE_MUT_REFERENCE;
}

bool type_is_pointer(const Type *type) {
  if (!type) return false;
  return type->kind == SEM_TYPE_RAW_POINTER || type_is_reference(type);
}

bool type_is_aggregate(const Type *type) {
  if (!type) return false;
  return type->kind == SEM_TYPE_STRUCT || type->kind == SEM_TYPE_TUPLE || 
         type->kind == SEM_TYPE_ARRAY || type->kind == SEM_TYPE_ENUM;
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
  if (!struct_type || struct_type->kind != SEM_TYPE_STRUCT || !field_name) {
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
  [SEM_TYPE_VOID] = "void",
  [SEM_TYPE_BOOL] = "bool",
  [SEM_TYPE_I8] = "i8", [SEM_TYPE_I16] = "i16", [SEM_TYPE_I32] = "i32", [SEM_TYPE_I64] = "i64",
  [SEM_TYPE_U8] = "u8", [SEM_TYPE_U16] = "u16", [SEM_TYPE_U32] = "u32", [SEM_TYPE_U64] = "u64",
  [SEM_TYPE_F16] = "f16", [SEM_TYPE_F32] = "f32", [SEM_TYPE_F64] = "f64",
  [SEM_TYPE_CHAR] = "char",
  [SEM_TYPE_STRING] = "String",
};

char *type_to_string(const Type *type) {
  if (!type) return strdup("(null)");
  
  char buffer[256];
  
  if (type->kind <= SEM_TYPE_STRING && type_kind_names[type->kind]) {
    snprintf(buffer, sizeof(buffer), "%s", type_kind_names[type->kind]);
  } else if (type->data.name) {
    snprintf(buffer, sizeof(buffer), "%s", type->data.name);
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
    if (type->data.name && strcmp(type->data.name, name) == 0) {
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
