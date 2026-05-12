# Nova Compiler: Import/Module System Analysis for Physics Constants

## Executive Summary

To add `import physics;` support with physics constants (c, h, k_B, etc.), the Nova compiler needs:
1. **Import statement handling** in semantic analysis (currently unimplemented for STMT_IMPORT)
2. **Module resolution** to locate and load physics module files
3. **Builtin registration** mechanism similar to how `print` and `println` are injected
4. **Type system support** for qty<f64, m/s> constants with unit algebra

---

## Current Architecture

### 1. AST Structure (include/compiler/ast.h)

**File:** `include/compiler/ast.h`  
**Lines:** 235-283

```c
typedef enum nova_stmt_kind {
    // ... other statements ...
    STMT_IMPORT,           // Line 235: Import statement exists in enum
    // ... other statements ...
} nova_stmt_kind_t;

typedef struct nova_stmt {
    nova_stmt_kind_t kind;
    nova_span_t span;
    union {
        // ... other data ...
        struct {
            char *module_name;      // e.g., "physics"
            char **imports;         // e.g., ["c", "h", "k_B"]
            size_t import_count;    // number of imports
            char *alias;            // optional: "import physics as phys"
        } import_stmt;              // Line 277-282
    } data;
} nova_stmt_t;
```

### 2. Parser Implementation (src/compiler/parser.c)

**File:** `src/compiler/parser.c`  
**Key Functions:**
- Line 159: Declaration of `parse_import_declaration()`
- Line 2310: Parsing MODULE / IMPORT / PUB section
- Line 2357: Implementation of `parse_import_declaration()`
- Lines 2356-2410: Handles three import forms:
  - `import foo::{bar, baz};` — selective imports
  - `import foo::*;` — wildcard import
  - `import foo as f;` — aliased import

**Current Behavior:**
```c
// Line 2357-2410: Parses import statement
// Extracts:
// - module_name (e.g., "physics")
// - imports[] (if selective: ["c", "h"])
// - import_count
// - alias (if present)
```

### 3. Semantic Analysis (src/compiler/semantic.c)

#### Symbol Table Implementation
**File:** `src/compiler/semantic.c`  
**Lines:** 151-224 (symbol_table structure)

```c
typedef struct symbol_table {
    struct symbol *symbols;        // Linked list of symbols
    int next_global_index;         // Global symbol counter
    int next_local_index;          // Local scope counter
    int scope_depth;               // Current scope depth
    int local_index_stack[64];     // Stack for scope management
} symbol_table_t;

// Line 195-213: symbol_table_add()
static void symbol_table_add(symbol_table_t *table, const char *name, 
                             const nova_type_t *type, bool is_mutable)
{
    // Creates symbol with name, type, mutability
    // Type is CLONED (ownership handled safely)
    // Added to global or local scope based on table->scope_depth
}

// Line 215-224: symbol_table_lookup()
static symbol_t *symbol_table_lookup(symbol_table_t *table, const char *name)
{
    // Linear search through symbol linked list
    // Returns NULL if not found
}
```

#### Builtin Registration
**File:** `src/compiler/semantic.c`  
**Lines:** 1349-1352

```c
/* Built-in symbols */
{
    nova_type_t *vt = nova_type_void();
    symbol_table_add(global_table, "print", vt, false);      // Line 1350
    symbol_table_add(global_table, "println", vt, false);    // Line 1351
    nova_type_free(vt);  /* symbol_table_add cloned; free original */
}
```

**Pattern for builtin registration:**
1. Create a `nova_type_t*` for the function/constant
2. Call `symbol_table_add(global_table, "name", type, is_mutable)`
3. Free the original type (table clones it internally)

#### Analysis Flow
**File:** `src/compiler/semantic.c`  
**Lines:** 1449+ (analyze_program)

```c
bool nova_semantic_analyze(nova_semantic_t *semantic)
{
    return analyze_program(semantic, semantic->ast);
}

// Inside analyze_program():
// 1. Line 1349-1352: Register builtins (print, println)
// 2. Line 1360+: PASS 1 — Register all function and enum names
// 3. Line 1410+: PASS 2 — Analyze all statements
}
```

**CRITICAL FINDING:** There is **NO case for STMT_IMPORT** in the statement analysis switch:
```c
switch (stmt->kind) {
    case STMT_VAR_DECL: ...      // Line 998
    case STMT_EXPR: ...           // Line 1038
    case STMT_CHECK: ...          // Line 1046
    case STMT_WHILE: ...          // Line 1067
    case STMT_FOR: ...            // Line 1085
    case STMT_RETURN:             // Line 1115
    case STMT_YIELD: ...          // Line 1116
    case STMT_FN: ...             // Line 1178
    case STMT_BLOCK: ...          // Line 1306
    case STMT_BREAK:              // Line 1316
    case STMT_CONTINUE: ...       // Line 1317
    case STMT_STRUCT_DECL: ...    // Line 1321
    // NO STMT_IMPORT HANDLER!
}
```

---

## Existing Physics Module Structure (zn/stdlib/physics/)

