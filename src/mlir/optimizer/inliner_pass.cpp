//===- inliner_pass.cpp - Function Inlining Pass ------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements function inlining for Nova.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"

#include "mlir/IR/BlockAndValueMapping.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/InliningUtils.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Nova Inliner Interface
//===----------------------------------------------------------------------===//

struct NovaInlinerInterface : public DialectInlinerInterface {
  using DialectInlinerInterface::DialectInlinerInterface;

  // Returns true if the given operation can be inlined
  bool isLegalToInline(Operation *call, Operation *callable,
                       bool wouldBeCloned) const final {
    // Only inline pure functions
    if (auto effectAttr = callable->getAttrOfType<EffectSetAttr>("effects")) {
      return effectAttr.isPure();
    }
    return true;
  }

  // Returns true if the given region can be inlined
  bool isLegalToInline(Region *dest, Region *src, bool wouldBeCloned,
                       IRMapping &valueMapping) const final {
    return true;
  }

  // Handle the terminator of an inlined region
  void handleTerminator(Operation *op, Block *newDest) const final {
    // z.return becomes branch to continuation
    auto returnOp = dyn_cast<ReturnOp>(op);
    if (!returnOp)
      return;

    OpBuilder builder(op);
    builder.create<func::ReturnOp>(op->getLoc(), returnOp.getOperands());
    op->erase();
  }
};

//===----------------------------------------------------------------------===//
// Inliner Pass
//===----------------------------------------------------------------------===//

struct InlinerPass : public PassWrapper<InlinerPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(InlinerPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    ModuleOp module = getOperation();

    // Register Nova inliner interface
    ctx->loadDialect<NovaDialect>();

    // Collect call sites
    SmallVector<CallOp, 16> callSites;
    module.walk([&](CallOp call) { callSites.push_back(call); });

    // Try to inline each call
    for (CallOp call : callSites) {
      tryInlineCall(call, module);
    }
  }

  void tryInlineCall(CallOp call, ModuleOp module) {
    // Look up callee
    auto calleeName = call.getCallee();
    auto callee = module.lookupSymbol<FnOp>(calleeName);
    if (!callee)
      return;

    // Check if should inline
    if (!shouldInline(call, callee))
      return;

    // Perform inlining
    // (Simplified - real impl would use InlinerInterface)
    // For now, just mark as candidate
  }

  bool shouldInline(CallOp call, FnOp callee) {
    // Check size heuristic
    unsigned opCount = 0;
    callee.walk([&](Operation *) { opCount++; });

    if (opCount > 50) // Don't inline large functions
      return false;

    // Always inline pure functions
    if (auto effectAttr = callee->getAttrOfType<EffectSetAttr>("effects")) {
      if (effectAttr.isPure())
        return true;
    }

    return opCount < 10; // Inline small functions
  }

  StringRef getArgument() const final { return "nova-inline"; }

  StringRef getDescription() const final { return "Inline Nova functions"; }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createInlinerPass() {
  return std::make_unique<InlinerPass>();
}

} // namespace nova
} // namespace mlir
