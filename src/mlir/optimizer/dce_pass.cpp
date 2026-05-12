//===- dce_pass.cpp - Dead Code Elimination Pass ------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements dead code elimination for Nova.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Dead Code Elimination Pass
//===----------------------------------------------------------------------===//

struct DCEPass : public PassWrapper<DCEPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(DCEPass)

  void runOnOperation() override {
    ModuleOp module = getOperation();

    // Track live operations
    llvm::SmallPtrSet<Operation *, 64> liveOps;

    // Mark phase: Find all live operations
    module.walk([&](Operation *op) {
      if (isLive(op)) {
        markLive(op, liveOps);
      }
    });

    // Sweep phase: Remove dead operations
    SmallVector<Operation *, 16> deadOps;
    module.walk([&](Operation *op) {
      if (!liveOps.contains(op) && !op->hasTrait<OpTrait::IsTerminator>()) {
        deadOps.push_back(op);
      }
    });

    // Erase dead ops
    for (Operation *op : llvm::reverse(deadOps)) {
      op->erase();
    }
  }

  // Check if operation is inherently live
  bool isLive(Operation *op) {
    // Terminator ops are live
    if (op->hasTrait<OpTrait::IsTerminator>())
      return true;

    // Operations with side effects are live
    if (hasEffects(op))
      return true;

    // Return operations are live
    if (isa<ReturnOp>(op))
      return true;

    // Drop operations are live (destructors)
    if (isa<DropOp>(op))
      return true;

    // Free operations are live
    if (isa<FreeOp>(op))
      return true;

    // Linear consume operations are live
    if (isa<LinearConsumeOp>(op))
      return true;

    // Operations with uses are live
    for (auto result : op->getResults()) {
      if (!result.use_empty())
        return true;
    }

    return false;
  }

  // Check if operation has side effects
  bool hasEffects(Operation *op) {
    // Check for effect attribute
    if (auto effectAttr = op->getAttrOfType<EffectSetAttr>("effects")) {
      return !effectAttr.isPure();
    }

    // Operations that modify state
    if (isa<StoreOp, AllocOp, FreeOp, CallOp, SpawnOp>(op))
      return true;

    return false;
  }

  // Mark operation and its operands as live
  void markLive(Operation *op, llvm::SmallPtrSet<Operation *, 64> &liveOps) {
    if (!liveOps.insert(op).second)
      return; // Already marked

    // Mark all operands as live
    for (Value operand : op->getOperands()) {
      if (Operation *defOp = operand.getDefiningOp()) {
        markLive(defOp, liveOps);
      }
    }

    // Mark parent operation as live
    if (Operation *parent = op->getParentOp()) {
      liveOps.insert(parent);
    }
  }

  StringRef getArgument() const final { return "nova-dce"; }

  StringRef getDescription() const final { return "Dead code elimination"; }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createDCEPass() { return std::make_unique<DCEPass>(); }

} // namespace nova
} // namespace mlir