**Directory:** `zn/stdlib/physics/`  
**Contents:**
- `core/constants.zn` — Physical constants with unit types
- `core/units.zn` — Unit definitions
- Subdirectories: astro, classical, cryogenic, electromagnetism, magnetism, materials, numerical, optics, particles, quantum, relativity, thermal, transport

### Physics Constants (zn/stdlib/physics/core/constants.zn)

**Example constants with units:**
```zn
use physics::core::units::*;

pub const SPEED_OF_LIGHT: MeterPerSecond = 299_792_458.0;
pub const HBAR: JouleSecond = 1.054_571_817e-34;
pub const PLANCK: JouleSecond = 6.626_070_15e-34;
pub const ELEMENTARY_CHARGE: Coulomb = 1.602_176_634e-19;
pub const GRAVITATIONAL_CONSTANT: CubicMeterPerKgSecondSquared = 6.674_30e-11;
pub const BOLTZMANN: JoulePerKelvin = 1.380_649e-23;
pub const AVOGADRO: PerMole = 6.022_140_76e23;
```

---

## Type System: qty<T, dim>

**File:** `include/compiler/ast.h`  
**Lines:** 50, 69-73

```c
typedef enum nova_type_kind {
    // ...
    TYPE_QTY      // qty<T, dim> for unit algebra (Line 50)
} nova_type_kind_t;

typedef struct nova_type {
    union {
        struct {
            struct nova_type *inner_type;  // e.g., f64
            char *unit_expr;               // e.g., "m/s"
            void *dimension;               // nova_dimension_t*
        } qty;
    } data;
} nova_type_t;
```

**Constructor:** `nova_type_qty(nova_type_t *base_type, const char *unit_expr)`

---

## Implementation Strategy

### Phase 1: Add STMT_IMPORT Handler
**Location:** `src/compiler/semantic.c` (analyze_statement_with_context)

Add case for STMT_IMPORT that:
1. Checks if module matches "physics"
2. Calls `load_physics_module(global_table, stmt->data.import_stmt)`
3. Handles selective imports vs. wildcard

### Phase 2: Create Physics Module Loader
**New function:** `load_physics_module()` in semantic.c

```c
static void load_physics_module(symbol_table_t *global_table, 
                               nova_stmt_t *import_stmt)
{
    const char *module = import_stmt->data.import_stmt.module_name;
    
    if (strcmp(module, "physics") != 0)
        return;  // Handle other modules later
    
    // Register physics constants
    // For each constant (c, h, k_B, ...):
    //   - Create qty<f64, unit> type
    //   - Add to symbol table
    //   - Mark as immutable, global
}
```

### Phase 3: Register Physics Constants

**Constants to register** (from zn/stdlib/physics/core/constants.zn):

| Name | Value | Type Annotation | qty Type |
|------|-------|-----------------|----------|
| c | 2.998e8 | MeterPerSecond | qty<f64, "m/s"> |
| h | 6.626e-34 | JouleSecond | qty<f64, "J*s"> |
| hbar | 1.055e-34 | JouleSecond | qty<f64, "J*s"> |
| k_B | 1.381e-23 | JoulePerKelvin | qty<f64, "J/K"> |
| G | 6.674e-11 | CubicMeterPerKgSecondSquared | qty<f64, "m^3/(kg*s^2)"> |
| e | 1.602e-19 | Coulomb | qty<f64, "C"> |
| N_A | 6.022e23 | PerMole | qty<f64, "1/mol"> |

**Implementation pattern:**
```c
// Create qty type for speed of light: qty<f64, m/s>
nova_type_t *c_type = nova_type_qty(nova_type_f64(), "m/s");
symbol_table_add(global_table, "c", c_type, false);  // immutable, global
nova_type_free(c_type);
```

### Phase 4: Handle Selective/Wildcard Imports

```c
// For: import physics::{c, h};
if (import_stmt->data.import_stmt.import_count > 0) {
    for (size_t i = 0; i < import_stmt->data.import_stmt.import_count; i++) {
        char *name = import_stmt->data.import_stmt.imports[i];
        // Register only the named constant
    }
}

// For: import physics::*;
else if (/* wildcard detected */) {
    // Register all physics constants
}
```

---

## Critical Files and Line Numbers Summary

| File | Lines | Purpose |
|------|-------|---------|
| `include/compiler/ast.h` | 235, 277-282 | STMT_IMPORT enum and import_stmt data structure |
| `src/compiler/parser.c` | 159, 2310, 2357-2410 | parse_import_declaration() implementation |
| `src/compiler/semantic.c` | 191-224 | symbol_table operations (add, lookup) |
| `src/compiler/semantic.c` | 1349-1352 | Builtin registration pattern |
| `src/compiler/semantic.c` | 1449+ | analyze_program() entry point |
| `src/compiler/semantic.c` | 990-1340 | analyze_statement_with_context() — **WHERE STMT_IMPORT HANDLER GOES** |
| `zn/stdlib/physics/core/constants.zn` | All | Physics constants definitions |

---

## Next Steps

1. **Implement STMT_IMPORT handler** in analyze_statement_with_context()
2. **Create load_physics_module()** function in semantic.c
3. **Register constants** with qty types using nova_type_qty()
4. **Test** with: `import physics; print(c);`

Expected output: Symbol table contains c, h, k_B, etc., all with qty<f64, unit> types.
