# 📋 Nova Implementation TODO List

**Created**: 2026-02-26  
**Status**: Active Development

---

## 🎯 Immediate Tasks (1-2 Days)

### Day 1: Type Creators & AST Nodes

#### ✅ Task 1.1: Implement Type Creators (2-3 hours)
**File**: `src/compiler/ast.c`

- [ ] Add `type_create_qty()` implementation
- [ ] Add `type_create_flow()` implementation
- [ ] Add `type_create_effect()` implementation
- [ ] Add `type_create_tensor()` implementation
- [ ] Add `type_is_qty()` implementation
- [ ] Add `type_is_flow()` implementation
- [ ] Add `type_is_effect()` implementation
- [ ] Add `type_is_tensor()` implementation

**Dependencies**: None (nova_types.h already updated)  
**Test**: Compilation successful

---

#### ✅ Task 1.2: Add AST Node Types (2-3 hours)
**File**: `include/nova_ast.h`

**Add to ASTNodeKind enum**:
- [ ] `AST_QTY_TYPE` - qty<T, dim>
- [ ] `AST_QTY_LITERAL` - 5.kg
- [ ] `AST_FLOW_TYPE` - Task<T>, Stream<T>
- [ ] `AST_ASYNC_FN` - async fn
- [ ] `AST_AWAIT_EXPR` - await expr
- [ ] `AST_EFFECT_TYPE` - IO<T>

**Add struct definitions**:
- [ ] `QtyTypeNode` struct
- [ ] `QtyLiteralNode` struct
- [ ] `FlowTypeNode` struct
- [ ] `AsyncFnNode` struct
- [ ] `AwaitExprNode` struct
- [ ] `EffectTypeNode` struct
- [ ] `DimensionExpr` struct

**Dependencies**: Task 1.1  
**Test**: Compilation successful

---

#### ✅ Task 1.3: Add AST Constructors (1-2 hours)
**File**: `src/compiler/ast.c`

- [ ] `ast_create_qty_type(base, dim)`
- [ ] `ast_create_qty_literal(value, dim)`
- [ ] `ast_create_flow_type(kind, inner)`
- [ ] `ast_create_async_fn(name, params, body)`
- [ ] `ast_create_await_expr(expr)`
- [ ] `ast_create_effect_type(effect, inner)`

**Dependencies**: Task 1.2  
**Test**: Compilation successful

---

### Day 2: Token Support & Initial Testing

#### ✅ Task 2.1: Add Tokens (30 minutes)
**File**: `src/compiler/tokens.c` and `include/nova_tokens.h`

**Add to TokenKind enum**:
- [ ] `TOKEN_QTY` - 'qty'
- [ ] `TOKEN_ASYNC` - 'async'
- [ ] `TOKEN_AWAIT` - 'await'
- [ ] `TOKEN_FLOW` - 'flow'
- [ ] `TOKEN_EFFECT` - 'effect'
- [ ] `TOKEN_UNIT` - 'unit'

**Update keyword table**:
- [ ] Add "qty" → TOKEN_QTY
- [ ] Add "async" → TOKEN_ASYNC
- [ ] Add "await" → TOKEN_AWAIT
- [ ] Add "flow" → TOKEN_FLOW
- [ ] Add "effect" → TOKEN_EFFECT
- [ ] Add "unit" → TOKEN_UNIT

**Dependencies**: None  
**Test**: Lexer recognizes keywords

---

#### ✅ Task 2.2: Compilation Test (1 hour)
**File**: `tests/unit/test_type_system_new.c`

- [ ] Test type creators
- [ ] Test AST constructors
- [ ] Test token recognition
- [ ] Verify compilation
- [ ] Run basic tests

**Dependencies**: All Day 1-2 tasks  
**Test**: All tests pass

---

## 🚀 Phase 2: Parser Integration (Week 3 - 4-6 hours)

### Task 3.1: Parse Qty Types (2 hours)
**File**: `src/compiler/parser.c`

- [ ] Add `parse_qty_type()` function
- [ ] Parse `qty<T, dim>` syntax
- [ ] Parse dimension expressions (kg, m/s²)
- [ ] Integrate into `parse_type()`

**Dependencies**: Day 1-2 complete  
**Test**: Parse `qty<f64, kg>` successfully

---

### Task 3.2: Parse Unit Literals (2 hours)
**File**: `src/compiler/lexer.c` and `parser.c`

- [ ] Lexer: Recognize `5.kg` pattern
- [ ] Lexer: Parse unit names
- [ ] Parser: Create qty literal nodes
- [ ] Integration test

**Dependencies**: Task 3.1  
**Test**: Parse `5.kg`, `9.81.m/s²`

---

### Task 3.3: Parse Async/Await (2 hours)
**File**: `src/compiler/parser.c`

