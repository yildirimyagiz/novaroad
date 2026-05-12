// ╔═══════════════════════════════════════════════════════════════════════════════╗
// ║ Nova MLIR Compiler Passes - TableGen Definitions                            ║
// ║ Type System, Effect Analysis, and Optimization Passes                          ║
// ╚═══════════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_PASSES
#define NOVA_PASSES

include "mlir/Pass/PassBase.td"

// ============================================================================
// OWNERSHIP & BORROW CHECKING PASSES
// ============================================================================

def NovaBorrowCheckPass : Pass<"nova-borrow-check", "mlir::ModuleOp"> {
  let summary = "Verify ownership and borrowing rules";
  let description = [{
    Implements Rust-like borrow checking at the MLIR level:
    - Ensures no aliasing between mutable and immutable borrows
    - Validates move semantics (use-after-move detection)
    - Checks lifetime constraints
    
    This pass operates on Nova dialect operations and produces
    diagnostic errors for violations.
  }];
  
  let constructor = "mlir::nova::createBorrowCheckPass()";
  let dependentDialects = ["nova::NovaDialect"];
}

def NovaLifetimeInferencePass : Pass<"nova-lifetime-inference", "mlir::FuncOp"> {
  let summary = "Infer and validate lifetimes (NLL-style)";
  let description = [{
    Non-Lexical Lifetime (NLL) inference:
    - Computes minimal lifetimes for all references
    - Builds constraint graph
    - Solves via topological sort
    - Reports lifetime errors with precise locations
  }];
  
  let constructor = "mlir::nova::createLifetimeInferencePass()";
}

def NovaOwnershipAnalysisPass : Pass<"nova-ownership-analysis", "mlir::ModuleOp"> {
  let summary = "Analyze ownership transfer and value moves";
  let description = [{
    Tracks ownership through the program:
    - Identifies move operations
    - Detects use-after-move
    - Validates drop points
    - Ensures linear resources are consumed exactly once
  }];
  
  let constructor = "mlir::nova::createOwnershipAnalysisPass()";
}

// ============================================================================
// EFFECT SYSTEM PASSES
// ============================================================================

def NovaEffectInferencePass : Pass<"nova-effect-inference", "mlir::ModuleOp"> {
  let summary = "Infer effects for all functions";
  let description = [{
    Bottom-up effect inference:
    - Analyzes all operations in function bodies
    - Propagates effects through call graph
    - Annotates functions with inferred effect sets
    - Marks pure functions for optimization
  }];
  
  let constructor = "mlir::nova::createEffectInferencePass()";
  let options = [
    Option<"trustUnannotated", "trust-unannotated", "bool",
           /*default=*/"false",
           "Trust functions without effect annotations as pure">
  ];
}

def NovaEffectCheckPass : Pass<"nova-effect-check", "mlir::ModuleOp"> {
  let summary = "Validate effect annotations";
  let description = [{
    Checks that declared effects match actual effects:
    - Ensures pure functions have no side effects
    - Validates effect boundaries
    - Reports violations with proof levels
  }];
  
  let constructor = "mlir::nova::createEffectCheckPass()";
}

// ============================================================================
// LINEAR TYPE PASSES
// ============================================================================

def NovaLinearTypeCheckPass : Pass<"nova-linear-check", "mlir::FuncOp"> {
  let summary = "Enforce linear type usage rules";
  let description = [{
    Validates linear type constraints:
    - Each linear value used exactly once
    - No implicit copies or drops
    - Detects use-after-consume
    - Validates resource cleanup
  }];
  
  let constructor = "mlir::nova::createLinearTypeCheckPass()";
}

def NovaAffineAnalysisPass : Pass<"nova-affine-analysis", "mlir::FuncOp"> {
  let summary = "Analyze affine type usage patterns";
  let description = [{
    Determines which values can be freely copied/dropped:
    - Identifies affine types (structural rules)
    - Enables copy propagation for affine values
    - Optimizes away unnecessary copies
  }];
  
  let constructor = "mlir::nova::createAffineAnalysisPass()";
}

