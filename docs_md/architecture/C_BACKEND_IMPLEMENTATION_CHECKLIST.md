# ✅ C Backend Implementation Checklist

**Date**: 2026-02-26  
**Goal**: Add QtyType, FlowType, EffectType support to C backend

---

## 🎯 Phase 1: QtyType (Unit Algebra) - Priority 1

### Step 1: Update Type System Headers
**File**: `include/nova_types.h`

- [ ] Add `TYPE_QTY` to `TypeKind` enum (after TYPE_GENERIC)
- [ ] Add `QtyTypeData` struct definition
- [ ] Add `QtyTypeData qty;` to Type union

```c
// Add to TypeKind enum (line ~46)
TYPE_QTY,          // Quantity type (qty<f64, kg>)

// Add before Type struct definition
typedef struct {
    Type *base_type;           // f64, f32, i32, etc.
    nova_dimension_t *dimension;  // From dimensions.h
} QtyTypeData;

// Add to Type union (line ~154)
QtyTypeData qty;
```

### Step 2: Update AST Headers
**File**: `include/nova_ast.h`

- [ ] Add `AST_QTY_TYPE` to `ASTNodeKind` enum
- [ ] Add `AST_QTY_LITERAL` to `ASTNodeKind` enum
- [ ] Add `QtyTypeNode` struct
- [ ] Add `QtyLiteralNode` struct

```c
// Add to ASTNodeKind enum
AST_QTY_TYPE,      // qty<T, dim>
AST_QTY_LITERAL,   // 5.kg

// Add struct definitions
typedef struct {
    ASTNode *base_type;
    DimensionExpr *dimension;
    SourceLocation location;
} QtyTypeNode;

typedef struct {
    double value;
    nova_dimension_t *dimension;
    SourceLocation location;
} QtyLiteralNode;

// Add DimensionExpr struct
typedef struct {
    int8_t mass;        // M (kg)
    int8_t length;      // L (m)
    int8_t time;        // T (s)
    int8_t current;     // I (A)
    int8_t temperature; // Θ (K)
    int8_t amount;      // N (mol)
    int8_t luminosity;  // J (cd)
} DimensionExpr;
```

### Step 3: Add Token Support
**File**: `src/compiler/tokens.c` / `include/nova_tokens.h`

- [ ] Add `TOKEN_QTY` token
- [ ] Add `TOKEN_UNIT` token
- [ ] Add `TOKEN_DIMS` token
- [ ] Update keyword table

```c
// Add to TokenKind enum
TOKEN_QTY,    // 'qty'
TOKEN_UNIT,   // 'unit'
TOKEN_DIMS,   // 'dims'

// Add to keyword table in tokens.c
{"qty", TOKEN_QTY},
{"unit", TOKEN_UNIT},
{"dims", TOKEN_DIMS},
```

### Step 4: Lexer Support for Unit Literals
**File**: `src/compiler/lexer.c`

- [ ] Add unit literal recognition (5.kg, 9.81.m/s²)
- [ ] Add compound unit parsing (m/s, kg·m/s²)

```c
// Add function
Token lexer_scan_unit_literal(Lexer *lexer) {
    // Parse: NUMBER DOT IDENTIFIER
    // Example: 5.kg, 9.81.m/s²
    
    double value = lexer_scan_number(lexer);
    
    if (lexer_peek(lexer) == '.') {
        lexer_advance(lexer);  // consume '.'
        
        // Parse unit name
        char *unit = lexer_scan_identifier(lexer);
        
        // Look up dimension
        nova_dimension_t *dim = nova_dim_parse(unit);
        
        // Create qty literal token
        return token_create_qty_literal(value, dim);
    }
    
    return token_create_number(value);
}
```

### Step 5: Parser Integration
**File**: `src/compiler/parser.c`

- [ ] Add `parse_qty_type()` function
- [ ] Add `parse_qty_literal()` function
- [ ] Add `parse_dimension_expr()` function
- [ ] Integrate into `parse_type()`

```c
// Parse qty<f64, kg>
ASTNode *parse_qty_type(Parser *p) {
    expect(p, TOKEN_QTY);
    expect(p, TOKEN_LESS);
    
    ASTNode *base = parse_type(p);
    expect(p, TOKEN_COMMA);
    
    DimensionExpr *dim = parse_dimension_expr(p);
    expect(p, TOKEN_GREATER);
    
    return ast_create_qty_type(base, dim);
}

// Parse dimension: kg, m/s², kg·m/s²
DimensionExpr *parse_dimension_expr(Parser *p) {
    // Simple version: parse identifier and look up
    char *unit = parse_identifier(p);
    nova_dimension_t *dim = nova_dim_parse(unit);
    
    // Convert to DimensionExpr
    DimensionExpr *expr = malloc(sizeof(DimensionExpr));
    // Copy exponents from nova_dimension_t
    // ...
    
    return expr;
}
```