- [ ] Add `parse_async_fn()` function
- [ ] Add `parse_await_expr()` function
- [ ] Modify `parse_function()` for async
- [ ] Integration test

**Dependencies**: Task 3.1  
**Test**: Parse async functions

---

## 🧠 Phase 3: Semantic Analysis (Week 4-5 - 6-8 hours)

### Task 4.1: Qty Type Checking (3 hours)
**File**: `src/compiler/semantic.c`

- [ ] Add `check_qty_type()` function
- [ ] Add `check_qty_literal()` function
- [ ] Add `check_qty_binop()` function
- [ ] Dimensional arithmetic rules
- [ ] Error messages for dimension mismatch

**Dependencies**: Parser complete  
**Test**: Dimensional arithmetic works

---

### Task 4.2: Async Type Checking (3 hours)
**File**: `src/compiler/semantic.c`

- [ ] Add `check_async_fn()` function
- [ ] Add `check_await_expr()` function
- [ ] Async context tracking
- [ ] Return type verification (Future<T>)
- [ ] Error messages

**Dependencies**: Task 4.1  
**Test**: Async functions type-check

---

### Task 4.3: Integration Testing (2 hours)
**File**: `tests/unit/zn/test_unit_algebra.zn`

- [ ] Run unit algebra tests
- [ ] Verify dimensional checking
- [ ] Test error cases
- [ ] Performance verification

**Dependencies**: Task 4.1-4.2  
**Test**: All tests pass

---

## 💻 Phase 4: Code Generation (Week 5 - 2-3 hours)

### Task 5.1: Qty Codegen (1 hour)
**File**: `src/compiler/codegen.c`

- [ ] Add `codegen_qty_literal()`
- [ ] Add `codegen_qty_binop()`
- [ ] Zero-cost optimization
- [ ] Test bytecode generation

**Dependencies**: Semantic analysis complete  
**Test**: Qty operations generate correct bytecode

---

### Task 5.2: Async Codegen (2 hours)
**File**: `src/compiler/codegen.c`

- [ ] Add `codegen_async_fn()`
- [ ] Add `codegen_await_expr()`
- [ ] State machine generation
- [ ] Integration with async runtime

**Dependencies**: Task 5.1  
**Test**: Async functions work end-to-end

---

## 📊 Progress Tracking

| Phase | Tasks | Status | Completion |
|-------|-------|--------|------------|
| Day 1-2: Foundation | 3 | ⏳ TODO | 0% |
| Week 3: Parser | 3 | ⏳ TODO | 0% |
| Week 4-5: Semantic | 3 | ⏳ TODO | 0% |
| Week 5: Codegen | 2 | ⏳ TODO | 0% |
| **Total** | **11** | | **0%** |

---

## 🎯 Success Criteria

### Phase 1 Complete (Day 1-2)
- [x] Type creators implemented
- [x] AST nodes defined
- [x] Tokens added
- [x] Compilation successful

### Phase 2 Complete (Week 3)
- [x] Parse qty<T, dim>
- [x] Parse unit literals (5.kg)
- [x] Parse async fn / await

### Phase 3 Complete (Week 4-5)
- [x] Dimensional arithmetic checking
- [x] Async type checking
- [x] test_unit_algebra.zn passes

### Phase 4 Complete (Week 5)
- [x] Qty operations generate bytecode
- [x] Async functions work
- [x] Zero runtime overhead verified

---

## 📁 File Organization

```
nova/
├── docs/
│   └── implementation/
│       ├── TODO.md (this file)
│       ├── unit_algebra/
│       │   ├── IMPLEMENTATION_PLAN.md
│       │   ├── PROGRESS.md
│       │   └── EXAMPLES.md
│       └── ml_features/
│           ├── GRADIENT_TYPES.md
│           ├── MIXED_PRECISION.md
│           └── MEMORY_BUDGETS.md
├── include/
│   ├── nova_types.h ✅ (updated)
│   ├── nova_ast.h ⏳ (to update)
│   └── nova_tokens.h ⏳ (to update)
└── src/
    └── compiler/
        ├── ast.c ⏳ (to update)
        ├── tokens.c ⏳ (to update)
        ├── lexer.c ⏳ (to update)
        ├── parser.c ⏳ (to update)
        ├── semantic.c ⏳ (to update)
        └── codegen.c ⏳ (to update)
```

---

## 🚀 Getting Started

### Today (Right Now!)

**Task**: Implement type creators in `src/compiler/ast.c`

**Steps**:
1. Open `src/compiler/ast.c`
2. Copy implementations from `/tmp/type_helpers.c`
3. Add to bottom of file
4. Compile and test
5. Move to next task

**Estimated time**: 2-3 hours

**Let's go!** 🎯