// ============================================================================
// ADVANCED ANALYSIS PASSES
// ============================================================================

def NovaEscapeAnalysisPass : Pass<"nova-escape-analysis", "mlir::FuncOp"> {
  let summary = "Escape analysis for stack allocation optimization";
  let description = [{
    Determines which allocations can be stack-based:
    - Tracks value flow through the program
    - Identifies non-escaping allocations
    - Enables stack promotion (heap → stack)
    - Reduces GC pressure
  }];
  
  let constructor = "mlir::nova::createEscapeAnalysisPass()";
}

def NovaAliasAnalysisPass : Pass<"nova-alias-analysis", "mlir::ModuleOp"> {
  let summary = "Precise alias analysis using ownership info";
  let description = [{
    Leverages ownership system for precise aliasing:
    - Uses move semantics to prove non-aliasing
    - Enables aggressive optimizations
    - Powers vectorization and parallelization
  }];
  
  let constructor = "mlir::nova::createAliasAnalysisPass()";
}

def NovaRegionAnalysisPass : Pass<"nova-region-analysis", "mlir::ModuleOp"> {
  let summary = "Region-based memory analysis";
  let description = [{
    Divides program into memory regions:
    - Each region has defined lifetime
    - Bulk deallocation at region end
    - Reduces individual free() calls
    - Similar to arena allocation
  }];
  
  let constructor = "mlir::nova::createRegionAnalysisPass()";
}

// ============================================================================
// OPTIMIZATION PASSES
// ============================================================================

def NovaAutoParallelizationPass : Pass<"nova-auto-parallel", "mlir::FuncOp"> {
  let summary = "Automatic parallelization using effect analysis";
  let description = [{
    Automatically parallelizes pure code:
    - Identifies pure sub-expressions
    - No effects = safe to parallelize
    - Generates async/parallel constructs
    - Targets multi-core and GPU
  }];
  
  let constructor = "mlir::nova::createAutoParallelizationPass()";
  let options = [
    Option<"minGrainSize", "min-grain-size", "int64_t",
           /*default=*/"1000",
           "Minimum work size for parallel task (in ops)">
  ];
}

def NovaMemoryCoalescingPass : Pass<"nova-memory-coalesce", "mlir::FuncOp"> {
  let summary = "Coalesce memory operations";
  let description = [{
    Uses ownership info to safely merge memory ops:
    - Combine adjacent loads/stores
    - Eliminate redundant copies
    - Optimize borrow patterns
  }];
  
  let constructor = "mlir::nova::createMemoryCoalescingPass()";
}

def NovaZeroCostAbstractionPass : Pass<"nova-zero-cost", "mlir::ModuleOp"> {
  let summary = "Eliminate abstraction overhead";
  let description = [{
    Ensures zero-cost abstractions:
    - Inline all ownership operations
    - Eliminate borrow/unborrow pairs
    - Optimize lifetime bookkeeping away
    - Reduce to raw memory operations
  }];
  
  let constructor = "mlir::nova::createZeroCostAbstractionPass()";
}

// ============================================================================
// PASS PIPELINES
// ============================================================================

def NovaTypeSafetyPipeline : PassPipeline<"nova-type-safety"> {
  let summary = "Complete type safety validation pipeline";
  let description = [{
    Runs all type safety checks:
    1. Ownership analysis
    2. Borrow checking
    3. Lifetime inference
    4. Linear type checking
    5. Effect validation
  }];
}

def NovaOptimizationPipeline : PassPipeline<"nova-optimize"> {
  let summary = "Full optimization pipeline leveraging type system";
  let description = [{
    Optimization passes that use type information:
    1. Escape analysis
    2. Alias analysis
    3. Effect-based parallelization
    4. Memory coalescing
    5. Zero-cost abstraction elimination
  }];
}

#endif // NOVA_PASSES
