//===- linearity_analysis.h - Linear Type Analysis -------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_ANALYSIS_LINEARITY_ANALYSIS_H
#define NOVA_MLIR_ANALYSIS_LINEARITY_ANALYSIS_H

#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Linear Resource State
//===----------------------------------------------------------------------===//

struct LinearResourceInfo {
  bool allocated = false;
  bool consumed = false;
  Operation *allocSite = nullptr;
  Operation *consumeSite = nullptr;
  unsigned useCount = 0;
};

//===----------------------------------------------------------------------===//
// Linearity Analysis
//===----------------------------------------------------------------------===//

class LinearityAnalysis {
public:
  explicit LinearityAnalysis(Operation *root);

  // Query resource state
  bool isLinear(Value value) const;
  bool isAllocated(Value value) const;
  bool isConsumed(Value value) const;

  // Get resource info
  Operation *getAllocSite(Value value) const;
  Operation *getConsumeSite(Value value) const;
  unsigned getUseCount(Value value) const;

  // Validation
  bool hasLeaks() const;
  SmallVector<Value, 4> getLeakedResources() const;
  SmallVector<Value, 4> getDoubleConsumed() const;

private:
  llvm::DenseMap<Value, LinearResourceInfo> resources;

  void analyze(Operation *op);
  bool isLinearType(Type type) const;
};

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_ANALYSIS_LINEARITY_ANALYSIS_H
