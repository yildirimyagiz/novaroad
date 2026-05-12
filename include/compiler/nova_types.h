/**
 * Nova Type System
 * Core type definitions and type operations
 */

#ifndef NOVA_TYPES_H
#define NOVA_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declarations
typedef struct nova_type nova_type_t;
struct nova_expr;
struct nova_stmt;



// ═══════════════════════════════════════════════════════════════════════════
// TYPE KINDS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  // Primitive types
  SEM_TYPE_VOID,
  SEM_TYPE_BOOL,
  SEM_TYPE_I8,
  SEM_TYPE_I16,
  SEM_TYPE_I32,
  SEM_TYPE_I64,
  SEM_TYPE_U8,
  SEM_TYPE_U16,
  SEM_TYPE_U32,
  SEM_TYPE_U64,
  SEM_TYPE_USIZE, // Alias for U64/size_t
  SEM_TYPE_F16,
  SEM_TYPE_F32,
  SEM_TYPE_F64,
  SEM_TYPE_CHAR,
  SEM_TYPE_STRING,
  SEM_TYPE_STR, // Alias for STRING

  // Complex types
  SEM_TYPE_ARRAY,
  SEM_TYPE_SLICE,
  SEM_TYPE_TUPLE,
  SEM_TYPE_STRUCT,
  SEM_TYPE_ENUM,
  SEM_TYPE_DATA, // Alias for STRUCT or general data
  SEM_TYPE_TRAIT,
  SEM_TYPE_UNION,
  SEM_TYPE_CONTRACT,
  SEM_TYPE_DEPENDENT_PI,



  // Function & closure types
  SEM_TYPE_FUNCTION,
  SEM_TYPE_CLOSURE,
  SEM_TYPE_FN, // Alias for FUNCTION

  // Reference & pointer types
  SEM_TYPE_REFERENCE,
  SEM_TYPE_MUT_REFERENCE,
  SEM_TYPE_RAW_POINTER,
  SEM_TYPE_POINTER, // Alias for RAW_POINTER
  SEM_TYPE_PTR,     // Alias for REFERENCE
  SEM_TYPE_PTR_MUT, // Alias for MUT_REFERENCE

  // Generic & meta types
  SEM_TYPE_PARAMETER, // Generic type parameter (e.g., T)
  SEM_TYPE_VAR,       // Alias for PARAMETER
  SEM_TYPE_GENERIC,   // Generic type instance (e.g., Vec<i32>)
  SEM_TYPE_INFERRED,  // Type to be inferred
  SEM_TYPE_UNKNOWN,   // Unknown/error type

  // Advanced types (Nova-specific)
  SEM_TYPE_QTY,    // Quantity type with dimensions (qty<f64, kg>)
  SEM_TYPE_FLOW,   // Reactive/async flow type (Task<T>, Stream<T>)
  SEM_TYPE_EFFECT, // Effect type (IO<T>, Async<T>)
  SEM_TYPE_TENSOR, // Shape-typed tensor (tensor<f32>[M, N])

  // Special types
  SEM_TYPE_NEVER, // ! - never returns
  SEM_TYPE_ANY,   // Dynamic typing escape hatch
} nova_type_kind_t;

// ═══════════════════════════════════════════════════════════════════════════
// TYPE STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct nova_type nova_type_t;

// Array type data
typedef struct {
  nova_type_t *element_type;
  size_t size; // 0 for dynamic arrays
  bool is_dynamic;
} ArrayTypeData;

// Slice type data
typedef struct {
  nova_type_t *element_type;
  bool is_mutable;
} SliceTypeData;

// Tuple type data
typedef struct {
  nova_type_t **elements; // Match ast.c
  size_t count;          // Match ast.c
} TupleTypeData;

// Struct field
typedef struct {
  char *name;
  nova_type_t *type;
  bool is_public;
  size_t offset; // Offset in bytes
} StructField;

// Struct type data
typedef struct {
  char *name;
  StructField *fields;
  size_t field_count;
  size_t size_bytes; // Total struct size
  size_t alignment;
  bool is_packed;
} StructTypeData;

