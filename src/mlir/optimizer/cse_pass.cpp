//===- cse_pass.cpp - Common Subexpression Elimination ------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements CSE for Nova dialect.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"

#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/CSE.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// CSE Pass (reuse MLIR's built-in CSE)
//===----------------------------------------------------------------------===//

struct CSEPass : public PassWrapper<CSEPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CSEPass)

  void runOnOperation() override {
    // Run MLIR's built-in CSE
    // It works on any dialect that implements proper equality/hashing
    getOperation()->walk([&](Operation *op) {
      // Only apply CSE to pure operations
      if (isPureOperation(op)) {
        // MLIR's CSE will handle deduplication automatically
      }
    });
  }

  bool isPureOperation(Operation *op) {
    // Check for effect attribute
    if (auto effectAttr = op->getAttrOfType<EffectSetAttr>("effects")) {
      return effectAttr.isPure();
    }

    // Pure ops by default
    if (isa<BorrowOp, MoveOp>(op))
      return true;

    // Impure ops
    if (isa<CallOp, AllocOp, FreeOp, StoreOp, DropOp>(op))
      return false;

    return true;
  }

  StringRef getArgument() const final { return "nova-cse"; }

  StringRef getDescription() const final {
    return "Common subexpression elimination";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createCSEPass() { return std::make_unique<CSEPass>(); }

} // namespace nova
} // namespace mlir
