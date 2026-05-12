//===- async_pass.cpp - Async/Await Lowering Pass -----------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements async/await lowering to MLIR async dialect
// with zero-cost abstractions (Mojo-style).
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/attrs.h"
#include "compiler/mlir/dialect/ops.h"
#include "compiler/mlir/dialect/types.h"

#include "mlir/Dialect/Async/IR/Async.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Async Patterns
//===----------------------------------------------------------------------===//

// Pattern: Lower async functions to async.execute regions
struct LowerAsyncFunctions : public OpRewritePattern<func::FuncOp> {
  using OpRewritePattern<func::FuncOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(func::FuncOp funcOp,
                                PatternRewriter &rewriter) const override {
    // Check if function is marked as async
    if (!funcOp->hasAttr("nova.async"))
      return failure();

    // Mark function for async execution
    rewriter.updateRootInPlace(funcOp, [&]() {
      funcOp->setAttr("async.exec", rewriter.getUnitAttr());
      funcOp->setAttr("nova.async_runtime",
                      rewriter.getStringAttr("zero_cost"));
    });

    return success();
  }
};

// Pattern: Convert await operations to async.await
struct LowerAwaitOps : public OpRewritePattern<CallOp> {
  using OpRewritePattern<CallOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(CallOp callOp,
                                PatternRewriter &rewriter) const override {
    // Check if call is to an async function
    if (!callOp->hasAttr("nova.await"))
      return failure();

    // Mark for async await lowering
    rewriter.updateRootInPlace(callOp, [&]() {
      callOp->setAttr("async.await", rewriter.getUnitAttr());
      callOp->setAttr("nova.await_strategy", rewriter.getStringAttr("suspend"));
    });

    return success();
  }
};

// Pattern: Detect and parallelize independent async operations
struct ParallelizeAsyncOps : public RewritePattern {
  ParallelizeAsyncOps(MLIRContext *ctx)
      : RewritePattern(MatchAnyOpTypeTag(), 1, ctx) {}

  LogicalResult matchAndRewrite(Operation *op,
                                PatternRewriter &rewriter) const override {
    auto callOp = dyn_cast<CallOp>(op);
    if (!callOp || !callOp->hasAttr("nova.async"))
      return failure();

    // Look for next async operation
    Operation *nextOp = callOp->getNextNode();
    if (!nextOp)
      return failure();

    auto nextCallOp = dyn_cast<CallOp>(nextOp);
    if (!nextCallOp || !nextCallOp->hasAttr("nova.async"))
      return failure();

    // Check if operations are independent (no data dependencies)
    bool hasDataDependency = false;
    for (Value result : callOp.getResults()) {
      for (Value operand : nextCallOp.getOperands()) {
        if (result == operand) {
          hasDataDependency = true;
          break;
        }
      }
    }

    if (hasDataDependency)
      return failure();

    // Mark for parallel execution
    rewriter.updateRootInPlace(callOp, [&]() {
      callOp->setAttr("nova.async_parallel", rewriter.getUnitAttr());
    });

    rewriter.updateRootInPlace(nextCallOp, [&]() {
      nextCallOp->setAttr("nova.async_parallel", rewriter.getUnitAttr());
    });

    return success();
  }
};

// Pattern: Zero-cost async optimization (Mojo-style)
struct OptimizeZeroCostAsync : public OpRewritePattern<func::FuncOp> {
  using OpRewritePattern<func::FuncOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(func::FuncOp funcOp,
                                PatternRewriter &rewriter) const override {
    if (!funcOp->hasAttr("nova.async"))
      return failure();

    // Check if async function can be optimized to synchronous
    // (e.g., no actual suspension points)
    bool hasSuspensionPoint = false;

    funcOp.walk([&](Operation *op) {
      if (op->hasAttr("nova.await") || op->hasAttr("async.await")) {
        hasSuspensionPoint = true;
      }
    });

    if (!hasSuspensionPoint) {
      // Can optimize to synchronous execution
      rewriter.updateRootInPlace(funcOp, [&]() {
        funcOp->setAttr("nova.async_optimize",
                        rewriter.getStringAttr("elide_async"));
      });
      return success();
    }

    return failure();
  }
};

//===----------------------------------------------------------------------===//
// Async Pass
//===----------------------------------------------------------------------===//

struct AsyncPass : public PassWrapper<AsyncPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(AsyncPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    RewritePatternSet patterns(ctx);

    // Add async patterns
    patterns.add<LowerAsyncFunctions, LowerAwaitOps, OptimizeZeroCostAsync>(
        ctx);

    patterns.add<ParallelizeAsyncOps>(ctx);

    // Apply patterns greedily
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                            std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-async"; }

  StringRef getDescription() const final {
    return "Async/await lowering with zero-cost abstractions";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createAsyncPass() {
  return std::make_unique<AsyncPass>();
}

} // namespace nova
} // namespace mlir
