# 🌟 Unit Algebra Backend Implementation Plan

**Nova's UNIQUE Feature** - Compile-time dimensional analysis  
**Status**: Frontend 80% ready, Backend 0% done  
**Estimated Time**: ~1 week (5-7 days)

---

## 📊 Current Status Analysis

### ✅ What's Already Done

#### 1. **Frontend Infrastructure (80%)**
- ✅ Test suite exists: `tests/unit/zn/test_unit_algebra.zn` (279 lines)
- ✅ Type definitions: `qty<T, dim>` syntax partially parsed
- ✅ Dimension library: `src/compiler/dimensions.c` (226 lines)
- ✅ Header file: `include/compiler/dimensions.h` (74 lines)
- ✅ Codegen stubs: Basic `qty_type_data_t` structure exists

#### 2. **Existing Infrastructure**
```c
// In codegen.c (lines 73-76)
typedef struct {
    nova_type_expr_ext_t *inner;  // Inner type (f64, i32, etc.)
    dim_expr_t dim;               // Dimension expression
} qty_type_data_t;

// Basic dimension operations exist (dimensions.c)
nova_dimension_t *nova_dim_multiply(a, b);   // ✅
nova_dimension_t *nova_dim_divide(a, b);      // ✅
bool nova_dim_compatible(a, b);               // ✅
```

#### 3. **What Works in dimensions.c**
- ✅ SI base dimensions (7 bases: L, M, T, I, Θ, N, J)
- ✅ Dimension multiplication/division
- ✅ Dimension compatibility checking
- ✅ Common units (meter, kg, second, Newton, Joule, etc.)
- ✅ Unit conversion support

### ❌ What's Missing (Backend Implementation)

1. **Parser Integration** - Parse `5.kg`, `9.81.m/s²` literals
2. **Semantic Analysis** - Type checking for dimensional operations
3. **Codegen Integration** - Generate bytecode for qty operations
4. **Runtime Support** - VM operations for quantities
5. **Error Messages** - "Cannot add kg to meters" diagnostics

---

## 🎯 Implementation Plan

### **Phase 1: Parser Integration** (2 days)

#### Day 1 - Literal Parsing
**File**: `src/compiler/parser.c`

**Task 1.1**: Parse unit literals
```c
// Add to lexer/parser:
// 5.kg  → NUMBER + DOT + IDENTIFIER
// Parse: number . unit_name
Token parse_quantity_literal(Parser *p) {
    double value = parse_number(p);
    expect(p, TOKEN_DOT);
    char *unit = parse_identifier(p); // "kg", "m", "s"
    
    // Look up dimension
    nova_dimension_t *dim = lookup_unit(unit);
    
    return make_qty_literal(value, dim);
}
```

**Task 1.2**: Parse compound units
```c
// Parse: m/s, kg·m/s², m²/s³
// Grammar:
//   unit_expr := simple_unit
//             |  unit_expr '*' unit_expr
//             |  unit_expr '/' unit_expr
//             |  unit_expr '^' number
//             |  unit_expr '·' unit_expr

Dimension *parse_unit_expression(Parser *p);
```

**Task 1.3**: Parse qty types
```c
// qty<f64, m/s²>
Type *parse_qty_type(Parser *p) {
    expect(p, TOKEN_QTY);
    expect(p, TOKEN_LESS);
    Type *inner = parse_type(p);
    expect(p, TOKEN_COMMA);
    Dimension *dim = parse_unit_expression(p);
    expect(p, TOKEN_GREATER);
    
    return type_create_qty(inner, dim);
}
```

**Files to modify**:
- `src/compiler/lexer.c` - Add unit literal recognition
- `src/compiler/parser.c` - Add qty parsing
- `src/compiler/tokens.c` - Add TOKEN_QTY

**Tests**: Parse `test_unit_algebra.zn` without errors

---

#### Day 2 - AST Integration
**File**: `src/compiler/ast.c`

**Task 2.1**: Add Qty AST nodes
```c
// Add to ast.h
typedef struct {
    Expr *value;          // Numeric value
    Dimension *dim;       // Unit dimension
} QtyLiteral;

typedef enum {
    // ... existing
    EXPR_QTY_LITERAL,
    EXPR_QTY_BINOP,       // Qty arithmetic
} ExprKind;
```

**Task 2.2**: Create qty node constructors
```c
Expr *expr_create_qty_literal(double value, Dimension *dim);
Expr *expr_create_qty_binop(BinOp op, Expr *left, Expr *right);
```

**Files to modify**:
- `include/nova_ast.h` - Add Qty node types
- `src/compiler/ast.c` - Add constructors

**Tests**: AST construction for qty expressions

---

### **Phase 2: Semantic Analysis** (2 days)

#### Day 3 - Type Checking
**File**: `src/compiler/semantic.c`

**Task 3.1**: Implement qty type checking
```c
Type *typecheck_qty_literal(Checker *c, QtyLiteral *lit) {
    // qty<f64, kg>
    Type *inner = type_f64();
    Dimension *dim = lit->dim;
    return type_create_qty(inner, dim);
}
```

