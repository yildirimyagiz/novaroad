# 🔄 Nova: .zn Frontend → C Backend Type Mapping

**Date**: 2026-02-26  
**Status**: Analysis Complete ✅

---

## 📋 Summary

Nova'nın `.zn` frontend'inde yeni tipler eklenmiş. Bunları C backend'e map etmemiz gerekiyor.

### New Types Added in .zn

| .zn Type | Purpose | Status in C |
|----------|---------|-------------|
| **QtyType** | Unit algebra (qty<f64, kg>) | ⚠️ Partial |
| **FlowType** | Reactive/Async (Signal, Stream, Task) | ❌ Missing |
| **EffectType** | Effect system (IO, Async, State) | ❌ Missing |
| **TensorType** | Shape-typed tensors | ⚠️ Partial |
| **DimensionExpr** | SI unit dimensions | ✅ Exists |

---

## 🔍 Detailed Analysis

### 1. **QtyType** (Unit Algebra) 🌟

#### .zn Definition (complete_type_system.zn:102-105)
```nova
expose data QtyType derives [Clone] {
    open base: Box<TypeExpr>,       // f64, f32
    open unit: DimensionExpr,       // kg, m/s², etc.
}
```

#### .zn AST (ast.zn:99)
```nova
TypeExpr::Qty(Box<TypeExpr>, DimExpr)
```

#### .zn DimensionExpr (complete_type_system.zn:110-118)
```nova
expose data DimensionExpr derives [Clone] {
    open mass: i32,        // M (kg)
    open length: i32,      // L (m)
    open time: i32,        // T (s)
    open current: i32,     // I (A)
    open temperature: i32, // Θ (K)
    open amount: i32,      // N (mol)
    open luminosity: i32,  // J (cd)
}
```

#### C Backend Status
✅ **Partial Implementation**

**Existing in C:**
```c
// In src/compiler/dimensions.c
struct nova_dimension {
    int8_t exponents[NUM_BASE_DIMS];  // Similar to DimensionExpr
    double scale_factor;
};

// In src/compiler/codegen.c:73-76
typedef struct {
    nova_type_expr_ext_t *inner;  // Maps to base
    dim_expr_t dim;               // Maps to unit
} qty_type_data_t;
```

**Missing:**
- [ ] Parser integration for `qty<T, dim>` syntax
- [ ] Semantic analysis for qty operations
- [ ] AST node in `nova_ast.h`

---

### 2. **FlowType** (Reactive/Async) ⚡

#### .zn Definition (complete_type_system.zn:140-150)
```nova
expose data FlowType derives [Clone] {
    open kind: FlowKind,
    open inner: Box<TypeExpr>,
}

expose cases FlowKind derives [Clone] {
    Signal,     // Single value that updates
    Stream,     // Sequence of values
    Task,       // Async computation
    Channel,    // Message passing channel
}
```

#### C Backend Status
❌ **Not Implemented**

**Need to Add:**
```c
// In include/nova_types.h
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

// Add to TypeKind enum
TYPE_FLOW,

// Add to Type union
FlowTypeData flow;
```

**Integration Points:**
- [ ] Add `flow<T>` syntax to parser
- [ ] Add semantic analysis for flow types
- [ ] Connect to existing runtime (async.zn, coroutine.c)

---

### 3. **EffectType** (Effect System) 🎭

#### .zn Definition (complete_type_system.zn:197-209)
```nova
expose data EffectType derives [Clone] {
    open effect: Effect,
    open inner: Box<TypeExpr>,
}

expose cases Effect derives [Clone] {
    IO,
    Async,
    State(Box<TypeExpr>),
    Exception,
    Nondet,
    Custom(String),
}
```

#### C Backend Status
❌ **Not Implemented**

**Need to Add:**
```c
// In include/nova_types.h
typedef enum {
    EFFECT_IO,
    EFFECT_ASYNC,
    EFFECT_STATE,
    EFFECT_EXCEPTION,
    EFFECT_NONDET,
    EFFECT_CUSTOM,
} EffectKind;

typedef struct {
    EffectKind kind;
    Type *state_type;      // For EFFECT_STATE
    char *custom_name;     // For EFFECT_CUSTOM
} Effect;

typedef struct {
    Effect effect;
    Type *inner_type;
} EffectTypeData;

// Add to TypeKind enum
TYPE_EFFECT,

// Add to Type union
EffectTypeData effect;
```

**Integration Points:**
- [ ] Add effect syntax to parser (`fn foo() -> T / {IO, Async}`)
- [ ] Add semantic analysis for effect checking
- [ ] Effect inference and propagation

---

### 4. **TensorType** (Shape-Typed Tensors) 🧮

#### .zn Definition (complete_type_system.zn:125-135)
```nova
expose data TensorType derives [Clone] {
    open dtype: Box<TypeExpr>,      // f32, f64, i64
    open shape: Vec<ShapeDim>,      // [batch, seq, dim]
}

expose cases ShapeDim derives [Clone] {
    Named(String),                   // batch, seq
    Const(i64),                      // 32, 512
    Dynamic,                         // ?
    Symbolic(String),                // N, M
}
```

