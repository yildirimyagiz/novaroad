# Nova Compiler Modes: Determinism First, Intelligence Second

## Core Philosophy

```
Compiler = Physics Engine (NEVER optional)
AI = Control System (ALWAYS bounded)
```

**Universal Rule**: Önce determinism, sonra intelligence 🚀

---

## Two-Mode Architecture

### Mode 1: DEV MODE (AI-Heavy Exploration)

**Purpose**: Fast iteration, AI learning, experimentation

**What's ENABLED**:
- ✅ AI pass ordering experiments
- ✅ Aggressive optimizations
- ✅ Heuristic tuning
- ✅ Extra diagnostics
- ✅ Runtime assertions
- ✅ IR profiling
- ✅ Cost model training

**What's STILL ENFORCED** (Hard Constraints):
- ✅ Ownership invariants
- ✅ Effect legality
- ✅ IR structural correctness
- ✅ Pass dependency graph
- ✅ Type safety
- ✅ Memory safety

**Characteristics**:
```
Speed:         Fast (minimal verification)
Determinism:   Relaxed (AI can experiment)
Safety:        ENFORCED (physics layer active)
Use case:      Development, prototyping, tuning
```

**Example**:
```bash
nova compile --mode=dev --ai-tuning=aggressive mycode.zn
```

---

### Mode 2: PRODUCTION MODE (Deterministic + Verified)

**Purpose**: Reproducible builds, formal guarantees, deployment

**What's ENABLED**:
- ✅ Fixed pass pipelines
- ✅ Gödel verification gates
- ✅ Formal proof checking
- ✅ Deterministic ordering
- ✅ Reproducible builds

**What's DISABLED/LIMITED**:
- 🟡 AI is advisory only (offline learning)
- 🟡 No runtime experiments
- 🟡 No heuristic variations

**Characteristics**:
```
Speed:         Slower (full verification)
Determinism:   STRICT (bit-for-bit reproducible)
Safety:        MAXIMUM (all gates active)
Use case:      Production, security-critical, certified systems
```

**Example**:
```bash
nova compile --mode=production --verify=full mycode.zn
```

---

## Hard Constraints (NEVER Optional)

These are the "physics engine" - always active in BOTH modes:

### 1. Ownership Invariants
```
✅ No use-after-free (verified at compile time)
✅ No data races (proven safe)
✅ Borrowing rules enforced
```

### 2. Effect Legality
```
✅ IO effects tracked
✅ State mutations verified
✅ Side-effect ordering guaranteed
```

### 3. IR Structural Correctness
```
✅ MLIR verification passes
✅ Type consistency
✅ SSA form validity
```

### 4. Pass Dependency Graph
```
✅ Pass ordering constraints
✅ Required analysis availability
✅ Invalidation tracking
```

**These constraints are ENFORCED even when AI is exploring.**

---

## Soft Intelligence Layer (AI Playground)

AI can optimize these WITHOUT breaking physics:

### 1. Pass Ordering Optimization
```
AI learns: "For this code pattern, run inliner before loop fusion"
Physics ensures: Dependencies still respected
```

### 2. Cost Model Learning
```
AI learns: "This SIMD pattern is 10x faster on AVX-512"
Physics ensures: Transformation is semantically correct
```

### 3. Vectorization Decisions
```
AI learns: "Vectorize this loop with width=8"
Physics ensures: No data races, correct memory access
```

### 4. Specialization Strategies
```
AI learns: "Specialize this function for i32"
Physics ensures: Type safety, no undefined behavior
```

---

## Why This Architecture Wins

### ❌ Wrong Approach (Pure AI)
```
AI optimizes everything
   ↓
No hard constraints
   ↓
Miscompiles leak into dataset
   ↓
AI learns incorrect optimizations
   ↓
CHAOS
```

### ✅ Right Approach (Nova)
```
Physics layer enforces correctness
   ↓
AI explores within safe bounds
   ↓
Only correct optimizations in dataset
   ↓
AI learns valid patterns
   ↓
STABILITY + INTELLIGENCE
```

---

## Real-World Example

### Scenario: Loop Vectorization

