# 🎯 Generics Backend Implementation Plan

**Duration**: 2 weeks (our speed: 1 week!)  
**Priority**: ⭐⭐⭐ CRITICAL  
**Status**: Starting Now

---

## 📋 Why Generics Backend is Critical

### Dependencies
- ✅ Result<T, E> needs generics
- ✅ Option<T> needs generics
- ✅ Vec<T>, HashMap<K, V> need generics
- ✅ All container types need generics

### Current Status
- ✅ Frontend: Type system designed (369 lines)
- ✅ Parser: Generic syntax ready
- ❌ Backend: NOT implemented
- ❌ Codegen: NOT implemented

---

## 🎯 Implementation Strategy

### Approach: Monomorphization (Rust-style)

**Concept**: 
- Generic functions are templates
- At compile-time, create concrete version for each type used
- No runtime overhead!

**Example**:
```nova
// Generic function
fn max<T>(a: T, b: T) -> T {
    check a > b { yield a; } else { yield b; }
}

// Usage
let x = max(10, 20);      // T = i64
let y = max(3.14, 2.71);  // T = f64

// After monomorphization (compile-time):
fn max_i64(a: i64, b: i64) -> i64 {
    check a > b { yield a; } else { yield b; }
}

fn max_f64(a: f64, b: f64) -> f64 {
    check a > b { yield a; } else { yield b; }
}

let x = max_i64(10, 20);
let y = max_f64(3.14, 2.71);
```

---

## 📁 Implementation Phases

### Phase 1: Type Parameter Collection (Day 1)
- [ ] Collect generic parameters from function signatures
- [ ] Store type parameters in symbol table
- [ ] Track generic constraints (bounds)

### Phase 2: Type Inference (Day 2)
- [ ] Infer concrete types at call sites
- [ ] Unification algorithm
- [ ] Constraint solving

### Phase 3: Monomorphization (Day 3-4)
- [ ] Generate concrete functions for each type
- [ ] Name mangling (max_i64, max_f64)
- [ ] Duplicate checking (don't generate twice)

### Phase 4: Code Generation (Day 5-6)
- [ ] Generate bytecode for monomorphized functions
- [ ] Handle generic structs/enums
- [ ] Generic method calls

### Phase 5: Testing (Day 7)
- [ ] Unit tests
- [ ] Integration tests
- [ ] Performance tests

---

## 🔧 Technical Details

### Data Structures Needed

```c
// Generic function instance
typedef struct {
    char *base_name;              // "max"
    Type **concrete_types;        // [i64]
    size_t type_count;
    char *mangled_name;           // "max_i64"
    bool is_generated;
} GenericInstance;

// Generic function registry
typedef struct {
    GenericFunction *functions;
    size_t count;
    size_t capacity;
} GenericRegistry;

// Type parameter
typedef struct {
    char *name;                   // "T"
    Type **bounds;                // [Display, Debug]
    size_t bound_count;
} TypeParameter;
```

### Name Mangling Rules

```
Function: max<i64> → max_i64
Function: max<f64> → max_f64
Function: max<Vec<i64>> → max_Vec_i64
Struct: Vec<i64> → Vec_i64
Enum: Option<String> → Option_String
```

---

## 📊 Files to Create/Modify

### New Files
- `src/compiler/generics.c` (500+ lines)
  - Type parameter collection
  - Monomorphization engine
  - Generic registry

- `src/compiler/type_inference.c` (300+ lines)
  - Type unification
  - Constraint solving
  - Type variable substitution

- `tests/unit/test_generics.c` (200+ lines)
  - Generic function tests
  - Generic struct tests
  - Type inference tests

### Modified Files
- `src/compiler/semantic.c`
  - Add generic type checking
  
- `src/compiler/codegen.c`
  - Generate monomorphized functions
  
- `src/compiler/parser.c`
  - Already parses generics ✅

---

## 🎯 Test Cases

### Test 1: Generic Function
```nova
fn identity<T>(x: T) -> T { yield x; }
assert(identity(42) == 42);
assert(identity(3.14) == 3.14);
```

### Test 2: Generic Struct
```nova
data Box<T> {
    value: T,
}

let b1 = Box { value: 10 };     // Box<i64>
let b2 = Box { value: "hi" };   // Box<String>
```

### Test 3: Option<T>
```nova
cases Option<T> {
    Some(T),
    None,
}

fn unwrap_or<T>(opt: Option<T>, default: T) -> T {
    match opt {
        Some(x) => x,
        None => default,
    }
}
```

### Test 4: Result<T, E>
```nova
cases Result<T, E> {
    Ok(T),
    Err(E),
}

fn map<T, U, E>(result: Result<T, E>, f: fn(T) -> U) -> Result<U, E> {
    match result {
        Ok(x) => Ok(f(x)),
        Err(e) => Err(e),
    }
}
```

### Test 5: Constraints
```nova
fn print_all<T: Display>(items: Vec<T>) {
    for item in items {
        println(item);
    }
}
```

---

## 🚀 Implementation Order

### Day 1-2: Core Infrastructure
1. Type parameter structures
2. Generic registry
3. Type inference basics

### Day 3-4: Monomorphization
4. Instance generation
5. Name mangling
6. Duplicate detection

### Day 5-6: Code Generation
7. Monomorphized function codegen
8. Generic struct handling
9. Generic enum handling

### Day 7: Testing & Polish
10. Comprehensive tests
11. Edge cases
12. Performance optimization

---

## 💡 Key Algorithms

### Type Unification
```
unify(T, i64):
  if T is type variable:
    T = i64
  else if T == i64:
    success
  else:
    error: type mismatch
```

### Monomorphization
```
for each generic function:
  collect all call sites
  for each call site:
    infer concrete types
    check if instance exists
    if not:
      generate new instance
      add to registry
```

---

## 🎯 Success Criteria

- [ ] Generic functions work
- [ ] Generic structs work
- [ ] Generic enums work
- [ ] Option<T> works
- [ ] Result<T, E> works
- [ ] Type inference works
- [ ] Constraints work
- [ ] All tests pass

---

**Ready to start Day 1: Type Parameter Collection?** 🚀
