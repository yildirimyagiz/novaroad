# Unit Conversion Codegen Analysis: 'in' Operator

## 1. Grep Results for Unit-Related Code
**Command:** `grep -n '"in"\|OP_UNIT_SCALE\|OP_UNIT_CONVERT\|OP_POP\|target_unit\|to_scale\|in_convert' src/compiler/codegen.c | head -40`

Key matches:
- **Line 565:** `strcmp(op, "in") == 0` - Entry point for 'in' operator handling
- **Line 580:** `const char *target_unit = NULL;` - Target unit variable declaration
- **Line 587:** `double to_scale = nova_dim_get_scale(to_dim);` - Get scale at compile time
- **Line 594:** `chunk_write_opcode(codegen->chunk, OP_POP, expr->span.line);` - Pop RHS identifier
- **Line 598:** `chunk_write_opcode(codegen->chunk, OP_UNIT_SCALE, expr->span.line);` - Emit inverse scale
- **Line 600:** `value_number(1.0 / to_scale)` - Inverse scale constant (de-normalise)
- **Lines 750-751:** EXPR_UNIT_LITERAL emits OP_UNIT_SCALE with scale factor

---

## 2. EXPR_BINARY 'in' Operator Implementation
**Lines 564-609 in src/compiler/codegen.c:**

```c
} else if (strcmp(op, "in") == 0) {
    /*
     * Unit conversion: `expr in target_unit`
     * e.g. `10.0.km in m`  →  10000.0
     *      `5.0.kg in g`   →  5000.0
     *
     * Strategy (compile-time):
     * The left side already has an SI-normalised value on the stack (from
     * EXPR_UNIT_LITERAL or a qty variable). The right side is the target unit
     * identifier (EXPR_IDENT with unit name like "m", "g", etc.).
     *
     * We compute the to-scale at compile time and emit OP_UNIT_SCALE(1/to_scale)
     * to de-normalise into the target unit.  If to_scale == 1.0 we skip it.
     */
    nova_expr_t *rhs = expr->data.binary.right;
    const char *target_unit = NULL;
    if (rhs->kind == EXPR_IDENT) {
        target_unit = rhs->data.ident;
    }

    if (target_unit) {
        nova_dimension_t *to_dim = nova_dim_parse(target_unit);
        if (to_dim) {
            double to_scale = nova_dim_get_scale(to_dim);
            nova_dim_destroy(to_dim);
            /* Stack already has SI value from left side (EXPR_UNIT_LITERAL handled above).
             * We need to pop the right-side IDENT value that was emitted above — but we
             * skipped generate_expression for RHS here. The generates above already ran
             * both sides, so we need to undo the RHS push. */
            /* Pop the spurious RHS constant (IDENT was pushed as 0) */
            chunk_write_opcode(codegen->chunk, OP_POP, expr->span.line);
            /* Now apply inverse scale: SI_value / to_scale = target_unit_value */
            if (to_scale != 1.0 && !isnan(to_scale) && to_scale != 0.0) {
                chunk_write_opcode(codegen->chunk, OP_UNIT_SCALE, expr->span.line);
                int inv_const = chunk_add_constant(codegen->chunk,
                                                   value_number(1.0 / to_scale));
                chunk_write(codegen->chunk, (uint8_t) inv_const, expr->span.line);
            }
        } else {
            /* Unknown target unit — just divide (keep default OP_DIVIDE fallback) */
            chunk_write_opcode(codegen->chunk, OP_DIVIDE, expr->span.line);
        }
    } else {
        /* RHS is not an ident — runtime divide fallback */
        chunk_write_opcode(codegen->chunk, OP_DIVIDE, expr->span.line);
    }
}
```

---

## 3. EXPR_UNIT_LITERAL Codegen
**Lines 723-756 in src/compiler/codegen.c:**

```c
case EXPR_UNIT_LITERAL: {
    /*
     * Unit literal: e.g. 5.0.kg, 9.81.m/s2
     * Strategy: emit the numeric value as a constant, then if scale_factor != 1.0,
     * emit OP_UNIT_SCALE to convert to SI base units at load time.
     *
     * Examples:
     *   5.0.kg  → value=5.0, unit="kg"  → scale=1.0 → just push 5.0
     *   10.0.km → value=10.0, unit="km" → scale=1000.0 → push 10.0, OP_UNIT_SCALE(1000)
     *   9.81.m  → value=9.81, unit="m"  → scale=1.0 → just push 9.81
     */
    double raw_value = expr->data.unit_literal.value;
    const char *unit  = expr->data.unit_literal.unit;

    /* Push raw value */
    chunk_write_opcode(codegen->chunk, OP_CONSTANT, expr->span.line);
    int val_const = chunk_add_constant(codegen->chunk, value_number(raw_value));
    chunk_write(codegen->chunk, (uint8_t) val_const, expr->span.line);

    /* Apply scale factor if unit has a non-unity scale (e.g. km, cm, g) */
    if (unit && *unit) {
        nova_dimension_t *dim = nova_dim_parse(unit);
        if (dim) {
            double scale = nova_dim_get_scale(dim);
            nova_dim_destroy(dim);
            if (scale != 1.0 && !isnan(scale) && scale != 0.0) {
                /* Emit OP_UNIT_SCALE with scale constant */
                chunk_write_opcode(codegen->chunk, OP_UNIT_SCALE, expr->span.line);
                int scale_const = chunk_add_constant(codegen->chunk, value_number(scale));
                chunk_write(codegen->chunk, (uint8_t) scale_const, expr->span.line);
            }
        }
    }
    break;
}
```

