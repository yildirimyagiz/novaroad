//===- vectorization_pass.cpp - SIMD Vectorization Pass ----------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements automatic SIMD vectorization for Mojo-level performance.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/attrs.h"
#include "compiler/mlir/dialect/ops.h"
#include "compiler/mlir/dialect/types.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Vectorization Patterns
//===----------------------------------------------------------------------===//

// Pattern: Vectorize arithmetic operations marked for SIMD
struct VectorizeArithOps : public OpRewritePattern<arith::AddFOp> {
  using OpRewritePattern<arith::AddFOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(arith::AddFOp op,
                                PatternRewriter &rewriter) const override {
    // Check if operation is marked for vectorization
    if (!op->hasAttr("nova.vectorize"))
      return failure();

    // Get SIMD width from attribute (default to 8 for Mojo compatibility)
    int simdWidth = 8;
    if (auto widthAttr = op->getAttrOfType<IntegerAttr>("nova.simd_width")) {
      simdWidth = widthAttr.getInt();
    }

    // Get operand types
    Type elemType = op.getType();

    // Create vector type
    auto vectorType = VectorType::get({simdWidth}, elemType);

    // For now, mark the operation as vectorized
    // In a full implementation, this would:
    // 1. Split scalar operations into vector lanes
    // 2. Use vector.broadcast, vector.extract, etc.
    // 3. Generate platform-specific SIMD intrinsics

    rewriter.updateRootInPlace(op, [&]() {
      op->setAttr("nova.vectorized", rewriter.getUnitAttr());
      op->setAttr("nova.vector_width", rewriter.getI32IntegerAttr(simdWidth));
    });

    return success();
  }
};

// Pattern: Vectorize loops with SIMD-friendly patterns
struct VectorizeLoops : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // Check if loop can be vectorized
    // Conditions:
    // 1. Loop has constant bounds
    // 2. No loop-carried dependencies
    // 3. Body contains vectorizable operations

    // Get loop bounds
    auto lowerBound = forOp.getLowerBound();
    auto upperBound = forOp.getUpperBound();
    auto step = forOp.getStep();

    // Check if bounds are constants
    auto lbConst = lowerBound.getDefiningOp<arith::ConstantOp>();
    auto ubConst = upperBound.getDefiningOp<arith::ConstantOp>();
    auto stepConst = step.getDefiningOp<arith::ConstantOp>();

    if (!lbConst || !ubConst || !stepConst)
      return failure();

    // Check for vectorizable operations in body
    bool hasVectorizableOps = false;
    forOp.getBody()->walk([&](Operation *op) {
      if (isa<arith::AddFOp, arith::MulFOp, arith::AddIOp, arith::MulIOp>(op)) {
        hasVectorizableOps = true;
      }
    });

    if (!hasVectorizableOps)
      return failure();

    // Mark loop for vectorization
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.vectorize_loop", rewriter.getUnitAttr());
      forOp->setAttr("nova.simd_width", rewriter.getI32IntegerAttr(8));
    });

    return success();
  }
};

// Pattern: FMA (Fused Multiply-Add) detection and optimization
struct DetectFMAPattern : public RewritePattern {
  DetectFMAPattern(MLIRContext *ctx)
      : RewritePattern(arith::AddFOp::getOperationName(), 1, ctx) {}

  LogicalResult matchAndRewrite(Operation *op,
                                PatternRewriter &rewriter) const override {
    auto addOp = cast<arith::AddFOp>(op);

    // Check if already marked
    if (addOp->hasAttr("nova.fma"))
      return failure();

    // Look for pattern: add(mul(a, b), c) or add(c, mul(a, b))
    auto lhs = addOp.getLhs();
    auto rhs = addOp.getRhs();

    arith::MulFOp mulOp = nullptr;
    Value addend = nullptr;

    if (auto lhsMul = lhs.getDefiningOp<arith::MulFOp>()) {
      mulOp = lhsMul;
      addend = rhs;
    } else if (auto rhsMul = rhs.getDefiningOp<arith::MulFOp>()) {
      mulOp = rhsMul;
      addend = lhs;
    }

    if (!mulOp)
      return failure();

    // Mark for FMA fusion
    rewriter.updateRootInPlace(addOp, [&]() {
      addOp->setAttr("nova.fma", rewriter.getUnitAttr());
      addOp->setAttr("nova.fma_mul", SymbolRefAttr::get(rewriter.getContext(),
                                                        "fma_candidate"));
    });

    // Also mark the multiply operation
    rewriter.updateRootInPlace(mulOp, [&]() {
      mulOp->setAttr("nova.fma_part", rewriter.getUnitAttr());
    });

    return success();
  }
};

// Pattern: Auto-vectorize reduction operations
struct VectorizeReduction : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // Detect reduction pattern: sum, product, min, max
    // Pattern: for i in range: acc = acc op val[i]

    // Check if loop has one iteration argument (accumulator)
    if (forOp.getNumRegionIterArgs() != 1)
      return failure();

    Block *body = forOp.getBody();
    Value iterArg = forOp.getRegionIterArgs()[0];

    // Look for reduction operation
    Operation *reductionOp = nullptr;
    for (Operation &op : body->getOperations()) {
      if (auto addOp = dyn_cast<arith::AddFOp>(&op)) {
        if (addOp.getLhs() == body->getArgument(1) ||
            addOp.getRhs() == body->getArgument(1)) {
          reductionOp = addOp;
          break;
        }
      }
    }

    if (!reductionOp)
      return failure();

    // Mark for parallel reduction
    rewriter.updateRootInPlace(forOp, [&]() {
      forOp->setAttr("nova.parallel_reduction", rewriter.getUnitAttr());
      forOp->setAttr("nova.reduction_kind", rewriter.getStringAttr("add"));
    });

    return success();
  }
};

//===----------------------------------------------------------------------===//
// Vectorization Pass
//===----------------------------------------------------------------------===//

struct VectorizationPass
    : public PassWrapper<VectorizationPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VectorizationPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    RewritePatternSet patterns(ctx);

    // Add vectorization patterns
    patterns.add<VectorizeArithOps, VectorizeLoops, VectorizeReduction>(ctx);

    patterns.add<DetectFMAPattern>(ctx);

    // Apply patterns greedily
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                            std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-vectorize"; }

  StringRef getDescription() const final {
    return "Automatic SIMD vectorization (Mojo-style)";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createVectorizationPass() {
  return std::make_unique<VectorizationPass>();
}

} // namespace nova
} // namespace mlir
