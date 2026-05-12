//===- loop_optimization_pass.cpp - Loop Optimizations ------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// Loop fusion, unrolling, and other loop optimizations for Mojo performance.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/ops.h"
#include "compiler/mlir/dialect/types.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Loop Optimization Patterns
//===----------------------------------------------------------------------===//

// Pattern: Loop unrolling for small iteration counts
struct UnrollSmallLoops : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // Check if already unrolled
    if (forOp->hasAttr("nova.unrolled"))
      return failure();

    // Get loop bounds
    auto lbConst = forOp.getLowerBound().getDefiningOp<arith::ConstantOp>();
    auto ubConst = forOp.getUpperBound().getDefiningOp<arith::ConstantOp>();
    auto stepConst = forOp.getStep().getDefiningOp<arith::ConstantOp>();

    if (!lbConst || !ubConst || !stepConst)
      return failure();

    // Calculate iteration count
    auto lbAttr = lbConst.getValue().dyn_cast<IntegerAttr>();
    auto ubAttr = ubConst.getValue().dyn_cast<IntegerAttr>();
    auto stepAttr = stepConst.getValue().dyn_cast<IntegerAttr>();

    if (!lbAttr || !ubAttr || !stepAttr)
      return failure();

    int64_t lb = lbAttr.getInt();
    int64_t ub = ubAttr.getInt();
    int64_t step = stepAttr.getInt();

    if (step <= 0)
      return failure();

    int64_t iterCount = (ub - lb) / step;

    // Only unroll small loops (< 8 iterations for Mojo compatibility)
    if (iterCount > 8 || iterCount <= 0)
      return failure();

    // Mark for unrolling
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.unroll", rewriter.getUnitAttr());
      forOp->setAttr("nova.unroll_count",
                     rewriter.getI32IntegerAttr(iterCount));
    });

    return success();
  }
};

// Pattern: Loop fusion for adjacent loops
struct FuseAdjacentLoops : public RewritePattern {
  FuseAdjacentLoops(MLIRContext *ctx)
      : RewritePattern(scf::ForOp::getOperationName(), 1, ctx) {}

  LogicalResult matchAndRewrite(Operation *op,
                                PatternRewriter &rewriter) const override {
    auto forOp = cast<scf::ForOp>(op);

    // Look for next loop
    Operation *nextOp = forOp->getNextNode();
    if (!nextOp)
      return failure();

    auto nextForOp = dyn_cast<scf::ForOp>(nextOp);
    if (!nextForOp)
      return failure();

    // Check if loops have same iteration space
    if (forOp.getLowerBound() != nextForOp.getLowerBound() ||
        forOp.getUpperBound() != nextForOp.getUpperBound() ||
        forOp.getStep() != nextForOp.getStep())
      return failure();

    // Check for dependencies between loops
    bool hasInterdependency = false;
    for (Value result : forOp.getResults()) {
      nextForOp.walk([&](Operation *innerOp) {
        for (Value operand : innerOp->getOperands()) {
          if (operand == result) {
            hasInterdependency = true;
          }
        }
      });
    }

    if (hasInterdependency)
      return failure();

    // Mark for fusion
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.loop_fusion_candidate", rewriter.getUnitAttr());
    });

    rewriter.updateRootInPlace(nextForOp, [&]() {
      nextForOp->setAttr("nova.loop_fusion_candidate", rewriter.getUnitAttr());
    });

    return success();
  }
};

// Pattern: Loop invariant code motion
struct LoopInvariantCodeMotion : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    Block *body = forOp.getBody();
    Value inductionVar = body->getArgument(0);

    // Find operations that don't depend on induction variable
    llvm::SmallVector<Operation *, 4> invariantOps;

    for (Operation &op : body->getOperations()) {
      bool usesInductionVar = false;

      for (Value operand : op.getOperands()) {
        if (operand == inductionVar) {
          usesInductionVar = true;
          break;
        }
      }

      if (!usesInductionVar && !isa<scf::YieldOp>(&op)) {
        invariantOps.push_back(&op);
      }
    }

    if (invariantOps.empty())
      return failure();

    // Mark operations for hoisting
    for (Operation *op : invariantOps) {
      rewriter.updateRootInPlace(op, [&]() {
        op->setAttr("nova.hoist_invariant", rewriter.getUnitAttr());
      });
    }

    return success();
  }
};

// Pattern: Loop tiling for cache optimization
struct TileLoopsForCache : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // Check if loop is large enough to benefit from tiling
    auto ubConst = forOp.getUpperBound().getDefiningOp<arith::ConstantOp>();
    if (!ubConst)
      return failure();

    auto ubAttr = ubConst.getValue().dyn_cast<IntegerAttr>();
    if (!ubAttr)
      return failure();

    int64_t iterCount = ubAttr.getInt();

    // Only tile large loops (> 256 iterations)
    if (iterCount < 256)
      return failure();

    // Mark for tiling with cache-friendly tile size
    // Apple Silicon has 128KB L1 cache, use tiles that fit
    int tileSize = 64; // Conservative tile size

    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.tile", rewriter.getUnitAttr());
      forOp->setAttr("nova.tile_size", rewriter.getI32IntegerAttr(tileSize));
    });

    return success();
  }
};

// Pattern: Strip mining for better cache utilization
struct StripMineLoops : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // Check if loop accesses arrays
    bool hasArrayAccess = false;

    forOp.walk([&](Operation *op) {
      // Check for memory operations
      if (op->hasTrait<OpTrait::HasRecursiveMemoryEffects>()) {
        hasArrayAccess = true;
      }
    });

    if (!hasArrayAccess)
      return failure();

    // Mark for strip mining
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.strip_mine", rewriter.getUnitAttr());
      forOp->setAttr("nova.strip_factor", rewriter.getI32IntegerAttr(16));
    });

    return success();
  }
};

//===----------------------------------------------------------------------===//
// Loop Optimization Pass
//===----------------------------------------------------------------------===//

struct LoopOptimizationPass
    : public PassWrapper<LoopOptimizationPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LoopOptimizationPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    RewritePatternSet patterns(ctx);

    // Add loop optimization patterns
    patterns.add<UnrollSmallLoops, LoopInvariantCodeMotion, TileLoopsForCache,
                 StripMineLoops>(ctx);

    patterns.add<FuseAdjacentLoops>(ctx);

    // Apply patterns greedily
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                            std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-loop-opt"; }

  StringRef getDescription() const final {
    return "Loop optimizations (fusion, unrolling, tiling)";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createLoopOptimizationPass() {
  return std::make_unique<LoopOptimizationPass>();
}

} // namespace nova
} // namespace mlir