### Step 6: Semantic Analysis
**File**: `src/compiler/semantic.c`

- [ ] Add `check_qty_type()` function
- [ ] Add `check_qty_binop()` function (dimensional arithmetic)
- [ ] Add dimension compatibility checking

```c
Type *check_qty_type(Checker *c, QtyTypeNode *node) {
    Type *base = check_type(c, node->base_type);
    
    // Verify base is numeric (f64, f32, i32, etc.)
    if (!is_numeric_type(base)) {
        error("Quantity base type must be numeric, got %s", 
              type_to_string(base));
    }
    
    Type *qty = type_create(TYPE_QTY);
    qty->data.qty.base_type = base;
    qty->data.qty.dimension = dimension_from_expr(node->dimension);
    
    return qty;
}

Type *check_qty_binop(Checker *c, BinOpNode *node) {
    Type *left = check_expr(c, node->left);
    Type *right = check_expr(c, node->right);
    
    if (left->kind != TYPE_QTY || right->kind != TYPE_QTY) {
        return check_binop(c, node);  // Normal binop
    }
    
    QtyTypeData *lqty = &left->data.qty;
    QtyTypeData *rqty = &right->data.qty;
    
    switch (node->op) {
        case OP_ADD:
        case OP_SUB:
            // Dimensions must match
            if (!nova_dim_compatible(lqty->dimension, rqty->dimension)) {
                error("Cannot %s %s and %s",
                      node->op == OP_ADD ? "add" : "subtract",
                      nova_dim_to_string(lqty->dimension),
                      nova_dim_to_string(rqty->dimension));
            }
            return left;  // Same type
            
        case OP_MUL:
            // Multiply dimensions
            Type *result = type_create(TYPE_QTY);
            result->data.qty.base_type = lqty->base_type;
            result->data.qty.dimension = 
                nova_dim_multiply(lqty->dimension, rqty->dimension);
            return result;
            
        case OP_DIV:
            // Divide dimensions
            Type *result = type_create(TYPE_QTY);
            result->data.qty.base_type = lqty->base_type;
            result->data.qty.dimension = 
                nova_dim_divide(lqty->dimension, rqty->dimension);
            return result;
            
        default:
            error("Invalid operation on quantities");
    }
}
```

### Step 7: Code Generation
**File**: `src/compiler/codegen.c`

- [ ] Add `codegen_qty_type()` function
- [ ] Add `codegen_qty_literal()` function
- [ ] Add `codegen_qty_binop()` function

```c
void codegen_qty_literal(Codegen *cg, QtyLiteralNode *node) {
    // At runtime, qty is just a number
    // Dimension is erased (zero-cost!)
    emit_number(cg, node->value);
}

void codegen_qty_binop(Codegen *cg, BinOpNode *node) {
    // Generate normal arithmetic
    // Dimensions already checked at compile-time
    codegen_expr(cg, node->left);
    codegen_expr(cg, node->right);
    
    switch (node->op) {
        case OP_ADD: emit_byte(cg, OP_ADD); break;
        case OP_SUB: emit_byte(cg, OP_SUB); break;
        case OP_MUL: emit_byte(cg, OP_MUL); break;
        case OP_DIV: emit_byte(cg, OP_DIV); break;
    }
}
```

### Step 8: Testing
**File**: `tests/unit/test_qty_backend.c`

- [ ] Create test suite
- [ ] Test parsing qty types
- [ ] Test dimensional arithmetic
- [ ] Test error cases
- [ ] Run `test_unit_algebra.zn`

```c
void test_qty_parsing() {
    Parser *p = parser_create("qty<f64, kg>");
    Type *t = parse_type(p);
    
    assert(t->kind == TYPE_QTY);
    assert(t->data.qty.base_type->kind == TYPE_F64);
    // ...
}

void test_dimensional_arithmetic() {
    // Test: 5.kg * 9.81.m/s² = 49.05.N
    // ...
}
```

---

## 🎯 Phase 2: FlowType (Async/Reactive) - Priority 2

### Step 1: Update Type System
**File**: `include/nova_types.h`

- [ ] Add `TYPE_FLOW` to TypeKind enum
- [ ] Add `FlowKind` enum
- [ ] Add `FlowTypeData` struct
- [ ] Add to Type union