// Enum variant
typedef struct {
  char *name;
  nova_type_t **data_types; // Associated data types
  size_t data_count;
  size_t tag_value; // Discriminant value
} EnumVariant;

// Enum type data
typedef struct {
  char *name;
  EnumVariant *variants;
  size_t variant_count;
  nova_type_t *discriminant_type; // Type used for tag (usually u32)
} EnumTypeData;

// Function type data
typedef struct {
  nova_type_t **params; // Match ast.c
  size_t param_count;   // Match ast.c
  nova_type_t *return_type;
  bool is_variadic;
  bool is_unsafe;
  bool is_extern;
  char *abi; // e.g., "C", "Rust", "system"
} FunctionTypeData;

// Reference type data
typedef struct {
  nova_type_t *referenced_type;
  bool is_mutable;
  size_t lifetime_id; // For lifetime tracking (0 = 'static)
} ReferenceTypeData;

// Generic type parameter
typedef struct {
  char *name;
  nova_type_t **bounds; // Trait bounds
  size_t bound_count;
  nova_type_t *default_type; // Default type argument
} TypeParameter;

// Generic type instance
typedef struct {
  char *name;           // Match ast.c (was base_name)
  nova_type_t *base_type;
  nova_type_t **args;   // Match ast.c (was type_args)
  size_t arg_count;     // Match ast.c (was type_arg_count)
} GenericTypeData;

// Forward declaration for dimension support
struct nova_dimension;

// Quantity type data (unit algebra)
typedef struct {
  nova_type_t *inner_type; // Match ast.c (was base_type)
  char *unit_expr;        // Match ast.c
  struct nova_dimension *dimension; // Physical dimension (from dimensions.h)
} QtyTypeData;

// Flow type data (reactive/async)
typedef enum {
  SEM_FLOW_SIGNAL,  // Single value that updates
  SEM_FLOW_STREAM,  // Sequence of values
  SEM_FLOW_TASK,    // Async computation (Future<T>)
  SEM_FLOW_CHANNEL, // Message passing channel
} nova_flow_kind_t;

typedef struct {
  nova_flow_kind_t flow_kind; // Match ast.c (was kind)
  nova_type_t *inner; // Match ast.c (was inner_type)
} FlowTypeData;


// Effect type data
typedef enum {
  SEM_EFFECT_IO,
  SEM_EFFECT_ASYNC,
  SEM_EFFECT_STATE,
  SEM_EFFECT_EXCEPTION,
  SEM_EFFECT_NONDET,
  SEM_EFFECT_CUSTOM,
} EffectKind;

typedef struct {
  EffectKind kind;
  nova_type_t *state_type;  // For SEM_EFFECT_STATE
  char *custom_name; // For SEM_EFFECT_CUSTOM
} Effect;

typedef struct {
  Effect effect;
  nova_type_t *inner_type;
} EffectTypeData;

// Tensor shape dimension
typedef enum {
  SEM_SHAPE_DIM_NAMED,    // Named dimension (batch, seq)
  SEM_SHAPE_DIM_CONST,    // Constant size (32, 512)
  SEM_SHAPE_DIM_DYNAMIC,  // Runtime dynamic (?)
  SEM_SHAPE_DIM_SYMBOLIC, // Symbolic variable (N, M)
} ShapeDimKind;

typedef struct {
  ShapeDimKind kind;
  union {
    char *name;   // For NAMED and SYMBOLIC
    int64_t size; // For CONST
  } data;
} ShapeDim;

typedef struct {
  nova_type_t *dtype;     // Element type (f32, f64, i64)
  ShapeDim *shape; // Shape dimensions
  size_t rank;     // Number of dimensions
} TensorTypeData;

// Main Type structure
struct nova_type {
  nova_type_kind_t kind;
  size_t size;      // Size in bytes (0 if unknown/unsized)
  size_t alignment; // Alignment requirement

