# 🚀 Nova Backend Implementation Plan

## Mevcut Durum Analizi

### ✅ Implementasyonu Var
- Basic expressions (binary, unary, literal)
- Variables (var, let)
- Functions (fn)
- Control flow (if/check, while, break/continue)
- Arrays
- Strings
- Basic operators (+, -, *, /, %, <, >, ==, etc.)

### ❌ Eksik Backend Features

#### 1. Generics System
**Gerekli:**
- Type parameter substitution
- Monomorphization (generic -> concrete type expansion)
- Generic function instantiation
- Generic struct/enum support

**Dosyalar:**
- `src/compiler/typesystem/type_infer.c` - Type inference
- `src/compiler/core/nova_generics.c` - Generic implementation
- `codegen.c` - Generic instantiation codegen

#### 2. Pattern Matching
**Gerekli:**
- Match expression codegen
- Pattern binding
- Exhaustiveness checking
- Guard expressions

**Yeni kod:**
```c
case EXPR_MATCH: {
    // 1. Evaluate scrutinee
    // 2. For each arm:
    //    - Test pattern
    //    - Jump if not match
    //    - Bind variables
    //    - Execute body
    //    - Jump to end
    // 3. End label
}
```

#### 3. Error Handling (Try/Catch)
**Gerekli:**
- Exception stack
- Try block entry/exit
- Catch handler registration
- Error propagation (`?` operator)

**Yeni kod:**
```c
case STMT_TRY: {
    // 1. Save error handler
    // 2. Execute try block
    // 3. Restore handler
    // 4. Jump over catch
    // 5. Catch handler label
    // 6. Execute catch block
}
```

#### 4. Contracts (Require/Ensure)
**Gerekli:**
- Pre-condition checking (require)
- Post-condition checking (ensure)
- Old value capture (for ensure)
- Runtime assertion generation

**Yeni kod:**
```c
case STMT_REQUIRE: {
    // 1. Evaluate condition
    // 2. If false, panic with message
}

case STMT_ENSURE: {
    // 1. Capture "old" values before function
    // 2. After function, evaluate condition
    // 3. If false, panic
}
```

#### 5. Unit Algebra 🌟
**Gerekli:**
- Unit type representation
- Dimensional checking at compile-time
- Unit conversion codegen
- Compile-time unit arithmetic

**Strategi:**
- Parse time: Extract unit info (`5.kg` -> value=5, unit="kg")
- Semantic: Build unit type system, check compatibility
- Codegen: Strip units (runtime just uses numbers)
- Error: Incompatible units = compile error

**Örnek:**
```c
// Compile time:
let mass: Qty<f64, kg> = 5.kg;     // Store: (5.0, "kg")
let dist: Qty<f64, m> = 10.m;      // Store: (10.0, "m")
let invalid = mass + dist;         // ERROR: kg + m invalid!

// Runtime (after type check):
double mass_value = 5.0;  // Unit info stripped
double dist_value = 10.0;
```

---

## 🎯 Implementation Strategy

### Phase 1: Quick Wins (1 hafta)
**Pattern Matching - En kolay, en etkili**

1. AST'ye `EXPR_MATCH` ekle
2. Parser'da match parsing'i aktif et
3. Codegen:
```c
case EXPR_MATCH: {
    generate_expression(scrutinee);
    
    for each arm:
        // Compare pattern
        OP_DUP              // Duplicate scrutinee
        generate_pattern()  // Pattern check
        OP_JUMP_IF_FALSE -> next_arm
        
        // Bind variables
        generate_bindings()
        
        // Execute body
        generate_expression(arm->body)
        OP_JUMP -> match_end
        
    next_arm:
    match_end:
}
```

### Phase 2: Generics (2 hafta)
**Monomorphization approach (Rust-style)**

1. Type parameter collection
2. Generic instantiation at call site
3. Code generation for each concrete type
4. Function name mangling (generic_fn_i64, generic_fn_f64)

```c
// Input:
fn identity<T>(x: T) -> T { yield x; }
let a = identity(42);      // T = i64
let b = identity(3.14);    // T = f64

// After monomorphization:
fn identity_i64(x: i64) -> i64 { yield x; }
fn identity_f64(x: f64) -> f64 { yield x; }
let a = identity_i64(42);
let b = identity_f64(3.14);
```

### Phase 3: Error Handling (1 hafta)
**Simple exception system**

1. Global exception stack
2. `setjmp`/`longjmp` for try/catch
3. Result type unwrapping for `?`

