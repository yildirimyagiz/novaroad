//===- pass_registry.h - Nova Pass Registry ------------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file declares the Nova MLIR pass registry.
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_PASSES_PASS_REGISTRY_H
#define NOVA_MLIR_PASSES_PASS_REGISTRY_H

#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Type System Passes
//===----------------------------------------------------------------------===//

/// Create a pass for borrow checking
std::unique_ptr<Pass> createBorrowCheckPass();

/// Create a pass for linear type checking
std::unique_ptr<Pass> createLinearTypesPass();

/// Create a pass for effect checking/inference
std::unique_ptr<Pass> createEffectCheckPass();

/// Create a pass for lifetime inference
std::unique_ptr<Pass> createLifetimePass();

//===----------------------------------------------------------------------===//
// Advanced Type System Passes
//===----------------------------------------------------------------------===//

/// Create a pass for dependent type checking
std::unique_ptr<Pass> createDependentTypesPass();

/// Create a pass for termination checking
std::unique_ptr<Pass> createTerminationPass();

/// Create a pass for equality checking
std::unique_ptr<Pass> createEqualityPass();

/// Create a pass for trait solving
std::unique_ptr<Pass> createTraitSolverPass();

//===----------------------------------------------------------------------===//
// Optimization Passes (Mojo-level)
//===----------------------------------------------------------------------===//

/// Create vectorization pass for SIMD optimization
std::unique_ptr<Pass> createVectorizationPass();

/// Create GPU/Metal backend optimization pass
std::unique_ptr<Pass> createGPUMetalPass();

/// Create async/await lowering pass
std::unique_ptr<Pass> createAsyncPass();

/// Create loop optimization pass (fusion, unrolling, tiling)
std::unique_ptr<Pass> createLoopOptimizationPass();

/// Create auto-parallelization pass
std::unique_ptr<Pass> createAutoParallelPass();

/// Create profile-guided optimization pass
std::unique_ptr<Pass> createPGOPass();

/// Create canonicalization pass
std::unique_ptr<Pass> createCanonicalizationPass();

//===----------------------------------------------------------------------===//
// Pass Registration
//===----------------------------------------------------------------------===//

/// Register all Nova passes
void registerNovaPasses();

/// Register individual pass categories
void registerTypeSafetyPasses();
void registerAdvancedTypePasses();
void registerOptimizationPasses();

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_PASSES_PASS_REGISTRY_H
