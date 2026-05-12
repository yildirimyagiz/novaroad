# Nova Compiler Organization

## Directory Structure

```
compiler/
├── frontend/           - Parsing & HIR/MIR generation
│   ├── parser.zn      - Syntax parser
│   ├── hir.zn         - High-level IR
│   ├── mir.zn         - Mid-level IR
│   ├── span.zn        - Source location tracking
│   └── errors.zn      - Error reporting
│
├── optimization/       - Optimization passes
│   ├── adaptive_simd.zn      - SIMD optimization
│   ├── egraph.zn             - E-graph optimizer
│   ├── learning_optimizer.zn - ML-based optimizer
│   ├── pattern_optimizer.zn  - Pattern matching
│   └── superoptimizer.zn     - Super optimization
│
├── analysis/           - Static analysis & verification
│   ├── smt.zn         - SMT solver integration
│   └── measurement.zn - Performance measurement
│
├── runtime/            - Runtime & JIT compilation
│   ├── jit.zn         - JIT compilation engine
│   ├── driver.zn      - Compiler driver
│   └── session.zn     - Compilation session management
│
├── utils/              - Common utilities
│   ├── data_structures.zn - Data structures
│   ├── interface.zn       - Public interfaces
│   ├── unified.zn         - Unified utilities
│   └── integration.zn     - Integration helpers
│
└── codegen/            - Code generation
    └── codegen.zn     - Code generation core
```

## Module Purposes

### Frontend (`compiler/frontend/`)
Handles source code parsing and initial IR generation:
- **parser.zn**: Parses Nova source code into AST
- **hir.zn**: High-level intermediate representation
- **mir.zn**: Mid-level intermediate representation
- **span.zn**: Tracks source code locations for error reporting
- **errors.zn**: Error message formatting and reporting

### Optimization (`compiler/optimization/`)
Various optimization strategies:
- **adaptive_simd.zn**: Auto-vectorization and SIMD optimization
- **egraph.zn**: E-graph based equality saturation
- **learning_optimizer.zn**: ML-based optimization decisions
- **pattern_optimizer.zn**: Pattern-based rewrite rules
- **superoptimizer.zn**: Exhaustive search optimization

### Analysis (`compiler/analysis/`)
Program analysis and verification:
- **smt.zn**: SMT solver integration for verification
- **measurement.zn**: Performance profiling and measurement

### Runtime (`compiler/runtime/`)
Runtime compilation and execution:
- **jit.zn**: Just-in-time compilation
- **driver.zn**: Main compiler driver logic
- **session.zn**: Manages compilation sessions

### Utils (`compiler/utils/`)
Common infrastructure:
- **data_structures.zn**: Shared data structures
- **interface.zn**: Public compiler interfaces
- **unified.zn**: Unified utility functions
- **integration.zn**: Integration helpers

### Codegen (`compiler/codegen/`)
Code generation infrastructure:
- **codegen.zn**: Core code generation logic

---

## Reorganization Benefits

✅ **Better Organization**: Related files grouped together
✅ **Easier Navigation**: Clear purpose for each directory
✅ **Scalability**: Easy to add new modules in appropriate categories
✅ **Maintainability**: Logical separation of concerns

---

Previous structure: 23 files in root directory
New structure: 5 organized directories + clean root