```c
// Runtime structure:
typedef struct {
    jmp_buf handler;
    Error *error;
} ExceptionFrame;

ExceptionFrame *exception_stack;

case STMT_TRY: {
    ExceptionFrame frame;
    if (setjmp(frame.handler) == 0) {
        push_exception_frame(&frame);
        generate_block(try_block);
        pop_exception_frame();
    } else {
        // Catch path
        generate_catch(catch_block, frame.error);
    }
}
```

### Phase 4: Contracts (3 gün)
**Runtime checking**

```c
case STMT_FUNCTION: {
    // Generate requires
    for each require:
        generate_expression(condition);
        OP_JUMP_IF_TRUE -> ok
        OP_PANIC "Precondition failed"
        ok:
    
    // Generate function body
    generate_block(body);
    
    // Generate ensures
    for each ensure:
        generate_expression(condition);
        OP_JUMP_IF_TRUE -> ok
        OP_PANIC "Postcondition failed"
        ok:
}
```

### Phase 5: Unit Algebra 🌟 (2-3 hafta)
**Compile-time dimensional analysis**

1. **Lexer**: Already parses `5.kg` ✅
2. **Parser**: Create `UnitLiteral` AST node
3. **Type system**: Add `UnitType` 
4. **Semantic**: Dimensional checking
5. **Codegen**: Strip units, emit plain numbers

```c
// Type system:
typedef struct {
    TypeKind base;  // f64, i64
    char *unit;     // "kg", "m", "m/s²"
    int dimensions[7]; // [M, L, T, I, Θ, N, J]
} UnitType;

// Semantic analysis:
bool check_unit_compatible(UnitType *a, UnitType *b) {
    for (int i = 0; i < 7; i++) {
        if (a->dimensions[i] != b->dimensions[i])
            return false;
    }
    return true;
}

// Addition: same units required
if (op == '+' || op == '-') {
    if (!check_unit_compatible(left_type, right_type))
        error("Incompatible units");
}

// Multiplication: add dimensions
if (op == '*') {
    for (int i = 0; i < 7; i++)
        result->dimensions[i] = left->dimensions[i] + right->dimensions[i];
}

// Codegen: Just emit the number!
case EXPR_UNIT_LITERAL: {
    double value = expr->data.unit_lit.value;
    // Unit already checked at semantic phase
    chunk_write_constant(chunk, NUMBER_VAL(value));
}
```

---

## 📁 Dosya Değişiklikleri

### include/nova_ast.h
```c
// Add new expression types:
typedef enum {
    // ... existing ...
    EXPR_MATCH,
    EXPR_UNIT_LITERAL,
    EXPR_TRY,
    EXPR_ERROR_PROPAGATE,  // ? operator
} ExprKind;

// Add new statement types:
typedef enum {
    // ... existing ...
    STMT_REQUIRE,
    STMT_ENSURE,
    STMT_TRY_CATCH,
} StmtKind;

// New structures:
typedef struct {
    Expr *scrutinee;
    MatchArm *arms;
    size_t arm_count;
} MatchExpr;

typedef struct {
    double value;
    char *unit;  // "kg", "m/s²", etc.
    int dimensions[7];  // SI base units
} UnitLiteral;
```

### src/compiler/codegen.c
```c
// Add handlers:
case EXPR_MATCH: {
    return generate_match(codegen, expr);
}

case EXPR_UNIT_LITERAL: {
    // Units already checked - just emit value
    chunk_write_constant(codegen->chunk, 
        NUMBER_VAL(expr->data.unit_lit.value));
    return true;
}
```

### src/compiler/typesystem/type_checker.c
```c
// Add unit type checking:
bool check_unit_arithmetic(UnitType *left, UnitType *right, const char *op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
        return units_equal(left, right);
    } else if (strcmp(op, "*") == 0) {
        return multiply_units(left, right);
    } else if (strcmp(op, "/") == 0) {
        return divide_units(left, right);
    }
    return false;
}
```

---

## 🎯 Öncelik Sırası

### Must Have (Production için gerekli)
1. **Generics** - Modern diller için zorunlu
2. **Pattern Matching** - Ergonomik kod için kritik
3. **Error Handling** - Güvenilir yazılım için şart

### Should Have (Farklılaştırıcı)
4. **Unit Algebra** 🌟 - Nova'nın killer feature'ı
5. **Contracts** - Formal verification için

### Nice to Have
- Async/await
- SIMD intrinsics
- Advanced optimizations

---

## 🚀 Başlangıç: Pattern Matching

En kolay ve en etkili özellik. Hemen başlayabilir miyiz?

```bash
# 1. AST'ye match expression ekle
# 2. Parser'da match parsing aktif et
# 3. Codegen'e match handler ekle
# 4. Test et!
```

Başlayalım mı? 🚀
