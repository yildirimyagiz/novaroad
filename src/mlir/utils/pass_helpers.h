//===- pass_helpers.h - MLIR Pass Utilities ----------------------*- C++
//-*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file provides utility functions for running MLIR pass pipelines.
// Now integrated with the Pass Pipeline Manager for production-grade stability.
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_UTILS_PASS_HELPERS_H
#define NOVA_MLIR_UTILS_PASS_HELPERS_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/LogicalResult.h"

namespace nova {
namespace mlir_utils {

//===----------------------------------------------------------------------===//
// Pipeline Modes (Legacy - for backward compatibility)
//===----------------------------------------------------------------------===//

enum class PipelineMode {
  // Verify IR only
  VerifyOnly,

  // Run optimizations
  Optimize,

  // Run optimizations + verify
  OptimizeAndVerify
};

//===----------------------------------------------------------------------===//
// Legacy Pipeline Runner (Deprecated - use PassPipelineManager instead)
//===----------------------------------------------------------------------===//

/// Run a simple pipeline on a module
/// NOTE: This is deprecated. Use PassPipelineManager for production code.
[[deprecated("Use PassPipelineManager::buildPipeline instead")]]
mlir::LogicalResult runPipeline(mlir::ModuleOp module, PipelineMode mode,
                                bool enableVerifier = true,
                                bool printIR = false);

//===----------------------------------------------------------------------===//
// Modern Pass Helpers (Integrated with Pass Pipeline Manager)
//===----------------------------------------------------------------------===//

/// Run standard Nova compilation pipeline
mlir::LogicalResult runStandardPipeline(mlir::ModuleOp module,
                                        unsigned optLevel = 2,
                                        bool enableVerifier = true,
                                        bool printIR = false);

/// Run verification-only pipeline
mlir::LogicalResult runVerificationPipeline(mlir::ModuleOp module,
                                            bool printIR = false);

/// Run Mojo-class performance pipeline
mlir::LogicalResult runMojoClassPipeline(mlir::ModuleOp module,
                                         bool enableVerifier = true,
                                         bool printIR = false);

/// Add common canonicalization passes
void addCommonCanonical(mlir::PassManager &pm);

/// Add verification passes
void addVerificationPasses(mlir::PassManager &pm);

/// Add optimization passes based on level
void addOptimizationPasses(mlir::PassManager &pm, unsigned optLevel);

//===----------------------------------------------------------------------===//
// Debug Utilities
//===----------------------------------------------------------------------===//

/// Configure pass manager for debugging
void configureDebugMode(mlir::PassManager &pm);

/// Dump IR after each pass
void enableIRDumping(mlir::PassManager &pm);

/// Enable timing statistics
void enableTiming(mlir::PassManager &pm);

} // namespace mlir_utils
} // namespace nova

#endif // NOVA_MLIR_UTILS_PASS_HELPERS_H
