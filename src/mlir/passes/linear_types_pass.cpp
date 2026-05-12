//===- linear_types_pass.cpp - Linear Type Checking Pass ----------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements the linear type checking pass for Nova.
// Ensures that linear values are consumed exactly once.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/analysis/linearity_analysis.h"
#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"
#include "compiler/mlir/dialect/nova_types.h"
#include "pass_registry.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Linear Resource Tracking
//===----------------------------------------------------------------------===//

struct ResourceState {
  bool allocated = false;
  bool consumed = false;
  Operation *allocSite = nullptr;
  Operation *consumeSite = nullptr;
  SmallVector<Operation *, 4> uses;
};

//===----------------------------------------------------------------------===//
// Linear Types Pass
//===----------------------------------------------------------------------===//

struct LinearTypesPass
    : public PassWrapper<LinearTypesPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LinearTypesPass)

  void runOnOperation() override {
    ModuleOp module = getOperation();
    bool hadError = false;

    // Walk all functions
    module.walk([&](FnOp func) { hadError |= checkFunction(func); });

    if (hadError)
      signalPassFailure();
  }

  bool checkFunction(FnOp func) {
    // Track all linear resources in this function
    llvm::DenseMap<Value, ResourceState> resources;
    bool hadError = false;

    // Collect all linear values
    func.walk([&](Operation *op) {
      // Check if operation produces a linear value
      for (auto result : op->getResults()) {
        if (isLinearType(result.getType())) {
          resources[result].allocated = true;
          resources[result].allocSite = op;
        }
      }
    });

    // Check linear.alloc operations
    func.walk([&](LinearAllocOp op) {
      Value resource = op.getResult();
      resources[resource].allocated = true;
      resources[resource].allocSite = op;
    });

    // Track linear.consume operations
    func.walk([&](LinearConsumeOp op) {
      Value resource = op.getResource();

      if (!resources.contains(resource)) {
        op.emitError("consuming unknown linear resource");
        hadError = true;
        return;
      }

      auto &state = resources[resource];

      // Check if already consumed
      if (state.consumed) {
        op.emitError("linear resource consumed more than once")
                .attachNote(state.consumeSite->getLoc())
            << "previously consumed here";
        hadError = true;
        return;
      }

      state.consumed = true;
      state.consumeSite = op;
    });

    // Check for unconsumed linear resources
    for (auto &[value, state] : resources) {
      if (state.allocated && !state.consumed) {
        state.allocSite->emitError("linear resource not consumed")
            << " - resource must be consumed exactly once";
        hadError = true;
      }
    }

    // Check linear values passed as arguments
    hadError |= checkLinearArguments(func, resources);

    // Check linear values in control flow
    hadError |= checkLinearControlFlow(func, resources);

    return hadError;
  }

  bool checkLinearArguments(FnOp func,
                            llvm::DenseMap<Value, ResourceState> &resources) {
    bool hadError = false;

    // Check function parameters
    for (auto arg : func.getArguments()) {
      if (isLinearType(arg.getType())) {
        // Linear argument must be consumed in function body
        auto &state = resources[arg];
        state.allocated = true;
        state.allocSite = func;

        if (!state.consumed) {
          func.emitError("linear parameter '")
              << arg << "' must be consumed in function body";
          hadError = true;
        }
      }
    }

    // Check return values
    if (auto returnOp = getReturnOp(func)) {
      for (auto operand : returnOp.getOperands()) {
        if (isLinearType(operand.getType())) {
          // Returning a linear value is valid (transfers ownership)
          if (resources.contains(operand)) {
            resources[operand].consumed = true;
            resources[operand].consumeSite = returnOp;
          }
        }
      }
    }

    return hadError;
  }

  bool checkLinearControlFlow(FnOp func,
                              llvm::DenseMap<Value, ResourceState> &resources) {
    bool hadError = false;

    // Check that linear resources are consumed on all paths
    func.walk([&](Operation *op) {
      // Check if/else branches
      if (op->getNumRegions() > 1) {
        for (auto &[value, state] : resources) {
          if (!state.consumed && state.allocSite->isBeforeInBlock(op)) {
            // Linear value must be consumed before branching
            // or in all branches
            hadError |= checkLinearInBranches(op, value, state);
          }
        }
      }
    });

    return hadError;
  }

  bool checkLinearInBranches(Operation *branchOp, Value linearValue,
                             ResourceState &state) {
    // Simplified check: linear values should be consumed before branching
    // Full implementation would track consumption in all branches
    if (!state.consumed) {
      branchOp->emitWarning(
          "linear resource may not be consumed on all control flow paths");
      return true;
    }
    return false;
  }

  // Helper: Check if type is linear
  bool isLinearType(Type type) { return type.isa<LinearType>(); }

  // Helper: Get return operation
  ReturnOp getReturnOp(FnOp func) {
    ReturnOp returnOp = nullptr;
    func.walk([&](ReturnOp op) { returnOp = op; });
    return returnOp;
  }

  StringRef getArgument() const final { return "nova-linear-types"; }

  StringRef getDescription() const final {
    return "Check linear type usage (exactly-once consumption)";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createLinearTypesPass() {
  return std::make_unique<LinearTypesPass>();
}

} // namespace nova
} // namespace mlir
