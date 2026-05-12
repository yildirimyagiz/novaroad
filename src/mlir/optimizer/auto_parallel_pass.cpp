//===- auto_parallel_pass.cpp - Auto Parallelization Pass ---------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// Automatic parallelization of independent computations (Mojo-style).
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
#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Dependency Analysis
//===----------------------------------------------------------------------===//

struct DependencyInfo {
  llvm::DenseMap<Operation *, llvm::SmallVector<Operation *, 4>> dependencies;
  llvm::DenseMap<Operation *, llvm::SmallVector<Operation *, 4>> dependents;

  void addDependency(Operation *from, Operation *to) {
    dependencies[from].push_back(to);
    dependents[to].push_back(from);
  }

  bool hasDependency(Operation *from, Operation *to) const {
    auto it = dependencies.find(from);
    if (it == dependencies.end())
      return false;
    return llvm::is_contained(it->second, to);
  }

  bool areIndependent(Operation *op1, Operation *op2) const {
    return !hasDependency(op1, op2) && !hasDependency(op2, op1);
  }
};

//===----------------------------------------------------------------------===//
// Auto Parallelization Patterns
//===----------------------------------------------------------------------===//

// Pattern: Parallelize independent loop iterations
struct ParallelizeIndependentLoops : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // Check if already parallelized
    if (forOp->hasAttr("nova.parallel"))
      return failure();

    // Analyze loop for parallelizability
    Block *body = forOp.getBody();

    // Check for loop-carried dependencies
    bool hasLoopCarriedDep = false;

    // If loop has iteration arguments (beyond induction var), check
    // dependencies
    if (forOp.getNumRegionIterArgs() > 0) {
      // Conservatively assume dependency if there are iter args
      // unless it's a known reduction pattern
      hasLoopCarriedDep = true;

      // Check for reduction pattern
      if (forOp.getNumRegionIterArgs() == 1) {
        // Look for reduction operation
        for (Operation &op : body->getOperations()) {
          if (isa<arith::AddFOp, arith::AddIOp, arith::MulFOp, arith::MulIOp>(
                  &op)) {
            // This might be a reduction - can be parallelized differently
            hasLoopCarriedDep = false;
            rewriter.updateRootInPlace(forOp, [&]() {
              forOp->setAttr("nova.parallel_reduction", rewriter.getUnitAttr());
            });
            return success();
          }
        }
      }
    }

    if (hasLoopCarriedDep)
      return failure();

    // Check iteration count - only parallelize if worth it
    auto ubConst = forOp.getUpperBound().getDefiningOp<arith::ConstantOp>();
    if (!ubConst)
      return failure();

    auto ubAttr = ubConst.getValue().dyn_cast<IntegerAttr>();
    if (!ubAttr || ubAttr.getInt() < 32)
      return failure(); // Too small to benefit from parallelization

    // Mark for parallel execution
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.parallel", rewriter.getUnitAttr());
      forOp->setAttr("nova.parallel_strategy",
                     rewriter.getStringAttr("static"));

      // Determine optimal thread count (Mojo uses runtime detection)
      int threadCount = 8; // Default for modern CPUs
      forOp->setAttr("nova.parallel_threads",
                     rewriter.getI32IntegerAttr(threadCount));
    });

    return success();
  }
};

// Pattern: Parallelize independent function calls
struct ParallelizeFunctionCalls : public RewritePattern {
  ParallelizeFunctionCalls(MLIRContext *ctx)
      : RewritePattern(MatchAnyOpTypeTag(), 1, ctx) {}

