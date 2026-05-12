# Physics Import System Analysis — Complete Summary

## Task Completion

Examined Nova compiler's import/module system to understand how to add `import physics;` support with physics constants (c, h, k_B, etc.) exposed as `qty<f64, unit>` types.

---

## Key Findings

### 1. Import Parsing ✓ (Already Working)
- **File:** `src/compiler/parser.c`, line 2357+
- **Status:** COMPLETE — parser already handles:
  - `import physics;`
  - `import physics::{c, h};`
  - `import physics::*;`
  - `import physics as phys;`
- **Output:** Creates `STMT_IMPORT` with module_name, imports[], import_count, alias

### 2. Import Semantic Analysis ✗ (Missing Implementation)
- **File:** `src/compiler/semantic.c`, line 990-1340
- **Status:** NOT IMPLEMENTED — switch statement has NO case for `STMT_IMPORT`
- **Required:** Add case handler to `analyze_statement_with_context()`
- **Action:** Insert `case STMT_IMPORT: return handle_import_statement(...);`

### 3. Symbol Registration ✓ (Pattern Available)
- **File:** `src/compiler/semantic.c`, line 195-213, 1349-1352
- **Status:** READY TO USE — `symbol_table_add()` function available
- **Pattern:** Create type → add to table → free original (table clones)
- **Example:** `symbol_table_add(table, "c", nova_type_qty(nova_type_f64(), "m/s"), false);`

### 4. Type System ✓ (Full Support)
- **File:** `include/compiler/ast.h`, line 50, 320
- **Status:** Complete — `TYPE_QTY` enum and `nova_type_qty()` constructor available
- **Features:** Full dimensional analysis, unit conversion with `in` operator
- **Unit expressions:** Flexible string-based notation (e.g., "m/s", "J*s", "m^3/(kg*s^2)")

### 5. Physics Module ✓ (Source Exists)
- **Location:** `zn/stdlib/physics/core/constants.zn`
- **Status:** Constants defined but not integrated into compiler
- **Constants:** c, h, hbar, k_B, G, e, N_A with high precision

---

## Implementation Summary

### Files to Modify
1. **`src/compiler/semantic.c`** (single file change)
   - Add helper: `register_physics_constant()`
   - Add handler: `handle_import_statement()`
   - Add case in switch: `case STMT_IMPORT:`

### Code Requirements
- ~120 lines of C code (see `PHYSICS_MODULE_IMPLEMENTATION_CODE.c`)
- Two new functions in semantic.c
- One case addition to existing switch statement

### Integration Points

| What | Where | How |
|------|-------|-----|
| Import handling | `analyze_statement_with_context()` line ~1330 | Add `case STMT_IMPORT:` |
| Register constants | New function `handle_import_statement()` | Check module name, iterate imports |
| Add to symbol table | Use existing `symbol_table_add()` | Call with qty type |
| Create qty type | Use existing `nova_type_qty()` | Pass f64 and unit string |

---

## Physics Constants to Register

**All with type `qty<f64, unit>`:**

| Name | Unit Expression | CODATA Value |
|------|-----------------|--------------|
| `c` | `m/s` | 2.99792458e8 |
| `h` | `J*s` | 6.62607015e-34 |
| `hbar` | `J*s` | 1.054571817e-34 |
| `k_B` | `J/K` | 1.380649e-23 |
| `G` | `m^3/(kg*s^2)` | 6.67430e-11 |
| `e` | `C` | 1.602176634e-19 |
| `N_A` | `1/mol` | 6.02214076e23 |

---

## Exact File Locations and Line Numbers

### Core Files

| File | Lines | Purpose |
|------|-------|---------|
| `include/compiler/ast.h` | 235 | `STMT_IMPORT` enum value |
| `include/compiler/ast.h` | 277-282 | `import_stmt` struct definition |
| `includeast.h` | 50 | `TYPE_QTY` enum value |
| `include/compiler/ast.h` | 69-73 | `qty` union in nova_type |
| `include/compiler/ast.h` | 320 | `nova_type_qty()` declaration |
| `src/compiler/parser.c` | 159 | `parse_import_declaration()` declaration |
| `src/compiler/parser.c` | 2357 | `parse_import_declaration()` implementation |
| `src/compiler/semantic.c` | 195-213 | `symbol_table_add()` implementation |
| `src/compiler/semantic.c` | 215-224 | `symbol_table_lookup()` implementation |
| `src/compiler/semantic.c` | 1349-1352 | Builtin registration pattern |
| `src/compiler/semantic.c` | 990-1340 | `analyze_statement_with_context()` — **WHERE TO ADD CASE** |
| `zn/stdlib/physics/core/constants.zn` | All | Physics constants source reference |

---

## What Gets Built

After implementation, user code:

```nova
import physics;

fn main() {
    let speed: qty<f64, m/s> = c;      // c has type qty<f64, "m/s">
    let planck: qty<f64, J*s> = h;     // h has type qty<f64, "J*s">
    let boltzmann = k_B;                // Type inference: qty<f64, "J/K">
}
```

**Symbol table will contain:**
- `c: qty<f64, "m/s">`
- `h: qty<f64, "J*s">`
- `hbar: qty<f64, "J*s">`
- `k_B: qty<f64, "J/K">`
- ... (and others based on import form)

---

## Documents Generated

1. **PHYSICS_IMPORT_SYSTEM_ANALYSIS.md** — Detailed 200+ line analysis
2. **IMPLEMENTATION_QUICK_REFERENCE.md** — Quick lookup table
3. **PHYSICS_MODULE_IMPLEMENTATION_CODE.c** — Ready-to-use C code
4. **SUMMARY.md** — This file

---

## Next Action

**To implement:**
1. Copy `register_physics_constant()` and `handle_import_statement()` from `PHYSICS_MODULE_IMPLEMENTATION_CODE.c`
2. Paste into `src/compiler/semantic.c` after line 1340
3. Add `case STMT_IMPORT:` handler in switch statement (~line 1330)
4. Compile and test with `import physics;` statements

**Estimated effort:** 30 minutes (copy/paste + minor adjustments)

**Test immediately with:**
```nova
import physics;
fn main() { println(c); }
```

