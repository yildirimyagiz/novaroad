# 🎓 Gradient Type System - Implementation Plan

**Feature**: Compile-time gradient tracking  
**Status**: Planned  
**Priority**: P1 (UNIQUE feature!)

---

## 🎯 Vision

Make gradients part of the type system to catch backpropagation bugs at compile-time:

```nova
fn train(model: Model<Diff>) {
    let loss = model.forward(x);
    loss.backward();  // ✓ Compile-time checked!
}

// ❌ Forgot backward() → COMPILE ERROR
// ❌ Gradient shape mismatch → COMPILE ERROR
```

---

## 📋 Design

### Type Definition
```c
// In nova_types.h
typedef struct {
    Type *value_type;      // T (the actual type)
    Type *grad_type;       // dT (gradient type)
    bool requires_grad;    // Track gradients?
} DiffTypeData;

// Add to TypeKind
TYPE_DIFF,  // Differentiable type

// Add to Type union
DiffTypeData diff;
```

### Nova Syntax
```nova
// Differentiable types
data Diff<T> {
    value: T,
    grad: Option<T>,
}

// Automatic gradient tracking
fn loss(pred: Diff<f64>, target: f64) -> Diff<f64> {
    let error = pred - target;
    return error * error;  // Gradient auto-tracked
}

// Explicit control
fn no_grad<T>(x: Diff<T>) -> T {
    x.value  // Drop gradient
}
```

---

## 🚀 Implementation Phases

### Phase 1: Type System (Week 5-6)
- [ ] Add TYPE_DIFF to TypeKind
- [ ] Add DiffTypeData struct
- [ ] Add type creators
- [ ] Add type queries

### Phase 2: AST & Parsing
- [ ] Add DiffTypeNode
- [ ] Parse Diff<T> syntax
- [ ] Parse .backward() calls
- [ ] Parse .value, .grad accessors

### Phase 3: Semantic Analysis
- [ ] Track differentiable context
- [ ] Verify .backward() calls
- [ ] Check gradient flow
- [ ] Error messages

### Phase 4: Code Generation
- [ ] Generate gradient computation code
- [ ] Optimize away unused gradients
- [ ] Integration with autograd

---

## 💡 Key Features

### 1. Compile-Time Verification
```nova
fn train(model: Model<Diff>) {
    let loss = model.forward(x);
    // Forgot loss.backward() → COMPILE ERROR
    optimizer.step();  // Error: gradients not computed!
}
```

### 2. Gradient Shape Checking
```nova
fn add_gradients(a: Diff<Tensor[M, N]>, b: Diff<Tensor[K, L]>) {
    // ❌ COMPILE ERROR: gradient shapes don't match
    return a + b;
}
```

### 3. Zero-Cost Abstraction
```nova
fn inference(model: Model) {  // No Diff<>
    // Gradients completely optimized away
    // Same performance as manual code
    return model.forward(x);
}
```

---

## 📊 Impact

- **Safety**: Catch ML bugs at compile-time
- **Performance**: Zero overhead when not needed
- **Uniqueness**: NO other language has this!

---

## 🔗 References

- Main TODO: `../TODO.md`
- ML Proposal: `/Users/yldyagz/novaRoad/nova/ML_TRAINING_ACCELERATION_PROPOSAL.md`
