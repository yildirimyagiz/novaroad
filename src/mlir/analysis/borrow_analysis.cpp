//===- borrow_analysis.cpp - Borrow Analysis Implementation -------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "borrow_analysis.h"

namespace mlir {
namespace nova {

BorrowAnalysis::BorrowAnalysis(Operation *root) { analyze(root); }

void BorrowAnalysis::analyze(Operation *op) {
  // Walk all values and track their borrow state
  op->walk([&](Operation *innerOp) {
    for (auto result : innerOp->getResults()) {
      // Initialize all values as owned
      borrowInfo[result].state = BorrowState::Owned;
    }
  });
}

BorrowState BorrowAnalysis::getState(Value value) const {
  auto it = borrowInfo.find(value);
  if (it != borrowInfo.end())
    return it->second.state;
  return BorrowState::Owned;
}

bool BorrowAnalysis::isOwned(Value value) const {
  return getState(value) == BorrowState::Owned;
}

bool BorrowAnalysis::isBorrowed(Value value) const {
  auto state = getState(value);
  return state == BorrowState::BorrowedShared ||
         state == BorrowState::BorrowedMut;
}

bool BorrowAnalysis::isMoved(Value value) const {
  return getState(value) == BorrowState::Moved;
}

bool BorrowAnalysis::canBorrowShared(Value value) const {
  auto state = getState(value);
  // Can borrow if owned or already shared borrowed
  return state == BorrowState::Owned || state == BorrowState::BorrowedShared;
}

bool BorrowAnalysis::canBorrowMut(Value value) const {
  // Can only mutably borrow if owned (no other borrows)
  return getState(value) == BorrowState::Owned;
}

bool BorrowAnalysis::canMove(Value value) const {
  auto state = getState(value);
  // Can only move if owned (not borrowed or already moved)
  return state == BorrowState::Owned;
}

bool BorrowAnalysis::canDrop(Value value) const {
  auto state = getState(value);
  // Can only drop if owned (not borrowed or already moved/dropped)
  return state == BorrowState::Owned;
}

Operation *BorrowAnalysis::getMoveSite(Value value) const {
  auto it = borrowInfo.find(value);
  if (it != borrowInfo.end())
    return it->second.moveSite;
  return nullptr;
}

unsigned BorrowAnalysis::getSharedBorrowCount(Value value) const {
  auto it = borrowInfo.find(value);
  if (it != borrowInfo.end())
    return it->second.sharedBorrowCount;
  return 0;
}

void BorrowAnalysis::updateBorrowState(Value value, BorrowState newState) {
  borrowInfo[value].state = newState;
}

} // namespace nova
} // namespace mlir