**Key point:** Yes, `10.0.km` emits `OP_UNIT_SCALE(1000.0)` to convert to SI (10000.0 m).

---

## 4. VM Handler for OP_UNIT_SCALE
**Lines 347-361 in src/backend/vm.c:**

```c
case OP_UNIT_SCALE: {
    /* OP_UNIT_SCALE <scale_const_index>
     * Pops a number, multiplies by scale factor, pushes result.
     * Used for: unit literals with non-SI scale (km, cm, g, etc.)
     * and for `in` unit conversion (inverse scale applied). */
    uint8_t scale_idx = READ_BYTE();
    Value scale_val = vm->chunk->constants.values[scale_idx];
    Value operand = pop(vm);
    if (operand.type != VAL_NUMBER || scale_val.type != VAL_NUMBER) {
        vm->error_message = strdup("OP_UNIT_SCALE: operands must be numbers");
        return INTERPRET_RUNTIME_ERROR;
    }
    push(vm, value_number(operand.as.number * scale_val.as.number));
    break;
}
```

---

## 5. Critical Bug Analysis: Order of Evaluation

### The Problem (Lines 535-539)
```c
// Generate operands
if (!generate_expression(codegen, expr->data.binary.left))
    return false;
if (!generate_expression(codegen, expr->data.binary.right))
    return false;
```

**Both LHS and RHS are generated BEFORE the operator-specific code runs.**

For `10.0.km in m`:
1. **Line 536:** Generate LHS (`10.0.km`)
   - Push 10.0 → OP_UNIT_SCALE(1000) → Stack: [10000.0]

2. **Line 538:** Generate RHS (`m`)
   - Push 0 (identifier not found) → Stack: [10000.0, 0]

3. **Line 594:** Pop the spurious RHS
   - OP_POP → Stack: [10000.0]

4. **Line 597-600:** Apply inverse scale (1/1.0 = no-op for 'm')
   - No OP_UNIT_SCALE emitted since to_scale=1.0 → Stack: [10000.0]

### Result: ✓ CORRECT (by accident)
The code **happens to work** because:
- RHS is evaluated but discarded
- For 'm' (SI base), to_scale=1.0, so no inverse scale is applied
- Result: 10000.0 ✓

### But the Logic is Flawed
The comments at **lines 589-592** reveal the issue:
```c
/* Stack already has SI value from left side (EXPR_UNIT_LITERAL handled above).
 * We need to pop the right-side IDENT value that was emitted above — but we
 * skipped generate_expression for RHS here. The generates above already ran
 * both sides, so we need to undo the RHS push. */
```

**The code is aware it has a problem but works around it with OP_POP.**

---

## 6. Codegen Flow Diagram

```
EXPR_BINARY "in" (10.0.km in m)
├── Line 536: generate_expression(LHS: 10.0.km)
│   ├── OP_CONSTANT 10.0
│   └── OP_UNIT_SCALE(1000.0) → stack: [10000.0]
├── Line 538: generate_expression(RHS: m)
│   └── OP_CONSTANT 0 (ident not found) → stack: [10000.0, 0]
├── Line 580: Extract target_unit = "m"
├── Line 594: OP_POP (remove RHS) → stack: [10000.0]
├── Line 587: to_scale = 1.0 (m is SI)
└── Line 596-601: to_scale == 1.0, so NO OP_UNIT_SCALE emitted
    Final stack: [10000.0] ✓
```

---

## 7. Summary

| Question | Answer |
|----------|--------|
| **Does codegen emit OP_UNIT_SCALE for km→m?** | Yes, but for **LHS only** (line 750-751). |
| **For `10.0.km in m`, does stack have 10000.0 after LHS?** | Yes (line 536 + 750-751). |
| **Is RHS ident 'm' generated first?** | No, it's generated **second** (line 538). |
| **Does OP_POP correctly clean it?** | Yes (line 594). |
| **Is the 'in' logic mathematically sound?** | Partially. For 'm' it works, but the workaround (OP_POP) suggests design issues. |
| **Bug in execution order?** | **No, but latent risk.** If RHS identifier lookup returned a non-zero value, or if target_unit parsing failed unexpectedly, the pop might remove the wrong value. |

**Key insight:** The code works for unit conversions but relies on RHS identifier lookup failing (pushing 0), then discarding it. A cleaner approach would skip RHS codegen for 'in' operator entirely, or use a dedicated EXPR_IN node type.