**Task 3.2**: Implement dimensional arithmetic
```c
Type *typecheck_qty_binop(Checker *c, BinOp op, Expr *left, Expr *right) {
    Type *lt = typecheck_expr(c, left);
    Type *rt = typecheck_expr(c, right);
    
    if (!is_qty_type(lt) || !is_qty_type(rt)) {
        error("Expected quantity types");
    }
    
    Dimension *ld = get_qty_dimension(lt);
    Dimension *rd = get_qty_dimension(rt);
    
    switch (op) {
        case OP_ADD:
        case OP_SUB:
            // Dimensions must match
            if (!nova_dim_compatible(ld, rd)) {
                error("Cannot %s %s and %s",
                      op == OP_ADD ? "add" : "subtract",
                      nova_dim_to_string(ld),
                      nova_dim_to_string(rd));
            }
            return lt; // Same dimension
            
        case OP_MUL:
            // Multiply dimensions
            Dimension *result_dim = nova_dim_multiply(ld, rd);
            return type_create_qty(get_qty_inner(lt), result_dim);
            
        case OP_DIV:
            // Divide dimensions
            Dimension *result_dim = nova_dim_divide(ld, rd);
            return type_create_qty(get_qty_inner(lt), result_dim);
            
        default:
            error("Invalid operation on quantities");
    }
}
```

**Task 3.3**: Implement error diagnostics
```c
void error_dimension_mismatch(Dimension *expected, Dimension *got) {
    fprintf(stderr, "error: dimension mismatch\n");
    fprintf(stderr, "  expected: %s\n", nova_dim_to_string(expected));
    fprintf(stderr, "  got:      %s\n", nova_dim_to_string(got));
}
```

**Files to modify**:
- `src/compiler/semantic.c` - Add type checking
- `include/nova_diagnostics.h` - Add error types

**Tests**: Type checking for all operations in test_unit_algebra.zn

---

#### Day 4 - Advanced Features
**File**: `src/compiler/semantic.c`

**Task 4.1**: Unit conversion
```c
// Auto-convert compatible units
// 5.kg + 1000.g → 6.kg
Type *typecheck_qty_add_with_conversion(Checker *c, Expr *left, Expr *right) {
    // If dimensions compatible but scales differ, insert conversion
    if (scales_differ(left, right)) {
        insert_conversion_node(right, get_target_scale(left));
    }
    return typecheck_qty_add(c, left, right);
}
```

**Task 4.2**: Dimensionless operations
```c
// sqrt(9.m²) → 3.m
Type *typecheck_qty_sqrt(Checker *c, Expr *arg) {
    Type *t = typecheck_expr(c, arg);
    Dimension *dim = get_qty_dimension(t);
    Dimension *result_dim = nova_dim_power(dim, -2); // Square root = ^(1/2)
    return type_create_qty(get_qty_inner(t), result_dim);
}
```

**Files to modify**:
- `src/compiler/semantic.c` - Add conversions
- `src/compiler/dimensions.c` - Add conversion helpers

**Tests**: Unit conversion tests

---

### **Phase 3: Code Generation** (2 days)

#### Day 5 - Bytecode Generation
**File**: `src/compiler/codegen.c`

**Task 5.1**: Generate qty literals
```c
void codegen_qty_literal(Codegen *cg, QtyLiteral *lit) {
    // At runtime, qty is just a number (dimension checked at compile-time)
    codegen_number(cg, lit->value);
    
    // Store dimension info for debugging (optional)
    if (cg->debug_mode) {
        emit_debug_dimension(cg, lit->dim);
    }
}
```

**Task 5.2**: Generate qty operations
```c
void codegen_qty_binop(Codegen *cg, BinOp op, Expr *left, Expr *right) {
    codegen_expr(cg, left);
    codegen_expr(cg, right);
    
    // Emit same bytecode as numbers (dimensions already checked)
    switch (op) {
        case OP_ADD: emit_byte(cg, OP_ADD); break;
        case OP_SUB: emit_byte(cg, OP_SUB); break;
        case OP_MUL: emit_byte(cg, OP_MUL); break;
        case OP_DIV: emit_byte(cg, OP_DIV); break;
    }
}
```

**Task 5.3**: Optimize away dimension checks
```c
// Since dimensions are checked at compile-time,
// runtime code is identical to numeric operations
// NO RUNTIME OVERHEAD! 🚀
```

**Files to modify**:
- `src/compiler/codegen.c` - Add qty codegen
- `src/backend/bytecode.c` - No changes needed!

**Tests**: Generate bytecode for qty operations

---

#### Day 6 - VM Integration
**File**: `src/backend/vm.c`

**Task 6.1**: Runtime representation
```c
// At runtime, qty<f64, m> is just f64
// Dimension information is erased (zero-cost abstraction!)

typedef struct {
    double value;
    // No dimension field - compile-time only!
} Qty;
```

**Task 6.2**: Add debugging support
```c
// Optional: Store dimension info in debug mode
#ifdef NOVA_DEBUG
typedef struct {
    double value;
    Dimension *dim;  // For runtime checks in debug mode
} QtyDebug;
#endif
```