**DEV MODE** (AI exploring):
```nova
// AI tries different SIMD widths
for i in 0..1024 {
    array[i] = array[i] * 2;
}

AI experiments:
- Width 4? (measure performance)
- Width 8? (measure performance)  
- Width 16? (measure performance)

Physics layer ensures:
✅ No out-of-bounds access
✅ No data races
✅ Correct semantics preserved
```

**PRODUCTION MODE** (deterministic):
```nova
// Fixed pipeline, no experiments
for i in 0..1024 {
    array[i] = array[i] * 2;
}

Compiler:
- Uses learned best width (8)
- Verifies with Gödel proof
- Generates deterministic code
- Bit-for-bit reproducible
```

---

## Mode Comparison Table

| Feature | DEV Mode | PRODUCTION Mode |
|---------|----------|-----------------|
| **AI Experimentation** | ✅ Active | 🟡 Advisory |
| **Pass Ordering** | 🔄 Dynamic | ✅ Fixed |
| **Optimization Level** | 🔥 Aggressive | ⚖️ Balanced |
| **Verification** | 🟡 Minimal | ✅ Full |
| **Determinism** | 🟡 Relaxed | ✅ Strict |
| **Build Speed** | ⚡ Fast | 🐢 Slower |
| **Reproducibility** | ❌ No | ✅ Yes |
| **Gödel Proofs** | 🟡 Optional | ✅ Required |
| **Ownership Checks** | ✅ ALWAYS | ✅ ALWAYS |
| **Effect Safety** | ✅ ALWAYS | ✅ ALWAYS |
| **IR Correctness** | ✅ ALWAYS | ✅ ALWAYS |

**Key**: Hard constraints ALWAYS active (even in DEV)

---

## Implementation Strategy

### Phase 1: Establish Physics Engine (Current)
```
✅ Pipeline Manager (done today)
✅ Hard constraint enforcement
✅ Deterministic baseline
```

### Phase 2: Add AI Layer (Future)
```
🔶 AI pass tuning
🔶 Cost model learning
🔶 Pattern recognition
```

**Critical**: Phase 1 MUST be solid before Phase 2

**Why**: AI needs stable foundation to learn from

---

## The Golden Rule

```
┌─────────────────────────────────────────────────────┐
│                                                     │
│  "AI can suggest, but physics must enforce"        │
│                                                     │
│  Learning system ≠ Proof system                    │
│  Optimization ≠ Correctness                        │
│                                                     │
│  Nova enforces BOTH.                             │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## Why Optional Stability is Dangerous

```
❌ If stability was optional:

AI in experimental mode
   ↓
Generates incorrect IR
   ↓
No enforcement layer
   ↓
Bad optimization gets learned
   ↓
Dataset contaminated
   ↓
AI optimizes for chaos
   ↓
COMPILER COLLAPSE
```

```
✅ With mandatory stability:

AI in experimental mode
   ↓
Tries incorrect optimization
   ↓
Physics layer rejects it
   ↓
AI never sees bad result
   ↓
Dataset stays clean
   ↓
AI learns only valid patterns
   ↓
STABLE INTELLIGENCE
```

---

## Lessons from Industry

### LLVM Experience
- O0: Fast, minimal verification
- O3: Aggressive, but verifier ALWAYS runs
- **Never**: "Optional correctness"

### Rust Experience
- Debug: Fast compile, extra checks
- Release: Optimized, but safety ALWAYS enforced
- **Never**: "Optional borrow checking"

### Nova's Approach
- DEV: Fast iteration, AI exploration
- PROD: Formal verification, determinism
- **Always**: Ownership + effects + IR correctness

---

## Conclusion

### The Right Decision

✅ **Two modes**: DEV (exploratory) + PROD (deterministic)  
✅ **Hard constraints**: NEVER optional  
✅ **AI intelligence**: ALWAYS bounded  
✅ **Physics first**: Foundation before experimentation  

### The Wrong Decision Would Be

❌ Optional stability layer  
❌ AI without constraints  
❌ Intelligence before determinism  

---

**Nova's commitment**:

> "We build the physics engine first.  
> Then we let AI play within the laws of physics.  
> Never the other way around."

This is **compiler evolution best practice**. 🚀

---

**Status**: Architectural Decision Confirmed  
**Principle**: Determinism First, Intelligence Second  
**Implementation**: Hard constraints mandatory, AI bounded  
**Date**: February 14, 2026
