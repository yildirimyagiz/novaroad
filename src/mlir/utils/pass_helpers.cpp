//===- pass_helpers.cpp - MLIR Pass Utilities ----------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "pass_helpers.h"
#include "../passes/pass_pipeline_manager.h"
#include "../passes/pass_registry.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Transforms/Passes.h"

namespace nova {
namespace mlir_utils {

//===----------------------------------------------------------------------===//
// Helper Functions
//===----------------------------------------------------------------------===//

void addCommonCanonical(mlir::PassManager &pm) {
  pm.addPass(mlir::createCanonicalizerPass());
  pm.addPass(mlir::createCSEPass());
  pm.addPass(mlir::createSymbolDCEPass());
}

void addVerificationPasses(mlir::PassManager &pm) {
  // These will be registered in pass_registry
  // For now, just enable the built-in verifier
  pm.enableVerifier(true);
}

void addOptimizationPasses(mlir::PassManager &pm, unsigned optLevel) {
  if (optLevel >= 1) {
    addCommonCanonical(pm);
  }

  if (optLevel >= 2) {
    pm.addPass(mlir::createInlinerPass());
    addCommonCanonical(pm);
  }

  if (optLevel >= 3) {
    // Aggressive optimizations
    pm.addPass(mlir::createLoopInvariantCodeMotionPass());
    addCommonCanonical(pm);
  }
}

void configureDebugMode(mlir::PassManager &pm) {
  pm.enableVerifier(true);
  pm.enableTiming();
  enableIRDumping(pm);
}

void enableIRDumping(mlir::PassManager &pm) {
  pm.getContext()->disableMultithreading();
  pm.enableIRPrinting([](mlir::Pass *, mlir::Operation *) { return true; },
                      [](mlir::Pass *, mlir::Operation *) { return true; },
                      /*printModuleScope=*/true,
                      /*printAfterOnlyOnChange=*/false);
}

void enableTiming(mlir::PassManager &pm) { pm.enableTiming(); }

//===----------------------------------------------------------------------===//
// Legacy Pipeline Runner (Deprecated)
//===----------------------------------------------------------------------===//

mlir::LogicalResult runPipeline(mlir::ModuleOp module, PipelineMode mode,
                                bool enableVerifier, bool printIR) {

  if (!module)
    return mlir::failure();

  mlir::MLIRContext *ctx = module.getContext();
  mlir::PassManager pm(ctx);

  pm.enableVerifier(enableVerifier);

  if (printIR) {
    enableIRDumping(pm);
  }

  switch (mode) {
  case PipelineMode::VerifyOnly: {
    // Just run verifier (enabled above)
    break;
  }
  case PipelineMode::Optimize: {
    addCommonCanonical(pm);
    break;
  }
  case PipelineMode::OptimizeAndVerify: {
    addCommonCanonical(pm);
    // Run twice for better convergence
    addCommonCanonical(pm);
    break;
  }
  }

  return pm.run(module);
}

//===----------------------------------------------------------------------===//
// Modern Pipeline Runners (Using Pass Pipeline Manager)
//===----------------------------------------------------------------------===//

mlir::LogicalResult runStandardPipeline(mlir::ModuleOp module,
                                        unsigned optLevel, bool enableVerifier,
                                        bool printIR) {

  if (!module)
    return mlir::failure();

  mlir::MLIRContext *ctx = module.getContext();
  mlir::PassManager pm(ctx);

  // Configure pass manager
  pm.enableVerifier(enableVerifier);
  if (printIR) {
    enableIRDumping(pm);
  }

  // Use Pass Pipeline Manager for production-grade stability
  nova::PassPipelineManager pipelineManager(ctx);

  // Configure pipeline
  nova::PipelineConfig config;
  config.optLevel = optLevel;
  config.verificationLevel = enableVerifier ? 2 : 0;
  config.dumpIRAfterEachPass = printIR;

  // Build pipeline
  if (failed(pipelineManager.buildPipeline(pm, config))) {
    return mlir::failure();
  }

  // Run pipeline
  return pm.run(module);
}

mlir::LogicalResult runVerificationPipeline(mlir::ModuleOp module,
                                            bool printIR) {

  if (!module)
    return mlir::failure();

  mlir::MLIRContext *ctx = module.getContext();
  mlir::PassManager pm(ctx);

  if (printIR) {
    enableIRDumping(pm);
  }

  // Use Pass Pipeline Manager
  nova::PassPipelineManager pipelineManager(ctx);

  // Build verification-only pipeline
  nova::buildVerificationPipeline(pipelineManager, pm);

  return pm.run(module);
}

mlir::LogicalResult runMojoClassPipeline(mlir::ModuleOp module,
                                         bool enableVerifier, bool printIR) {

  if (!module)
    return mlir::failure();

  mlir::MLIRContext *ctx = module.getContext();
  mlir::PassManager pm(ctx);

  pm.enableVerifier(enableVerifier);
  if (printIR) {
    enableIRDumping(pm);
  }

  // Use Pass Pipeline Manager for Mojo-class performance
  nova::PassPipelineManager pipelineManager(ctx);

  // Build Mojo-class pipeline
  nova::buildMojoClassPipeline(pipelineManager, pm);

  return pm.run(module);
}

} // namespace mlir_utils
} // namespace nova
