//===- lifetime_analysis.h - Lifetime Analysis -----------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_ANALYSIS_LIFETIME_ANALYSIS_H
#define NOVA_MLIR_ANALYSIS_LIFETIME_ANALYSIS_H

#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Lifetime Region
//===----------------------------------------------------------------------===//

struct LifetimeRegion {
  StringRef name;
  unsigned startPoint = 0;
  unsigned endPoint = 0;
  SmallVector<Value, 4> values;
};

//===----------------------------------------------------------------------===//
// Lifetime Constraint
//===----------------------------------------------------------------------===//

struct LifetimeConstraint {
  StringRef sub; // Must not outlive
  StringRef sup; // Must outlive this
  Operation *origin = nullptr;
};

//===----------------------------------------------------------------------===//
// Lifetime Analysis
//===----------------------------------------------------------------------===//

class LifetimeAnalysis {
public:
  explicit LifetimeAnalysis(Operation *root);

  // Query lifetimes
  const LifetimeRegion *getRegion(StringRef lifetime) const;
  bool outlives(StringRef lifetime1, StringRef lifetime2) const;

  // Program points
  unsigned getProgramPoint(Operation *op) const;

  // Validation
  bool checkConstraints() const;
  SmallVector<LifetimeConstraint, 8> getViolations() const;

private:
  llvm::StringMap<LifetimeRegion> regions;
  llvm::DenseMap<Operation *, unsigned> programPoints;
  SmallVector<LifetimeConstraint, 16> constraints;

  void analyze(Operation *op);
  void assignProgramPoints(Operation *op);
  void inferRegions(Operation *op);
  void solveConstraints();
};

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_ANALYSIS_LIFETIME_ANALYSIS_H
