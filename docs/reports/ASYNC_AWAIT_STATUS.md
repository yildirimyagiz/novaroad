# Nova Async/Await & Effect System - Status Report

**Date:** 2026-02-28  
**Analysis:** Complete Feature Investigation

---

## Summary

Nova has **foundational infrastructure** for async/await and effect systems:

| Component | Status | Completeness |
|-----------|--------|--------------|
| AST Nodes | ✅ Defined | 100% |
| Type System | ✅ Partial | 60% |
| Parser | ⚠️ Partial | 40% |
| Runtime | ⚠️ Stub | 20% |
| Stdlib | ⚠️ Stub | 30% |

**Overall Status:** 🟡 ~40% Complete (Foundation exists, needs implementation)

---

## What Exists

### 1. AST Infrastructure (100% ✅)

Defined in `include/nova_ast.h`:

```c
// Async function node
AST_ASYNC_FN          // async fn declarations

// Await expression node  
AST_AWAIT_EXPR        // await expr

// Flow types
typedef enum {
    FLOW_SIGNAL,      // Single reactive value
    FLOW_STREAM,      // Sequence of values
    FLOW_TASK,        // Async computation (Future<T>)
    FLOW_CHANNEL,     // Message passing
} FlowTypeKind;

// Effect types
typedef enum {
    EFFECT_IO,
    EFFECT_ASYNC,
    EFFECT_STATE,
    EFFECT_EXCEPTION,
    EFFECT_NONDET,
    EFFECT_CUSTOM
} EffectTypeKind;
```

**Status:** Fully defined ✅

### 2. Type System Support (60% ✅)

From `zn/src/compiler/frontend/core/complete_type_system.zn`:

```nova
cases FlowKind {
    Signal,     // Signal<T> - reactive value
    Stream,     // Stream<T> - async sequence
    Task,       // Task<T> - async computation
    Chan,       // Chan<T> - message passing
}
```

**Status:** Types defined, inference incomplete ⚠️

### 3. Syntax Support (40% ⚠️)

Tokens and basic parsing exist:
- `async` keyword ✅
- `await` keyword ✅
- `Task<T>` type syntax ✅
- Function declaration parsing ⚠️ (partial)
- Expression parsing ⚠️ (partial)

**Status:** Foundation exists, needs completion ⚠️

---

## What's Missing

### 1. Parser Implementation (60% TODO)

**Needed:**
- [ ] `async fn` parsing in function declarations
- [ ] `await` expression parsing
- [ ] `spawn { }` block parsing for creating tasks
- [ ] Async closures/lambdas
- [ ] Integration with existing expression parser

**Effort:** Medium (~3-5 days)

### 2. Type Inference (40% TODO)

