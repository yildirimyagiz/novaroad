/**
 * Nova Type System
 * Core type definitions and type operations
 */

#ifndef NOVA_TYPES_H
#define NOVA_TYPES_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// TYPE KINDS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  // Primitive types
  TYPE_VOID,
  TYPE_BOOL,
  TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64,
  TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64,
  TYPE_F16, TYPE_F32, TYPE_F64,
  TYPE_CHAR,
  TYPE_STRING,
  
  // Complex types
  TYPE_ARRAY,
  TYPE_SLICE,
  TYPE_TUPLE,
  TYPE_STRUCT,
  TYPE_ENUM,
  TYPE_TRAIT,
  TYPE_UNION,
  
  // Function & closure types
  TYPE_FUNCTION,
  TYPE_CLOSURE,
  
  // Reference & pointer types
  TYPE_REFERENCE,
  TYPE_MUT_REFERENCE,
  TYPE_RAW_POINTER,
  
  // Generic & meta types
  TYPE_PARAMETER,    // Generic type parameter (e.g., T)
  TYPE_GENERIC,      // Generic type instance (e.g., Vec<i32>)
  TYPE_INFERRED,     // Type to be inferred
  TYPE_UNKNOWN,      // Unknown/error type
  
  // Special types
  TYPE_NEVER,        // ! - never returns
  TYPE_ANY,          // Dynamic typing escape hatch
} TypeKind;

// ═══════════════════════════════════════════════════════════════════════════
// TYPE STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct Type Type;

// Array type data
typedef struct {
  Type *element_type;
  size_t size;          // 0 for dynamic arrays
  bool is_dynamic;
} ArrayTypeData;

// Slice type data  
typedef struct {
  Type *element_type;
  bool is_mutable;
} SliceTypeData;

// Tuple type data
typedef struct {
  Type **element_types;
  size_t element_count;
} TupleTypeData;

// Struct field
typedef struct {
  char *name;
  Type *type;
  bool is_public;
  size_t offset;        // Offset in bytes
} StructField;

// Struct type data
typedef struct {
  char *name;
  StructField *fields;
  size_t field_count;
  size_t size_bytes;    // Total struct size
  size_t alignment;
  bool is_packed;
} StructTypeData;

// Enum variant
typedef struct {
  char *name;
  Type **data_types;    // Associated data types
  size_t data_count;
  size_t tag_value;     // Discriminant value
} EnumVariant;

// Enum type data
typedef struct {
  char *name;
  EnumVariant *variants;
  size_t variant_count;
  Type *discriminant_type;  // Type used for tag (usually u32)
} EnumTypeData;

// Function type data
typedef struct {
  Type **param_types;
  size_t param_count;
  Type *return_type;
  bool is_variadic;
  bool is_unsafe;
  bool is_extern;
  char *abi;            // e.g., "C", "Rust", "system"
} FunctionTypeData;

// Reference type data
typedef struct {
  Type *referenced_type;
  bool is_mutable;
  size_t lifetime_id;   // For lifetime tracking (0 = 'static)
} ReferenceTypeData;

// Generic type parameter
typedef struct {
  char *name;
  Type **bounds;        // Trait bounds
  size_t bound_count;
  Type *default_type;   // Default type argument
} TypeParameter;

// Generic type instance
typedef struct {
  char *base_name;      // e.g., "Vec"
  Type *base_type;      // The generic definition
  Type **type_args;     // e.g., [i32] for Vec<i32>
  size_t type_arg_count;
} GenericTypeData;

// Main Type structure
struct Type {
  TypeKind kind;
  char *name;           // Optional name (for named types)
  size_t size;          // Size in bytes (0 if unknown/unsized)
  size_t alignment;     // Alignment requirement
  
  union {
    ArrayTypeData array;
    SliceTypeData slice;
    TupleTypeData tuple;
    StructTypeData struct_data;
    EnumTypeData enum_data;
    FunctionTypeData function;
    ReferenceTypeData reference;
    TypeParameter type_param;
    GenericTypeData generic;
  } data;
};

// ═══════════════════════════════════════════════════════════════════════════
// TYPE OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

// Type creation
Type *type_create(TypeKind kind);
Type *type_create_named(TypeKind kind, const char *name);
Type *type_create_array(Type *element_type, size_t size);
Type *type_create_slice(Type *element_type, bool is_mutable);
Type *type_create_tuple(Type **element_types, size_t count);
Type *type_create_struct(const char *name, StructField *fields, size_t field_count);
Type *type_create_enum(const char *name, EnumVariant *variants, size_t variant_count);
Type *type_create_function(Type **param_types, size_t param_count, Type *return_type);
Type *type_create_reference(Type *referenced_type, bool is_mutable);
Type *type_create_generic(const char *base_name, Type **type_args, size_t arg_count);

// Type utilities
Type *type_copy(const Type *type);
void type_destroy(Type *type);
bool type_equals(const Type *a, const Type *b);
size_t type_hash(const Type *type);

// Type queries
bool type_is_primitive(const Type *type);
bool type_is_numeric(const Type *type);
bool type_is_integer(const Type *type);
bool type_is_float(const Type *type);
bool type_is_signed(const Type *type);
bool type_is_unsigned(const Type *type);
bool type_is_reference(const Type *type);
bool type_is_pointer(const Type *type);
bool type_is_aggregate(const Type *type);
bool type_is_copy(const Type *type);        // Can be copied bitwise
bool type_is_move(const Type *type);        // Requires move semantics
bool type_is_zero_sized(const Type *type);

// Type conversions
bool type_can_convert(const Type *from, const Type *to);
Type *type_get_common(const Type *a, const Type *b);
Type *type_promote(const Type *type);

// Type layout
size_t type_sizeof(const Type *type);
size_t type_alignof(const Type *type);
size_t type_field_offset(const Type *struct_type, const char *field_name);

// Type printing
char *type_to_string(const Type *type);
void type_print(const Type *type);

// ═══════════════════════════════════════════════════════════════════════════
// TYPE CONTEXT (for type inference and unification)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct TypeContext {
  Type **registered_types;
  size_t type_count;
  size_t type_capacity;
  
  // Type variable substitutions (for generics)
  void *substitutions;  // HashMap<TypeVariable, Type>
  
  // Interned type cache
  void *type_cache;     // HashMap<TypeSignature, Type*>
} TypeContext;

TypeContext *type_context_create(void);
void type_context_destroy(TypeContext *ctx);
Type *type_context_intern(TypeContext *ctx, Type *type);
Type *type_context_lookup(TypeContext *ctx, const char *name);
bool type_context_register(TypeContext *ctx, Type *type);

#endif // NOVA_TYPES_H