```c
typedef enum {
    FLOW_SIGNAL,
    FLOW_STREAM,
    FLOW_TASK,
    FLOW_CHANNEL,
} FlowKind;

typedef struct {
    FlowKind kind;
    Type *inner_type;
} FlowTypeData;

// Add to TypeKind
TYPE_FLOW,

// Add to Type union
FlowTypeData flow;
```

### Step 2: Parser Integration
**File**: `src/compiler/parser.c`

- [ ] Add `parse_async_fn()` function
- [ ] Add `parse_await_expr()` function
- [ ] Add tokens: TOKEN_ASYNC, TOKEN_AWAIT

```c
FnDef *parse_async_fn(Parser *p) {
    bool is_async = match(p, TOKEN_ASYNC);
    expect(p, TOKEN_FN);
    
    // Parse rest of function...
    
    if (is_async) {
        // Wrap return type in Task<T>
        fn->return_type = create_flow_type(FLOW_TASK, fn->return_type);
        fn->is_async = true;
    }
    
    return fn;
}
```

### Step 3: Semantic Analysis
**File**: `src/compiler/semantic.c`

- [ ] Add `check_async_fn()` function
- [ ] Add `check_await_expr()` function
- [ ] Add async context tracking

```c
Type *check_await_expr(Checker *c, AwaitExpr *expr) {
    if (!c->in_async_fn) {
        error("'await' can only be used in async functions");
    }
    
    Type *future_type = check_expr(c, expr->expr);
    
    if (future_type->kind != TYPE_FLOW || 
        future_type->data.flow.kind != FLOW_TASK) {
        error("'await' expects Task<T>, got %s", type_to_string(future_type));
    }
    
    // await Task<T> returns T
    return future_type->data.flow.inner_type;
}
```

---

## 🎯 Phase 3: EffectType - Priority 3

### Step 1: Update Type System
**File**: `include/nova_types.h`

- [ ] Add `TYPE_EFFECT` to TypeKind
- [ ] Add `EffectKind` enum
- [ ] Add `EffectTypeData` struct

```c
typedef enum {
    EFFECT_IO,
    EFFECT_ASYNC,
    EFFECT_STATE,
    EFFECT_EXCEPTION,
} EffectKind;

typedef struct {
    EffectKind kind;
    Type *inner_type;
} EffectTypeData;
```

---

## 📊 Progress Tracking

| Task | Status | Priority | Estimated Time |
|------|--------|----------|----------------|
| QtyType - Type System | ⏳ | P1 | 2h |
| QtyType - AST | ⏳ | P1 | 1h |
| QtyType - Tokens | ⏳ | P1 | 1h |
| QtyType - Lexer | ⏳ | P1 | 3h |
| QtyType - Parser | ⏳ | P1 | 4h |
| QtyType - Semantic | ⏳ | P1 | 6h |
| QtyType - Codegen | ⏳ | P1 | 2h |
| QtyType - Testing | ⏳ | P1 | 3h |
| **QtyType Total** | | | **~3 days** |
| | | | |
| FlowType - Type System | ⏳ | P2 | 2h |
| FlowType - Parser | ⏳ | P2 | 4h |
| FlowType - Semantic | ⏳ | P2 | 6h |
| FlowType - Codegen | ⏳ | P2 | 4h |
| FlowType - Testing | ⏳ | P2 | 2h |
| **FlowType Total** | | | **~2.5 days** |

---

## 🎯 Success Criteria

### QtyType ✅
- [x] Parse `qty<f64, kg>` syntax
- [x] Parse unit literals `5.kg`, `9.81.m/s²`
- [x] Type check dimensional arithmetic
- [x] Error on incompatible dimensions
- [x] Zero-cost code generation
- [x] All tests in `test_unit_algebra.zn` pass

### FlowType ✅
- [x] Parse `async fn` syntax
- [x] Parse `await` expressions
- [x] Type check async functions
- [x] Error on `await` outside async
- [x] Integration with async runtime

---

## 💡 Implementation Tips

1. **Start Small**: Begin with basic parsing, then add semantic checks
2. **Test Incrementally**: Test each component as you build it
3. **Reuse Existing Code**: dimensions.c already has most qty logic
4. **Zero-Cost**: qty operations compile to normal arithmetic
5. **Error Messages**: Make dimension errors clear and helpful

---

## 🚀 Ready to Start?

Next step: **Implement QtyType in C backend**

Start with:
1. Update `include/nova_types.h` - Add TYPE_QTY
2. Update `include/nova_ast.h` - Add AST nodes
3. Test compilation

Would you like me to start implementing? 🎯
