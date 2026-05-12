//===- borrow_analysis.h - Borrow Analysis Infrastructure ------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_ANALYSIS_BORROW_ANALYSIS_H
#define NOVA_MLIR_ANALYSIS_BORROW_ANALYSIS_H

#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Borrow State
//===----------------------------------------------------------------------===//

enum class BorrowState {
  Owned,          // Value is owned
  BorrowedShared, // Immutably borrowed (can have multiple)
  BorrowedMut,    // Mutably borrowed (exclusive)
  Moved,          // Ownership transferred
  Dropped         // Value dropped
};

//===----------------------------------------------------------------------===//
// Value Borrow Info
//===----------------------------------------------------------------------===//

struct ValueBorrowInfo {
  BorrowState state = BorrowState::Owned;
  unsigned sharedBorrowCount = 0;
  Operation *lastUse = nullptr;
  Operation *moveSite = nullptr;
  SmallVector<Value, 4> activeBorrows;
};

//===----------------------------------------------------------------------===//
// Borrow Analysis
//===----------------------------------------------------------------------===//

class BorrowAnalysis {
public:
  explicit BorrowAnalysis(Operation *root);

  // Query borrow state
  BorrowState getState(Value value) const;
  bool isOwned(Value value) const;
  bool isBorrowed(Value value) const;
  bool isMoved(Value value) const;

  // Check borrow rules
  bool canBorrowShared(Value value) const;
  bool canBorrowMut(Value value) const;
  bool canMove(Value value) const;
  bool canDrop(Value value) const;

  // Get diagnostics info
  Operation *getMoveSite(Value value) const;
  unsigned getSharedBorrowCount(Value value) const;

private:
  llvm::DenseMap<Value, ValueBorrowInfo> borrowInfo;

  void analyze(Operation *op);
  void updateBorrowState(Value value, BorrowState newState);
};

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_ANALYSIS_BORROW_ANALYSIS_H