**Needed:**
- [ ] Automatic `Task<T>` wrapping for async functions
- [ ] `await` expression type unwrapping
- [ ] Effect tracking through call graph
- [ ] Async context validation (can't await outside async)

**Effort:** Medium (~2-4 days)

### 3. Runtime Implementation (80% TODO)

**Needed:**
- [ ] Task scheduler/executor
- [ ] Event loop
- [ ] Future/Promise implementation
- [ ] Async state machine generation
- [ ] Cooperative multitasking primitives

**Effort:** High (~1-2 weeks)

### 4. Standard Library (70% TODO)

**Needed:**
- [ ] `async` module with Task/Future APIs
- [ ] Timer/delay functions
- [ ] Async I/O primitives
- [ ] Channel/Stream implementations
- [ ] Combinators (then, and_then, join, select)

**Effort:** Medium (~1 week)

### 5. Code Generation (90% TODO)

**Needed:**
- [ ] Async function transformation to state machines
- [ ] Await point suspension/resumption
- [ ] Task creation bytecode
- [ ] Scheduling integration

**Effort:** High (~2-3 weeks)

---

## Example Syntax (Target)

### Basic Async Function

```nova
async fn fetch_data(url: String) -> Result<Data, Error> {
    let response = await http_get(url);
    let data = await response.json();
    yield Ok(data);
}
```

### Task Spawning

```nova
fn main() {
    let task1 = spawn { compute_heavy_work() };
    let task2 = spawn { fetch_from_network() };
    
    let result1 = await task1;
    let result2 = await task2;
    
    yield result1 + result2;
}
```

### Streams

```nova
async fn process_stream(input: Stream<i64>) -> i64 {
    var sum = 0;
    for await value in input {
        sum = sum + value;
    }
    yield sum;
}
```

---

## Implementation Priority

### Phase 1: Core Async (Highest Priority)
1. ✅ AST nodes (DONE)
2. ⚠️ Parser for `async fn` and `await`
3. ⚠️ Type system integration
4. ⚠️ Basic runtime executor

**Estimated Time:** 2-3 weeks

### Phase 2: Runtime & Scheduler
1. Event loop implementation
2. Task scheduling
3. State machine codegen
4. Cooperative multitasking

**Estimated Time:** 3-4 weeks

### Phase 3: Standard Library
1. Core async primitives
2. Timers and delays
3. Async I/O
4. Stream/Channel implementations

**Estimated Time:** 2-3 weeks

### Phase 4: Advanced Features
1. Async closures
2. Select/join combinators
3. Cancellation tokens
4. Backpressure handling

**Estimated Time:** 2-3 weeks

**Total Estimated Effort:** 9-13 weeks (2-3 months)

---

## Effect System Status

### What Exists

```nova
// Effect type definitions
cases Effect {
    IO,           // I/O operations
    Async,        // Async computations
    State(Type),  // Stateful operations
    Exception,    // Exception handling
    Nondet,       // Non-deterministic
    Custom(String),
}
```

**Status:** Types defined, no runtime support ⚠️

### What's Missing

- [ ] Effect tracking in type system
- [ ] Effect handlers (algebraic effects)
- [ ] Effect inference
- [ ] Effect composition rules
- [ ] Runtime effect interpreter

**Estimated Effort:** 6-8 weeks

---

## Comparison: Unit Algebra vs Async/Await

| Feature | Unit Algebra | Async/Await |
|---------|--------------|-------------|
| AST Support | ✅ 100% | ✅ 100% |
| Parser | ✅ 100% | ⚠️ 40% |
| Type System | ✅ 100% | ⚠️ 60% |
| Semantic | ✅ 100% | ⚠️ 30% |
| Codegen | ✅ 100% | ❌ 10% |
| Runtime | ✅ 100% | ❌ 20% |
| Stdlib | ✅ 100% | ⚠️ 30% |
| **TOTAL** | **✅ 100%** | **⚠️ 40%** |

---

## Recommendations

### Option 1: Complete Async/Await (Long-term)
**Pros:**
- Powerful concurrency model
- Modern language feature
- High developer demand

**Cons:**
- 2-3 months of work
- Complex runtime requirements
- Needs extensive testing

**Recommendation:** Good for v1.1 or v1.2 release

### Option 2: Focus on Other Features (Short-term)
Since Unit Algebra is already complete and working, consider:

1. **Pattern Matching** - Enhance existing support
2. **Module System** - Complete import/export
3. **Generics** - Full implementation
4. **Error Handling** - Result<T,E> and ? operator
5. **Borrow Checker** - Memory safety guarantees

**Recommendation:** Better ROI for v1.0 release

### Option 3: Hybrid Approach
1. Complete basic `async fn` and `await` parsing (1-2 weeks)
2. Simple green thread runtime (1-2 weeks)
3. Minimal stdlib support (1 week)
4. Mark as "experimental" feature

**Recommendation:** Balanced approach for v1.0

---

## Conclusion

**Async/Await Status:**
- Foundation: ✅ Solid (AST, types defined)
- Implementation: ⚠️ Incomplete (~40% done)
- Effort Required: 🔴 High (2-3 months for full implementation)

**Next Steps:**
1. ✅ Unit Algebra is COMPLETE and production-ready
2. ⚠️ Async/Await needs significant work
3. 💡 Consider prioritizing other features for v1.0
4. 📋 Add async/await to v1.1 roadmap

**Recommendation:** Ship v1.0 with complete unit algebra, add async/await in v1.1