#### C Backend Status
⚠️ **Partial Implementation**

**Existing in C:**
```c
// In src/compiler/codegen.c:78-83
typedef struct {
    nova_type_expr_ext_t *inner;
    int *dims;        // Shape dimensions
    int dim_count;
} tensor_type_data_t;
```

**Missing:**
- [ ] Named dimensions support
- [ ] Symbolic dimensions
- [ ] Shape checking in semantic analysis

---

## 🎯 Implementation Priority

### Priority 1: QtyType (Unit Algebra) 🌟
**Why:** UNIQUE feature, foundation exists, easiest to complete

**Tasks:**
1. Add `TYPE_QTY` to `TypeKind` enum
2. Add `QtyTypeData` to `Type` union
3. Update parser to handle `qty<T, dim>` syntax
4. Add semantic analysis for dimensional operations
5. Test with `test_unit_algebra.zn`

**Estimated Time:** 2-3 days

---

### Priority 2: FlowType (Async/Reactive) ⚡
**Why:** Runtime exists, needed for async/await

**Tasks:**
1. Add `TYPE_FLOW` to `TypeKind` enum
2. Add `FlowTypeData` to `Type` union
3. Update parser to handle `flow<T>` syntax
4. Connect to existing async runtime
5. Add Task<T> as a FlowKind::Task

**Estimated Time:** 2-3 days

---

### Priority 3: EffectType (Effect System) 🎭
**Why:** Advanced feature, can come later

**Tasks:**
1. Add `TYPE_EFFECT` to `TypeKind` enum
2. Add `EffectTypeData` to `Type` union
3. Update parser to handle effect syntax
4. Implement effect inference
5. Effect checking in semantic analysis

**Estimated Time:** 3-4 days

---

### Priority 4: TensorType Enhancements 🧮
**Why:** Basic support exists, just needs enhancement

**Tasks:**
1. Add named dimension support
2. Add symbolic dimension support
3. Shape inference and checking
4. Integration with existing tensor runtime

**Estimated Time:** 2 days

---

## 📁 Files to Modify

### Core Type System
```
include/nova_types.h          - Add new TypeKind values and data structures
include/nova_ast.h            - Add new AST node types
src/compiler/ast.c            - Add constructors for new types
```

### Parser
```
src/compiler/lexer.c          - Add tokens (async, await, qty, flow, effect)
src/compiler/tokens.c         - Token definitions
src/compiler/parser.c         - Parse new type syntax
```

### Semantic Analysis
```
src/compiler/semantic.c       - Type checking for new types
src/compiler/dimensions.c     - Extend for qty operations
```

### Code Generation
```
src/compiler/codegen.c        - Generate bytecode for new types
```

---

## 🔄 Mapping Table

| .zn Type | .zn File | C Header | C Implementation | Status |
|----------|----------|----------|------------------|--------|
| QtyType | complete_type_system.zn:103 | nova_types.h | dimensions.c | ⚠️ 60% |
| DimensionExpr | complete_type_system.zn:110 | dimensions.h | dimensions.c | ✅ 100% |
| FlowType | complete_type_system.zn:141 | nova_types.h | (new) | ❌ 0% |
| FlowKind | complete_type_system.zn:145 | nova_types.h | (new) | ❌ 0% |
| EffectType | complete_type_system.zn:198 | nova_types.h | (new) | ❌ 0% |
| Effect | complete_type_system.zn:202 | nova_types.h | (new) | ❌ 0% |
| TensorType | complete_type_system.zn:126 | nova_types.h | (partial) | ⚠️ 40% |
| ShapeDim | complete_type_system.zn:130 | nova_types.h | (new) | ❌ 0% |

---

## 🚀 Recommended Implementation Order

### Week 1: QtyType (Unit Algebra)
**Days 1-3**: Complete QtyType implementation
- Add TYPE_QTY to TypeKind
- Parser integration
- Semantic analysis
- Testing with test_unit_algebra.zn

### Week 2: FlowType + Async
**Days 4-6**: Complete FlowType implementation
- Add TYPE_FLOW to TypeKind
- Parser integration for async/await
- Connect to async runtime
- Testing

### Week 3: EffectType
**Days 7-9**: Complete EffectType implementation
- Add TYPE_EFFECT to TypeKind
- Effect syntax parsing
- Effect inference
- Testing

### Week 4: Polish
**Days 10-11**: TensorType enhancements
**Days 12-14**: Integration testing, documentation

---

## 💡 Next Steps

**Ready to start implementing!**

Choose:
1. **Start with QtyType** (RECOMMENDED) - Easiest, UNIQUE feature
2. **Start with FlowType** - Needed for async/await
3. **Start with EffectType** - Advanced, can wait
4. **Review detailed plan** - Ask questions first

Which would you like to tackle first? 🎯