  union {
    char *name;       // For SEM_TYPE_DATA compatibility (ast.c / semantic.c)
    ArrayTypeData array;
    SliceTypeData slice;
    TupleTypeData tuple;
    StructTypeData struct_data;
    EnumTypeData enum_data;
    FunctionTypeData fn; // Use 'fn' for ast.c compatibility
    ReferenceTypeData reference;
    TypeParameter type_param;
    char *var_name; // Match ast.c
    GenericTypeData generic;
    QtyTypeData qty;       // Quantity type
    FlowTypeData flow;     // Reactive/async type
    EffectTypeData effect; // Effect type
    TensorTypeData tensor; // Tensor type
    struct {
      nova_type_t *base_type;
      struct nova_expr **requires;
      struct nova_expr **ensures;
      size_t num_requires;
      size_t num_ensures;
    } contract; // Match ast.c
    struct {
      nova_type_t *pointee;
      size_t size;
    } ptr; // Compatibility with legacy
  } data;
};




// ═══════════════════════════════════════════════════════════════════════════
// TYPE OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

// Type creation
nova_type_t *nova_type_create(nova_type_kind_t kind);
nova_type_t *nova_type_primitive(nova_type_kind_t kind);
nova_type_t *nova_type_create_named(nova_type_kind_t kind, const char *name);

nova_type_t *nova_type_create_array(nova_type_t *element_type, size_t size);
nova_type_t *nova_type_create_slice(nova_type_t *element_type, bool is_mutable);
nova_type_t *nova_type_create_tuple(nova_type_t **element_types, size_t count);
nova_type_t *nova_type_create_struct(const char *name, StructField *fields,
                         size_t field_count);
nova_type_t *nova_type_create_enum(const char *name, EnumVariant *variants,
                       size_t variant_count);
nova_type_t *nova_type_create_function(nova_type_t **param_types, size_t param_count,
                           nova_type_t *return_type);

// Nova-specific type creators
nova_type_t *nova_type_create_qty(nova_type_t *base_type, struct nova_dimension *dimension);
nova_type_t *nova_type_create_flow(nova_flow_kind_t kind, nova_type_t *inner_type);
nova_type_t *nova_type_create_effect(EffectKind effect_kind, nova_type_t *inner_type);

nova_type_t *nova_type_create_tensor(nova_type_t *dtype, ShapeDim *shape, size_t rank);
nova_type_t *nova_type_create_reference(nova_type_t *referenced_type, bool is_mutable);
nova_type_t *nova_type_create_generic(const char *base_name, nova_type_t **type_args,
                          size_t arg_count);

// Type utilities
nova_type_t *nova_type_copy(const nova_type_t *type);
void nova_type_destroy(nova_type_t *type);
bool nova_type_equals(nova_type_t *a, nova_type_t *b);

size_t nova_type_hash(const nova_type_t *type);

// Type queries
bool nova_type_is_primitive(const nova_type_t *type);
bool nova_type_is_numeric(const nova_type_t *type);
bool nova_type_is_integer(const nova_type_t *type);
bool nova_type_is_float(const nova_type_t *type);
bool nova_type_is_signed(const nova_type_t *type);
bool nova_type_is_qty(const nova_type_t *type);
bool nova_type_is_flow(const nova_type_t *type);
bool nova_type_is_effect(const nova_type_t *type);
bool nova_type_is_tensor(const nova_type_t *type);
bool nova_type_is_unsigned(const nova_type_t *type);
bool nova_type_is_reference(const nova_type_t *type);
bool nova_type_is_pointer(const nova_type_t *type);
bool nova_type_is_aggregate(const nova_type_t *type);
bool nova_type_is_copy(const nova_type_t *type); // Can be copied bitwise
bool nova_type_is_move(const nova_type_t *type); // Requires move semantics
bool nova_type_is_zero_sized(const nova_type_t *type);

// Type conversions
bool nova_type_can_convert(const nova_type_t *from, const nova_type_t *to);
nova_type_t *nova_type_get_common(const nova_type_t *a, const nova_type_t *b);
nova_type_t *nova_type_promote(const nova_type_t *type);

// Type layout
size_t nova_type_sizeof(const nova_type_t *type);
size_t nova_type_alignof(const nova_type_t *type);
size_t nova_type_field_offset(const nova_type_t *struct_type, const char *field_name);