  LogicalResult matchAndRewrite(Operation *op,
                                PatternRewriter &rewriter) const override {
    // Look for sequences of independent function calls
    auto callOp = dyn_cast<CallOp>(op);
    if (!callOp)
      return failure();

    // Check if function is pure (no side effects)
    if (!callOp->hasAttr("effects"))
      return failure();

    auto effectAttr = callOp->getAttrOfType<EffectSetAttr>("effects");
    if (!effectAttr || !effectAttr.isPure())
      return failure();

    // Find next call
    Operation *nextOp = callOp->getNextNode();
    if (!nextOp)
      return failure();

    auto nextCall = dyn_cast<CallOp>(nextOp);
    if (!nextCall)
      return failure();

    // Check independence
    bool isIndependent = true;
    for (Value result : callOp.getResults()) {
      for (Value operand : nextCall.getOperands()) {
        if (result == operand) {
          isIndependent = false;
          break;
        }
      }
    }

    if (!isIndependent)
      return failure();

    // Mark for parallel execution
    rewriter.updateRootInPlace(callOp, [&]() {
      callOp->setAttr("nova.parallel_call", rewriter.getUnitAttr());
    });

    rewriter.updateRootInPlace(nextCall, [&]() {
      nextCall->setAttr("nova.parallel_call", rewriter.getUnitAttr());
    });

    return success();
  }
};

// Pattern: Nested loop parallelization
struct ParallelizeNestedLoops : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp outerLoop,
                                PatternRewriter &rewriter) const override {
    // Look for nested loops
    scf::ForOp innerLoop = nullptr;

    outerLoop.getBody()->walk([&](scf::ForOp loop) {
      if (!innerLoop && loop != outerLoop) {
        innerLoop = loop;
      }
    });

    if (!innerLoop)
      return failure();

    // Check if both loops are independent
    if (!outerLoop->hasAttr("nova.parallel") &&
        !innerLoop->hasAttr("nova.parallel"))
      return failure();

    // Decide which loop to parallelize (outer vs inner)
    // Heuristic: parallelize outer loop for better cache locality
    auto outerUB = outerLoop.getUpperBound().getDefiningOp<arith::ConstantOp>();
    auto innerUB = innerLoop.getUpperBound().getDefiningOp<arith::ConstantOp>();

    if (!outerUB || !innerUB)
      return failure();

    auto outerCount = outerUB.getValue().dyn_cast<IntegerAttr>();
    auto innerCount = innerUB.getValue().dyn_cast<IntegerAttr>();

    if (!outerCount || !innerCount)
      return failure();

    // Parallelize the loop with more iterations
    if (outerCount.getInt() > innerCount.getInt()) {
      rewriter.updateRootInPlace(outerLoop, [&]() {
        outerLoop->setAttr("nova.parallel_nested",
                           rewriter.getStringAttr("outer"));
      });
    } else {
      rewriter.updateRootInPlace(innerLoop, [&]() {
        innerLoop->setAttr("nova.parallel_nested",
                           rewriter.getStringAttr("inner"));
      });
    }

    return success();
  }
};

// Pattern: Work-stealing parallelization for irregular workloads
struct WorkStealingParallelization : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    if (!forOp->hasAttr("nova.parallel"))
      return failure();

    // Detect irregular workload (varying iteration time)
    // For now, use heuristic: if loop contains conditionals or calls
    bool hasIrregularWork = false;

    forOp.walk([&](Operation *op) {
      if (isa<scf::IfOp, CallOp>(op)) {
        hasIrregularWork = true;
      }
    });

    if (!hasIrregularWork)
      return failure();

    // Switch to work-stealing strategy
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.parallel_strategy",
                     rewriter.getStringAttr("work_stealing"));
    });

    return success();
  }
};

//===----------------------------------------------------------------------===//
// Auto Parallelization Pass
//===----------------------------------------------------------------------===//

struct AutoParallelPass
    : public PassWrapper<AutoParallelPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(AutoParallelPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    RewritePatternSet patterns(ctx);

    // Add parallelization patterns
    patterns.add<ParallelizeIndependentLoops, ParallelizeNestedLoops,
                 WorkStealingParallelization>(ctx);

    patterns.add<ParallelizeFunctionCalls>(ctx);

    // Apply patterns greedily
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                            std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-auto-parallel"; }

  StringRef getDescription() const final {
    return "Automatic parallelization of independent computations";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createAutoParallelPass() {
  return std::make_unique<AutoParallelPass>();
}

} // namespace nova
} // namespace mlir