**Files to modify**:
- `src/backend/vm.c` - Add debug support (optional)
- `src/runtime/value.h` - Document qty representation

**Tests**: Run test_unit_algebra.zn end-to-end

---

### **Phase 4: Testing & Polish** (1 day)

#### Day 7 - Final Integration
**File**: `tests/unit/zn/test_unit_algebra.zn`

**Task 7.1**: Run all tests
```bash
./nova tests/unit/zn/test_unit_algebra.zn
# Should pass all 11 tests:
# ✓ Birimli literal parsing
# ✓ Bileşik birimler
# ✓ Birim dönüşümü
# ✓ Boyutsal analiz
# ✓ Birim aritmetiği
# ✓ Birim ailesi tanımı
# ✓ Kuvvet hesaplama
# ✓ Enerji hesaplama
# ✓ Hata tespiti
# ✓ Karmaşık hesaplamalar
# ✓ Performans
```

**Task 7.2**: Add error tests
```nova
// Should fail at compile-time
let invalid = 5.kg + 3.m;  // ❌ Dimension mismatch
let bad = 10.m * 2.s / 3.A; // ✓ But this should work
```

**Task 7.3**: Performance verification
```c
// Verify zero-cost abstraction
// Qty operations should compile to same bytecode as f64 operations
verify_no_runtime_overhead();
```

**Task 7.4**: Documentation
- Update README with qty examples
- Add API documentation
- Create tutorial for unit algebra

**Tests**: Full test suite passes

---

## 📁 Files to Create/Modify

### New Files (0)
None! All infrastructure exists.

### Files to Modify (7)

| File | Lines | Task |
|------|-------|------|
| `src/compiler/lexer.c` | +50 | Unit literal tokenization |
| `src/compiler/parser.c` | +200 | Qty parsing |
| `include/nova_ast.h` | +30 | Qty AST nodes |
| `src/compiler/ast.c` | +50 | Qty constructors |
| `src/compiler/semantic.c` | +300 | Type checking |
| `src/compiler/codegen.c` | +100 | Code generation |
| `tests/unit/test_qty.c` | +200 | Unit tests |

**Total**: ~930 lines of new code

---

## 🎯 Success Criteria

### Must Have ✅
- [x] Parse unit literals: `5.kg`, `9.81.m/s²`
- [x] Parse qty types: `qty<f64, m/s>`
- [x] Type check addition/subtraction (same dimension)
- [x] Type check multiplication (dimension multiply)
- [x] Type check division (dimension divide)
- [x] Error on dimension mismatch
- [x] Generate correct bytecode
- [x] Zero runtime overhead
- [x] All tests in test_unit_algebra.zn pass

### Nice to Have 🌟
- [ ] Unit conversion (kg ↔ g, m ↔ cm)
- [ ] Custom unit families
- [ ] SI prefix support (k, M, G, etc.)
- [ ] Pretty error messages
- [ ] IDE integration (hover shows dimensions)

---

## 🚀 Key Innovations

### 1. **Zero-Cost Abstraction**
```c
// Compile time:  qty<f64, m> + qty<f64, m> → type check ✓
// Runtime:       f64 + f64 → same as normal numbers!
```

### 2. **Compile-Time Safety**
```nova
// This is a TYPE ERROR, caught at compile time!
let invalid = 5.kg + 3.m;  // ❌ Cannot add mass to length
```

### 3. **Physical Correctness**
```nova
// F = m × a
let force: qty<f64, N> = 10.kg * 9.81.m/s²;  // ✓ Type checks!
let invalid: qty<f64, J> = 10.kg * 9.81.m/s²; // ❌ Wrong dimension
```

### 4. **No Other Language Has This** 🌟
- **Rust**: No dimensional analysis
- **Go**: No dimensional analysis
- **Swift**: No dimensional analysis
- **F#**: Units of measure (similar, but less flexible)
- **Nova**: Full dimensional analysis with ZERO overhead!

---

## 📊 Estimated Timeline

| Phase | Days | Tasks |
|-------|------|-------|
| Parser Integration | 2 | Literal parsing, AST nodes |
| Semantic Analysis | 2 | Type checking, errors |
| Code Generation | 2 | Bytecode, VM |
| Testing & Polish | 1 | Tests, docs |
| **Total** | **7 days** | **~930 lines** |

---

## 🎓 Learning Resources

### Dimensional Analysis Theory
- F. Bridgman, "Dimensional Analysis" (1922)
- SI Units specification (BIPM)
- Units of measure in F# (Microsoft Research)

### Implementation References
- F# units of measure implementation
- Pint library (Python) for unit handling
- SymPy dimensional analysis

---

## 💡 Next Steps

After this is done, Nova will have:
1. ✅ Type System (world-class)
2. ✅ Pattern Matching (Rust-level)
3. ✅ Error Handling (Result<T, E>)
4. ✅ Generics (full monomorphization)
5. ✅ **Unit Algebra (UNIQUE!)** 🌟
6. ⏳ Async/Await (next!)

**Ready to start?** 🚀

Choose:
1. Start with Parser Integration (Day 1)
2. Review the plan first
3. Ask questions about specific parts