// Type printing
const char *nova_type_to_string(nova_type_t *type);
void nova_type_print(nova_type_t *type);



// ═══════════════════════════════════════════════════════════════════════════
// TYPE CONTEXT (for type inference and unification)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct TypeContext {
  nova_type_t **registered_types;
  size_t type_count;
  size_t type_capacity;

  // Type variable substitutions (for generics)
  void *substitutions; // HashMap<TypeVariable, Type>

  // Interned type cache
  void *type_cache; // HashMap<TypeSignature, Type*>
} TypeContext;

TypeContext *type_context_create(void);
void type_context_destroy(TypeContext *ctx);
nova_type_t *type_context_intern(TypeContext *ctx, nova_type_t *type);
nova_type_t *type_context_lookup(TypeContext *ctx, const char *name);
bool type_context_register(TypeContext *ctx, nova_type_t *type);

// Compatibility macros to map legacy TYPE_ prefix to bifurcated SEM_TYPE_ prefix
#define TYPE_VOID SEM_TYPE_VOID
#define TYPE_BOOL SEM_TYPE_BOOL
#define TYPE_I8 SEM_TYPE_I8
#define TYPE_I16 SEM_TYPE_I16
#define TYPE_I32 SEM_TYPE_I32
#define TYPE_I64 SEM_TYPE_I64
#define TYPE_U8 SEM_TYPE_U8
#define TYPE_U16 SEM_TYPE_U16
#define TYPE_U32 SEM_TYPE_U32
#define TYPE_U64 SEM_TYPE_U64
#define TYPE_USIZE SEM_TYPE_USIZE
#define TYPE_F16 SEM_TYPE_F16
#define TYPE_F32 SEM_TYPE_F32
#define TYPE_F64 SEM_TYPE_F64
#define TYPE_CHAR SEM_TYPE_CHAR
#define TYPE_STRING SEM_TYPE_STRING
#define TYPE_STR SEM_TYPE_STR
#define TYPE_ARRAY SEM_TYPE_ARRAY
#define TYPE_SLICE SEM_TYPE_SLICE
#define TYPE_TUPLE SEM_TYPE_TUPLE
#define TYPE_STRUCT SEM_TYPE_STRUCT
#define TYPE_ENUM SEM_TYPE_ENUM
#define TYPE_DATA SEM_TYPE_DATA
#define TYPE_TRAIT SEM_TYPE_TRAIT
#define TYPE_UNION SEM_TYPE_UNION
#define TYPE_CONTRACT SEM_TYPE_CONTRACT
#define TYPE_DEPENDENT_PI SEM_TYPE_DEPENDENT_PI
#define TYPE_FUNCTION SEM_TYPE_FUNCTION
#define TYPE_CLOSURE SEM_TYPE_CLOSURE
#define TYPE_FN SEM_TYPE_FN
#define TYPE_REFERENCE SEM_TYPE_REFERENCE
#define TYPE_MUT_REFERENCE SEM_TYPE_MUT_REFERENCE
#define TYPE_RAW_POINTER SEM_TYPE_RAW_POINTER
#define TYPE_POINTER SEM_TYPE_POINTER
#define TYPE_PTR SEM_TYPE_PTR
#define TYPE_PTR_MUT SEM_TYPE_PTR_MUT
#define TYPE_PARAMETER SEM_TYPE_PARAMETER
#define TYPE_VAR SEM_TYPE_VAR
#define TYPE_GENERIC SEM_TYPE_GENERIC
#define TYPE_INFERRED SEM_TYPE_INFERRED
#define TYPE_UNKNOWN SEM_TYPE_UNKNOWN
#define TYPE_QTY SEM_TYPE_QTY
#define TYPE_FLOW SEM_TYPE_FLOW
#define TYPE_EFFECT SEM_TYPE_EFFECT
#define TYPE_TENSOR SEM_TYPE_TENSOR
#define TYPE_NEVER SEM_TYPE_NEVER
#define TYPE_ANY SEM_TYPE_ANY

#endif // NOVA_TYPES_H

