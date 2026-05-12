//===- pass_registry.cpp - Nova Pass Registry ------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "pass_registry.h"
#include "mlir/Pass/PassRegistry.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Pass Registration
//===----------------------------------------------------------------------===//

void registerNovaPasses() {
  // Type safety passes
  registerPass(
      []() -> std::unique_ptr<Pass> { return createBorrowCheckPass(); });

  registerPass(
      []() -> std::unique_ptr<Pass> { return createLinearTypesPass(); });

  registerPass(
      []() -> std::unique_ptr<Pass> { return createEffectCheckPass(); });

  registerPass([]() -> std::unique_ptr<Pass> { return createLifetimePass(); });

  // Advanced type system passes
  registerPass(
      []() -> std::unique_ptr<Pass> { return createDependentTypesPass(); });

  registerPass(
      []() -> std::unique_ptr<Pass> { return createTerminationPass(); });

  registerPass([]() -> std::unique_ptr<Pass> { return createEqualityPass(); });

  registerPass(
      []() -> std::unique_ptr<Pass> { return createTraitSolverPass(); });

  // Optimization passes (Mojo-level)
  registerPass(
      []() -> std::unique_ptr<Pass> { return createVectorizationPass(); });

  registerPass([]() -> std::unique_ptr<Pass> { return createGPUMetalPass(); });

  registerPass([]() -> std::unique_ptr<Pass> { return createAsyncPass(); });

  registerPass(
      []() -> std::unique_ptr<Pass> { return createLoopOptimizationPass(); });

  registerPass(
      []() -> std::unique_ptr<Pass> { return createAutoParallelPass(); });

  registerPass([]() -> std::unique_ptr<Pass> { return createPGOPass(); });

  registerPass(
      []() -> std::unique_ptr<Pass> { return createCanonicalizationPass(); });
}

void registerTypeSafetyPasses() {
  registerPass([]() { return createBorrowCheckPass(); });
  registerPass([]() { return createLinearTypesPass(); });
  registerPass([]() { return createEffectCheckPass(); });
  registerPass([]() { return createLifetimePass(); });
}

void registerAdvancedTypePasses() {
  registerPass([]() { return createDependentTypesPass(); });
  registerPass([]() { return createTerminationPass(); });
  registerPass([]() { return createEqualityPass(); });
  registerPass([]() { return createTraitSolverPass(); });
}

void registerOptimizationPasses() {
  registerPass([]() { return createVectorizationPass(); });
  registerPass([]() { return createGPUMetalPass(); });
  registerPass([]() { return createAsyncPass(); });
  registerPass([]() { return createLoopOptimizationPass(); });
  registerPass([]() { return createAutoParallelPass(); });
  registerPass([]() { return createPGOPass(); });
  registerPass([]() { return createCanonicalizationPass(); });
}

} // namespace nova
} // namespace mlir
