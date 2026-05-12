//===- pgo_pass.cpp - Profile-Guided Optimization Pass ------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// Profile-guided optimization using runtime profiling data.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/attrs.h"
#include "compiler/mlir/dialect/ops.h"
#include "compiler/mlir/dialect/types.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "llvm/ADT/DenseMap.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Profile Data Structures
//===----------------------------------------------------------------------===//

struct ProfileData {
  llvm::DenseMap<Operation *, uint64_t> executionCounts;
  llvm::DenseMap<Operation *, double> executionTimes;
  llvm::DenseMap<scf::IfOp, bool> branchProbabilities;

  uint64_t getCount(Operation *op) const {
    auto it = executionCounts.find(op);
    return it != executionCounts.end() ? it->second : 0;
  }

  double getTime(Operation *op) const {
    auto it = executionTimes.find(op);
    return it != executionTimes.end() ? it->second : 0.0;
  }

  bool isHotPath(Operation *op) const {
    return getCount(op) > 10000; // Threshold for "hot" code
  }
};

//===----------------------------------------------------------------------===//
// PGO Patterns
//===----------------------------------------------------------------------===//

// Pattern: Inline hot functions based on profile data
struct InlineHotFunctions : public OpRewritePattern<CallOp> {
  const ProfileData &profile;

  InlineHotFunctions(MLIRContext *ctx, const ProfileData &prof)
      : OpRewritePattern<CallOp>(ctx), profile(prof) {}

  LogicalResult matchAndRewrite(CallOp callOp,
                                PatternRewriter &rewriter) const override {
    // Check if call site is hot
    if (!profile.isHotPath(callOp))
      return failure();

    // Mark for aggressive inlining
    rewriter.updateRootInPlace(callOp, [&]() {
      callOp->setAttr("nova.inline", rewriter.getUnitAttr());
      callOp->setAttr("nova.inline_reason",
                      rewriter.getStringAttr("pgo_hot_path"));
      callOp->setAttr("nova.execution_count",
                      rewriter.getI64IntegerAttr(profile.getCount(callOp)));
    });

    return success();
  }
};

// Pattern: Optimize branch prediction based on profiles
struct OptimizeBranchPrediction : public OpRewritePattern<scf::IfOp> {
  const ProfileData &profile;

  OptimizeBranchPrediction(MLIRContext *ctx, const ProfileData &prof)
      : OpRewritePattern<scf::IfOp>(ctx), profile(prof) {}

  LogicalResult matchAndRewrite(scf::IfOp ifOp,
                                PatternRewriter &rewriter) const override {
    // Get branch profile data
    auto it = profile.branchProbabilities.find(ifOp);
    if (it == profile.branchProbabilities.end())
      return failure();

    bool thenBranchLikely = it->second;

    // Add branch prediction hints
    rewriter.updateRootInPlace(ifOp, [&]() {
      ifOp->setAttr("nova.branch_likely",
                    rewriter.getBoolAttr(thenBranchLikely));
      ifOp->setAttr("nova.pgo_optimized", rewriter.getUnitAttr());
    });

    return success();
  }
};

// Pattern: Specialize hot loops based on common values
struct SpecializeHotLoops : public OpRewritePattern<scf::ForOp> {
  const ProfileData &profile;

  SpecializeHotLoops(MLIRContext *ctx, const ProfileData &prof)
      : OpRewritePattern<scf::ForOp>(ctx), profile(prof) {}

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // Check if loop is hot
    if (!profile.isHotPath(forOp))
      return failure();

    // Mark for specialization
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.hot_loop", rewriter.getUnitAttr());
      forOp->setAttr("nova.specialize", rewriter.getUnitAttr());
      forOp->setAttr("nova.execution_count",
                     rewriter.getI64IntegerAttr(profile.getCount(forOp)));
    });

    return success();
  }
};

// Pattern: Code layout optimization based on hotness
struct OptimizeCodeLayout : public OpRewritePattern<func::FuncOp> {
  const ProfileData &profile;

  OptimizeCodeLayout(MLIRContext *ctx, const ProfileData &prof)
      : OpRewritePattern<func::FuncOp>(ctx), profile(prof) {}

  LogicalResult matchAndRewrite(func::FuncOp funcOp,
                                PatternRewriter &rewriter) const override {
    // Calculate function hotness
    uint64_t totalCount = 0;
    funcOp.walk([&](Operation *op) { totalCount += profile.getCount(op); });

    if (totalCount == 0)
      return failure();

    // Classify function as hot, warm, or cold
    std::string classification;
    if (totalCount > 100000) {
      classification = "hot";
    } else if (totalCount > 10000) {
      classification = "warm";
    } else {
      classification = "cold";
    }

    rewriter.updateRootInPlace(funcOp, [&]() {
      funcOp->setAttr("nova.hotness", rewriter.getStringAttr(classification));
      funcOp->setAttr("nova.total_execution_count",
                      rewriter.getI64IntegerAttr(totalCount));
    });

    return success();
  }
};

//===----------------------------------------------------------------------===//
// PGO Pass
//===----------------------------------------------------------------------===//

struct PGOPass : public PassWrapper<PGOPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(PGOPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();

    // In a real implementation, load profile data from file
    // For now, create mock profile data
    ProfileData profile;

    // Simulate some hot paths
    getOperation().walk([&](Operation *op) {
      // Mock: mark some operations as hot
      if (auto callOp = dyn_cast<CallOp>(op)) {
        profile.executionCounts[op] = 50000;
      }
      if (auto forOp = dyn_cast<scf::ForOp>(op)) {
        profile.executionCounts[op] = 100000;
      }
    });

    RewritePatternSet patterns(ctx);

    // Add PGO patterns with profile data
    patterns.add<InlineHotFunctions>(ctx, profile);
    patterns.add<OptimizeBranchPrediction>(ctx, profile);
    patterns.add<SpecializeHotLoops>(ctx, profile);
    patterns.add<OptimizeCodeLayout>(ctx, profile);

    // Apply patterns greedily
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                            std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-pgo"; }

  StringRef getDescription() const final {
    return "Profile-guided optimization";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createPGOPass() { return std::make_unique<PGOPass>(); }

} // namespace nova
} // namespace mlir
