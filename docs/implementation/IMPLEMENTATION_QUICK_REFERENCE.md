# Quick Reference: Adding Physics Import to Nova Compiler

## Exact Locations

### 1. Where to add STMT_IMPORT handler
**File:** `src/compiler/semantic.c`  
**Location:** Inside `analyze_statement_with_context()` function, around line 1330 (after STMT_STRUCT_DECL case)  
**Add before:** Final `default:` or at end of switch

```c
case STMT_IMPORT:
    return handle_import_statement(semantic, stmt, table);
```

---

### 2. Symbol Table API
**File:** `src/compiler/semantic.c`

```c
// Line 195-213: Add symbols
static void symbol_table_add(symbol_table_t *table, const char *name, 
                             const nova_type_t *type, bool is_mutable)

// Pattern:
nova_type_t *qty_type = nova_type_qty(nova_type_f64(), "m/s");
symbol_table_add(global_table, "c", qty_type, false);  // immutable
nova_type_free(qty_type);
```

---

### 3. Type Constructor for Physics Constants
**File:** `include
**Available API:**

```c
// For qty types (from line 320):
nova_type_t *nova_type_qty(nova_type_t *base_type, const char *unit_expr);

// Primitive types:
nova_type_t *nova_type_f64(void);
nova_type_t *nova_type_i64(void);
nova_type_t *nova_type_clone(const nova_type_t *type);
void nova_type_free(nova_type_t *type);
```

---

### 4. Import Statement Structure
**File:** `include

```c
struct {
    char *module_name;      // "physics"
    char **imports;         // ["c", "h"] or NULL for wildcard
    size_t import_count;    // 2 or 0 for wildcard
    char *alias;            // "phys" if: import physics as phys
} import_stmt;
```

---

### 5. Built-in Registration Pattern
**File:** `src/compiler/semantic.c` (Lines 1349-1352)

This shows how to inject global constants:
```c
{
    nova_type_t *vt = nova_type_void();
    symbol_table_add(global_table, "print", vt, false);
    symbol_table_add(global_table, "println", vt, false);
    nova_type_free(vt);
}
```

---

## Physics Constants to Register

From `zn/stdlib/physics/core/constants.zn`:

| Constant | Value | Unit Type | qty Expression |
|----------|-------|-----------|-----------------|
| `c` | 2.998e8 | m/s | `nova_type_qty(nova_type_f64(), "m/s")` |
| `h` | 6.626e-34 | J·s | `nova_type_qty(nova_type_f64(), "J*s")` |
| `hbar` | 1.055e-34 | J·s | `nova_type_qty(nova_type_f64(), "J*s")` |
| `k_B` | 1.381e-23 | J/K | `nova_type_qty(nova_type_f64(), "J/K")` |
| `G` | 6.674e-11 | m³/(kg·s²) | `nova_type_qty(nova_type_f64(), "m^3/(kg*s^2)")` |
| `e` | 1.602e-19 | C | `nova_type_qty(nova_type_f64(), "C")` |
| `N_A` | 6.022e23 | /mol | `nova_type_qty(nova_type_f64(), "1/mol")` |

---

## Pseudo-code for Implementation

```c
static bool handle_import_statement(nova_semantic_t *semantic, nova_stmt_t *stmt,
                                    symbol_table_t *table)
{
    const char *module = stmt->data.import_stmt.module_name;
    
    if (strcmp(module, "physics") == 0) {
        // Register physics constants based on import_count
        // If import_count == 0 → wildcard (register all)
        // If import_count > 0 → register only listed imports
        
        // Example for wildcard:
        {
            nova_type_t *c_type = nova_type_qty(nova_type_f64(), "m/s");
            symbol_table_add(table, "c", c_type, false);
            nova_type_free(c_type);
            
            // ... repeat for h, k_B, etc.
        }
        
        return true;
    }
    
    // Handle other modules here
    return true;
}
```

---

## Test Case

After implementation, this should work:

```nova
import physics;

fn main() {
    let speed: qty<f64, m/s> = c;
    println(speed);
}
```

Expected: Symbol table lookup for "c" returns qty<f64, "m/s"> type

---

## Key Design Decisions

1. **When to register:** In `analyze_statement_with_context()` during PASS 2 (statement analysis)
2. **Scope:** Global table only (physics constants are global)
3. **Mutability:** All physics constants are `false` (immutable)
4. **Type cloning:** `symbol_table_add()` clones types, so always free the original
5. **Wildcard detection:** Check `import_count == 0` (parser sets this for `import physics::*;`)
