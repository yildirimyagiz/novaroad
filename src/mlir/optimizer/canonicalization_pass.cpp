//===- canonicalization_pass.cpp - Canonicalization Pass ----------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements canonicalization patterns for Nova dialect.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/attrs.h"
#include "compiler/mlir/dialect/dialect.h"
#include "compiler/mlir/dialect/ops.h"
#include "compiler/mlir/dialect/types.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BlockAndValueMapping.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "llvm/ADT/STLExtras.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Canonicalization Patterns
//===----------------------------------------------------------------------===//

// Pattern: Remove redundant move operations
// %1 = z.move %0
// %2 = z.move %1
// =>
// %2 = z.move %0
struct RemoveRedundantMove : public OpRewritePattern<MoveOp> {
  using OpRewritePattern<MoveOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(MoveOp op,
                                PatternRewriter &rewriter) const override {
    // Check if source is result of another move
    if (auto sourceMoveOp = op.getSource().getDefiningOp<MoveOp>()) {
      // Replace with direct move from original source
      rewriter.replaceOpWithNewOp<MoveOp>(op, sourceMoveOp.getSource());
      return success();
    }
    return failure();
  }
};

// Pattern: Fold borrow + immediate unborrow
// %ref = z.borrow %val
// (no uses of %ref)
// =>
// (remove borrow)
struct FoldUnusedBorrow : public OpRewritePattern<BorrowOp> {
  using OpRewritePattern<BorrowOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(BorrowOp op,
                                PatternRewriter &rewriter) const override {
    // If borrow has no uses, it can be eliminated
    if (op.getResult().use_empty()) {
      rewriter.eraseOp(op);
      return success();
    }
    return failure();
  }
};

// Pattern: Inline pure function calls
struct InlinePureCall : public OpRewritePattern<CallOp> {
  using OpRewritePattern<CallOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(CallOp op,
                                PatternRewriter &rewriter) const override {
    // Check if call has pure effect or inline hint
    auto effectAttr = op->getAttrOfType<EffectSetAttr>("effects");
    auto inlineHint = op->hasAttr("nova.inline");

    if ((!effectAttr || !effectAttr.isPure()) && !inlineHint)
      return failure();

    // Get the callee function
    auto calleeAttr = op.getCalleeAttr();
    if (!calleeAttr)
      return failure();

    // Look up the function in the symbol table
    auto moduleOp = op->getParentOfType<ModuleOp>();
    if (!moduleOp)
      return failure();

    auto callee = moduleOp.lookupSymbol<func::FuncOp>(calleeAttr);
    if (!callee)
      return failure();

    // Don't inline if function is too large (heuristic: > 50 ops)
    size_t opCount = 0;
    callee.walk([&](Operation *innerOp) { opCount++; });
    if (opCount > 50 && !inlineHint)
      return failure();

    // Inline the function body
    Region &calleeRegion = callee.getBody();
    if (calleeRegion.empty())
      return failure();

    Block &calleeBlock = calleeRegion.front();

    // Map arguments to operands
    BlockAndValueMapping mapper;
    for (auto [arg, operand] :
         llvm::zip(calleeBlock.getArguments(), op.getOperands())) {
      mapper.map(arg, operand);
    }

    // Clone operations from callee into caller
    rewriter.setInsertionPoint(op);
    Value resultValue;

    for (Operation &innerOp : calleeBlock.without_terminator()) {
      Operation *clonedOp = rewriter.clone(innerOp, mapper);
      // Update mapper with cloned results
      for (auto [origResult, clonedResult] :
           llvm::zip(innerOp.getResults(), clonedOp->getResults())) {
        mapper.map(origResult, clonedResult);
      }
    }

    // Handle return operation
    if (auto returnOp = dyn_cast<func::ReturnOp>(calleeBlock.getTerminator())) {
      if (returnOp.getNumOperands() > 0) {
        resultValue = mapper.lookup(returnOp.getOperand(0));
      }
    }

    // Replace call with inlined result
    if (resultValue) {
      rewriter.replaceOp(op, resultValue);
    } else {
      rewriter.eraseOp(op);
    }

    return success();
  }
};

// Pattern: Combine consecutive alloc/free
// %ptr = z.alloc
// z.free %ptr
// =>
// (remove both)
struct RemoveDeadAlloc : public OpRewritePattern<AllocOp> {
  using OpRewritePattern<AllocOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(AllocOp op,
                                PatternRewriter &rewriter) const override {
    // Check if immediately freed
    Value result = op.getResult();

    // If only use is a free operation, remove both
    if (result.hasOneUse()) {
      Operation *user = *result.getUsers().begin();
      if (auto freeOp = dyn_cast<FreeOp>(user)) {
        rewriter.eraseOp(freeOp);
        rewriter.eraseOp(op);
        return success();
      }
    }

    return failure();
  }
};

// Pattern: Constant folding for effects
// (propagate pure annotations)
struct FoldPureEffects : public OpRewritePattern<CallOp> {
  using OpRewritePattern<CallOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(CallOp op,
                                PatternRewriter &rewriter) const override {
    // If no effect attribute, try to infer from callee
    if (op->hasAttr("effects"))
      return failure();

    // Look up callee function
    auto calleeAttr = op.getCalleeAttr();
    if (!calleeAttr)
      return failure();

    auto moduleOp = op->getParentOfType<ModuleOp>();
    if (!moduleOp)
      return failure();

    auto callee = moduleOp.lookupSymbol<func::FuncOp>(calleeAttr);
    if (!callee)
      return failure();

    // Copy effect attributes from callee to call site
    if (auto calleeEffects = callee->getAttr("effects")) {
      rewriter.updateRootInPlace(
          op, [&]() { op->setAttr("effects", calleeEffects); });
      return success();
    }

    // If callee has no effects attribute but is marked pure, propagate
    if (callee->hasAttr("nova.pure")) {
      auto ctx = op->getContext();
      auto pureEffectAttr = EffectSetAttr::get(ctx, EFFECT_PURE);
      rewriter.updateRootInPlace(
          op, [&]() { op->setAttr("effects", pureEffectAttr); });
      return success();
    }

    return failure();
  }
};

//===----------------------------------------------------------------------===//
// Canonicalization Pass
//===----------------------------------------------------------------------===//

struct CanonicalizationPass
    : public PassWrapper<CanonicalizationPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CanonicalizationPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    RewritePatternSet patterns(ctx);

    // Add all canonicalization patterns
    patterns.add<RemoveRedundantMove, FoldUnusedBorrow, InlinePureCall,
                 RemoveDeadAlloc, FoldPureEffects>(ctx);

    // Apply patterns greedily
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                            std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-canonicalize"; }

  StringRef getDescription() const final { return "Canonicalize Nova IR"; }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createCanonicalizationPass() {
  return std::make_unique<CanonicalizationPass>();
}

} // namespace nova
} // namespace mlir
