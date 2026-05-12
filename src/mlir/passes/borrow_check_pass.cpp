//===- borrow_check_pass.cpp - Borrow Checking Pass --------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements the borrow checking pass for Nova.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/analysis/borrow_analysis.h"
#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"
#include "pass_registry.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Borrow State Tracking
//===----------------------------------------------------------------------===//

enum class BorrowState { Owned, BorrowedShared, BorrowedMut, Moved, Dropped };

struct ValueState {
  BorrowState state = BorrowState::Owned;
  unsigned borrowCount = 0;
  Operation *lastUse = nullptr;
};

//===----------------------------------------------------------------------===//
// Borrow Check Pass
//===----------------------------------------------------------------------===//

struct BorrowCheckPass
    : public PassWrapper<BorrowCheckPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(BorrowCheckPass)

  void runOnOperation() override {
    ModuleOp module = getOperation();
    bool hadError = false;

    // Walk all functions
    module.walk([&](FnOp func) { hadError |= checkFunction(func); });

    if (hadError)
      signalPassFailure();
  }

  bool checkFunction(FnOp func) {
    // Track state of each SSA value
    llvm::DenseMap<Value, ValueState> states;
    bool hadError = false;

    // Initialize function parameters as Owned
    for (auto arg : func.getArguments()) {
      states[arg].state = BorrowState::Owned;
    }

    // Walk operations in the function
    func.walk([&](Operation *op) {
      // Check borrow operations
      if (auto borrowOp = dyn_cast<BorrowOp>(op)) {
        hadError |= checkBorrow(borrowOp, states);
      }

      // Check move operations
      else if (auto moveOp = dyn_cast<MoveOp>(op)) {
        hadError |= checkMove(moveOp, states);
      }

      // Check drop operations
      else if (auto dropOp = dyn_cast<DropOp>(op)) {
        hadError |= checkDrop(dropOp, states);
      }

      // Check load operations (requires valid borrow)
      else if (auto loadOp = dyn_cast<LoadOp>(op)) {
        hadError |= checkLoad(loadOp, states);
      }

      // Check store operations (requires mutable borrow)
      else if (auto storeOp = dyn_cast<StoreOp>(op)) {
        hadError |= checkStore(storeOp, states);
      }
    });

    return hadError;
  }

  bool checkBorrow(BorrowOp op, llvm::DenseMap<Value, ValueState> &states) {
    Value source = op.getValue();
    auto &state = states[source];

    // Cannot borrow moved or dropped value
    if (state.state == BorrowState::Moved) {
      op.emitError("cannot borrow moved value");
      return true;
    }
    if (state.state == BorrowState::Dropped) {
      op.emitError("cannot borrow dropped value");
      return true;
    }

    // Check mutable borrow aliasing
    if (op.getIsMutable()) {
      if (state.state == BorrowState::BorrowedShared) {
        op.emitError("cannot create mutable borrow while shared borrows exist");
        return true;
      }
      if (state.state == BorrowState::BorrowedMut) {
        op.emitError(
            "cannot create mutable borrow while another mutable borrow exists");
        return true;
      }
      state.state = BorrowState::BorrowedMut;
    } else {
      if (state.state == BorrowState::BorrowedMut) {
        op.emitError("cannot create shared borrow while mutable borrow exists");
        return true;
      }
      state.state = BorrowState::BorrowedShared;
      state.borrowCount++;
    }

    return false;
  }

  bool checkMove(MoveOp op, llvm::DenseMap<Value, ValueState> &states) {
    Value source = op.getSource();
    auto &state = states[source];

    // Cannot move borrowed value
    if (state.state == BorrowState::BorrowedShared ||
        state.state == BorrowState::BorrowedMut) {
      op.emitError("cannot move borrowed value");
      return true;
    }

    // Cannot move already moved value
    if (state.state == BorrowState::Moved) {
      op.emitError("use of moved value");
      return true;
    }

    // Mark as moved
    state.state = BorrowState::Moved;
    state.lastUse = op;

    // Result is owned
    states[op.getResult()].state = BorrowState::Owned;

    return false;
  }

  bool checkDrop(DropOp op, llvm::DenseMap<Value, ValueState> &states) {
    Value value = op.getValue();
    auto &state = states[value];

    // Cannot drop borrowed value
    if (state.state == BorrowState::BorrowedShared ||
        state.state == BorrowState::BorrowedMut) {
      op.emitError("cannot drop borrowed value");
      return true;
    }

    // Cannot drop already moved/dropped value
    if (state.state == BorrowState::Moved) {
      op.emitError("cannot drop moved value");
      return true;
    }
    if (state.state == BorrowState::Dropped) {
      op.emitError("cannot drop value twice");
      return true;
    }

    state.state = BorrowState::Dropped;
    return false;
  }

  bool checkLoad(LoadOp op, llvm::DenseMap<Value, ValueState> &states) {
    // Load requires a valid reference - checked by type system
    return false;
  }

  bool checkStore(StoreOp op, llvm::DenseMap<Value, ValueState> &states) {
    // Store requires mutable reference - checked by type system
    return false;
  }

  StringRef getArgument() const final { return "nova-borrow-check"; }

  StringRef getDescription() const final {
    return "Check ownership and borrowing rules";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createBorrowCheckPass() {
  return std::make_unique<BorrowCheckPass>();
}

} // namespace nova
} // namespace mlir
