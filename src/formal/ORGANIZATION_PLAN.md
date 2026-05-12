# Formal Verification Folder Organization Plan

## Current Files Analysis

### 1️⃣ **Verification Core** → `verification/`
- nova_verification.c
- nova_ai_verification.c
- nova_verify_graph.c

### 2️⃣ **Proof Systems** → `proofs/`
- nova_proof.c
- nova_formal.c
- nova_prover.c

### 3️⃣ **Solvers & Decision** → `solvers/`
- nova_solver.c
- nova_decision_system.c

### 4️⃣ **Invariants & Contracts** → `invariants/`
- nova_invariants.c
- nova_invariant_engine.c
- nova_kernel_contracts.c
- nova_graph_obligations.c

### 5️⃣ **Safety & Bounds** → `safety/`
- nova_bounds_engine.c
- nova_error_recovery.c

### 6️⃣ **Testing & Validation** → `testing/`
- nova_deterministic.c
- nova_deterministic_kernel.c

## New Structure:
```
formal/
├── verification/
│   ├── nova_verification.c
│   ├── nova_ai_verification.c
│   └── nova_verify_graph.c
├── proofs/
│   ├── nova_proof.c
│   ├── nova_formal.c
│   └── nova_prover.c
├── solvers/
│   ├── nova_solver.c
│   └── nova_decision_system.c
├── invariants/
│   ├── nova_invariants.c
│   ├── nova_invariant_engine.c
│   ├── nova_kernel_contracts.c
│   └── nova_graph_obligations.c
├── safety/
│   ├── nova_bounds_engine.c
│   └── nova_error_recovery.c
└── testing/
    ├── nova_deterministic.c
    └── nova_deterministic_kernel.c
```

## Categories:
- **Verification**: Core verification logic
- **Proofs**: Mathematical proof systems
- **Solvers**: Constraint & decision solvers
- **Invariants**: System invariants & contracts
- **Safety**: Bounds checking & error recovery
- **Testing**: Deterministic testing & validation
