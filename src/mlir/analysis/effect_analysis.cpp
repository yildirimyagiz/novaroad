//===- effect_analysis.cpp - Effect Analysis Implementation ---------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "effect_analysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "llvm/ADT/StringSwitch.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// EffectAnalysis Implementation
//===----------------------------------------------------------------------===//

EffectAnalysis::EffectAnalysis(Operation *root) { analyze(root); }

uint64_t EffectAnalysis::getEffects(Operation *op) const {
  auto it = opEffects.find(op);
  if (it != opEffects.end())
    return it->second;
  return EFFECT_UNKNOWN;
}

uint64_t EffectAnalysis::getFunctionEffects(StringRef funcName) const {
  auto it = functionEffects.find(funcName);
  if (it != functionEffects.end())
    return it->second;
  return EFFECT_UNKNOWN;
}

bool EffectAnalysis::isPure(Operation *op) const {
  return getEffects(op) == EFFECT_PURE;
}

bool EffectAnalysis::hasIO(Operation *op) const {
  return (getEffects(op) & EFFECT_IO) != 0;
}

bool EffectAnalysis::hasMemory(Operation *op) const {
  return (getEffects(op) & EFFECT_MEMORY) != 0;
}

bool EffectAnalysis::hasAsync(Operation *op) const {
  return (getEffects(op) & EFFECT_ASYNC) != 0;
}

uint64_t EffectAnalysis::combineEffects(uint64_t e1, uint64_t e2) const {
  // If either is unknown, result is unknown
  if ((e1 & EFFECT_UNKNOWN) || (e2 & EFFECT_UNKNOWN))
    return EFFECT_UNKNOWN;

  // Otherwise, union of effects
  return e1 | e2;
}

bool EffectAnalysis::isSubsetOf(uint64_t sub, uint64_t sup) const {
  // sub is subset of sup if all bits in sub are in sup
  return (sub & sup) == sub;
}

std::string EffectAnalysis::effectsToString(uint64_t effects) const {
  if (effects == EFFECT_PURE)
    return "pure";

  std::string result;
  bool first = true;

  auto addEffect = [&](const char *name) {
    if (!first)
      result += "|";
    result += name;
    first = false;
  };

  if (effects & EFFECT_IO)
    addEffect("io");
  if (effects & EFFECT_MEMORY)
    addEffect("memory");
  if (effects & EFFECT_THROW)
    addEffect("throw");
  if (effects & EFFECT_ASYNC)
    addEffect("async");
  if (effects & EFFECT_UNSAFE)
    addEffect("unsafe");
  if (effects & EFFECT_UNKNOWN)
    addEffect("unknown");

  return result.empty() ? "pure" : result;
}

void EffectAnalysis::analyze(Operation *op) {
  // Recursively analyze all operations
  op->walk([&](Operation *innerOp) {
    uint64_t effects = inferEffects(innerOp);
    opEffects[innerOp] = effects;

    // If it's a function, store function effects
    if (auto funcOp = dyn_cast<func::FuncOp>(innerOp)) {
      functionEffects[funcOp.getName()] = effects;
    }
  });
}

uint64_t EffectAnalysis::inferEffects(Operation *op) {
  // Check for effect attributes first
  if (auto effectsAttr = op->getAttrOfType<IntegerAttr>("effects")) {
    return effectsAttr.getInt();
  }

  // Infer based on operation name
  StringRef opName = op->getName().getStringRef();

  // Pure operations
  if (opName.contains("arith.") || opName.contains("math.")) {
    return EFFECT_PURE;
  }

  // Memory operations
  if (opName.contains("memref.") || opName.contains("alloc") ||
      opName.contains("store") || opName.contains("load")) {
    return EFFECT_MEMORY;
  }

  // I/O operations
  if (opName.contains("print") || opName.contains("read") ||
      opName.contains("write") || opName.contains("io.")) {
    return EFFECT_IO;
  }

  // Async operations
  if (opName.contains("async.") || opName.contains("await")) {
    return EFFECT_ASYNC;
  }

  // Function calls - combine callee effects
  if (auto callOp = dyn_cast<func::CallOp>(op)) {
    StringRef callee = callOp.getCallee();
    return getFunctionEffects(callee);
  }

  // Nova-specific operations
  if (opName.startswith("z.")) {
    // Check operation type
    if (opName.contains("alloc") || opName.contains("free"))
      return EFFECT_MEMORY;
    if (opName.contains("borrow") || opName.contains("move"))
      return EFFECT_PURE; // Ownership operations are pure
    if (opName.contains("async") || opName.contains("spawn"))
      return EFFECT_ASYNC;
    if (opName.contains("unsafe"))
      return EFFECT_UNSAFE;
  }

  // Default: assume operations with side effects unless proven otherwise
  if (op->hasTrait<OpTrait::IsTerminator>())
    return EFFECT_PURE;

  // Operations with no operands or results are likely pure
  if (op->getNumOperands() == 0 && op->getNumResults() == 0)
    return EFFECT_PURE;

  // Conservative default
  return EFFECT_MEMORY;
}

} // namespace nova
} // namespace mlir
