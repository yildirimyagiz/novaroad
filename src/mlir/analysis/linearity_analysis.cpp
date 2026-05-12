//===- linearity_analysis.cpp - Linear Type Analysis ----------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "linearity_analysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/StringRef.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// LinearityAnalysis Implementation
//===----------------------------------------------------------------------===//

LinearityAnalysis::LinearityAnalysis(Operation *root) { analyze(root); }

bool LinearityAnalysis::isLinear(Value value) const {
  return isLinearType(value.getType());
}

bool LinearityAnalysis::isAllocated(Value value) const {
  auto it = resources.find(value);
  return it != resources.end() && it->second.allocated;
}

bool LinearityAnalysis::isConsumed(Value value) const {
  auto it = resources.find(value);
  return it != resources.end() && it->second.consumed;
}

Operation *LinearityAnalysis::getAllocSite(Value value) const {
  auto it = resources.find(value);
  if (it != resources.end())
    return it->second.allocSite;
  return nullptr;
}

Operation *LinearityAnalysis::getConsumeSite(Value value) const {
  auto it = resources.find(value);
  if (it != resources.end())
    return it->second.consumeSite;
  return nullptr;
}

unsigned LinearityAnalysis::getUseCount(Value value) const {
  auto it = resources.find(value);
  if (it != resources.end())
    return it->second.useCount;
  return 0;
}

bool LinearityAnalysis::hasLeaks() const {
  for (const auto &entry : resources) {
    const auto &info = entry.second;
    // A resource is leaked if it's allocated but not consumed
    if (info.allocated && !info.consumed)
      return true;
  }
  return false;
}

SmallVector<Value, 4> LinearityAnalysis::getLeakedResources() const {
  SmallVector<Value, 4> leaked;

  for (const auto &entry : resources) {
    const auto &info = entry.second;
    if (info.allocated && !info.consumed)
      leaked.push_back(entry.first);
  }

  return leaked;
}

SmallVector<Value, 4> LinearityAnalysis::getDoubleConsumed() const {
  SmallVector<Value, 4> doubleConsumed;

  for (const auto &entry : resources) {
    const auto &info = entry.second;
    // A resource is double-consumed if use count > 1
    if (info.useCount > 1)
      doubleConsumed.push_back(entry.first);
  }

  return doubleConsumed;
}

void LinearityAnalysis::analyze(Operation *op) {
  // Walk the operation tree
  op->walk([&](Operation *innerOp) {
    StringRef opName = innerOp->getName().getStringRef();

    // Check for linear resource allocation
    if (opName.contains("linear.alloc") || opName.contains("z.linear.create")) {
      for (Value result : innerOp->getResults()) {
        if (isLinear(result)) {
          auto &info = resources[result];
          info.allocated = true;
          info.allocSite = innerOp;
          info.useCount = 0;
        }
      }
    }

    // Check for linear resource consumption
    if (opName.contains("linear.consume") ||
        opName.contains("z.linear.consume")) {
      for (Value operand : innerOp->getOperands()) {
        if (isLinear(operand)) {
          auto &info = resources[operand];
          info.consumed = true;
          info.consumeSite = innerOp;
          info.useCount++;
        }
      }
    }

    // Track uses of linear resources
    for (Value operand : innerOp->getOperands()) {
      if (isLinear(operand)) {
        auto it = resources.find(operand);
        if (it != resources.end()) {
          // Only count non-consuming uses
          if (!opName.contains("linear.consume")) {
            it->second.useCount++;
          }
        }
      }
    }

    // Check for moves (transfers ownership without consuming)
    if (opName.contains("linear.move") || opName.contains("z.move")) {
      for (size_t i = 0; i < innerOp->getNumOperands(); ++i) {
        Value operand = innerOp->getOperand(i);
        if (isLinear(operand) && i < innerOp->getNumResults()) {
          Value result = innerOp->getResult(i);

          // Transfer resource info from operand to result
          auto it = resources.find(operand);
          if (it != resources.end()) {
            resources[result] = it->second;
            // Mark old value as consumed (moved from)
            it->second.consumed = true;
            it->second.consumeSite = innerOp;
          }
        }
      }
    }
  });
}

bool LinearityAnalysis::isLinearType(Type type) const {
  // Check if type is a linear type
  StringRef typeName = type.getAsOpaquePointer() ? "" : "";

  // Check for explicit linear type marker
  if (auto namedType = type.dyn_cast_or_null<Type>()) {
    // For MLIR types, we need to check the dialect
    // Nova linear types have the form !z.lin<T>
    std::string typeStr;
    llvm::raw_string_ostream os(typeStr);
    type.print(os);
    os.flush();

    if (typeStr.find("z.lin<") != std::string::npos)
      return true;

    // Also check for types marked with linear attribute
    // This requires access to the defining operation
  }

  // Conservative: assume non-linear unless proven otherwise
  return false;
}

} // namespace nova
} // namespace mlir
