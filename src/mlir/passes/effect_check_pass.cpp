//===- effect_check_pass.cpp - Effect Checking Pass ---------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements effect inference and checking for Nova.
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/analysis/effect_analysis.h"
#include "compiler/mlir/dialect/nova_attrs.h"
#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"
#include "pass_registry.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Effect Inference
//===----------------------------------------------------------------------===//

class EffectInference {
public:
  // Infer effects for an operation
  uint64_t inferEffects(Operation *op) {
    // Check if operation has explicit effect annotation
    if (auto effectAttr = op->getAttrOfType<EffectSetAttr>("effects")) {
      return effectAttr.getEffectBits();
    }

    // Infer from operation type
    return llvm::TypeSwitch<Operation *, uint64_t>(op)
        .Case<CallOp>([&](CallOp callOp) { return inferCallEffects(callOp); })
        .Case<LoadOp>([](LoadOp) { return EFFECT_MEMORY; })
        .Case<StoreOp>([](StoreOp) { return EFFECT_MEMORY; })
        .Case<AllocOp>([](AllocOp) { return EFFECT_MEMORY; })
        .Case<FreeOp>([](FreeOp) { return EFFECT_MEMORY; })
        .Case<SpawnOp>([](SpawnOp) { return EFFECT_ASYNC; })
        .Case<AwaitOp>([](AwaitOp) { return EFFECT_ASYNC; })
        .Case<BorrowOp, MoveOp, DropOp>([](Operation *) { return EFFECT_PURE; })
        .Default([](Operation *) { return EFFECT_PURE; });
  }

  // Infer effects for a function call
  uint64_t inferCallEffects(CallOp callOp) {
    // Check if callee has effect annotation
    if (auto effectAttr = callOp->getAttrOfType<EffectSetAttr>("effects")) {
      return effectAttr.getEffectBits();
    }

    // Look up callee function
    auto calleeName = callOp.getCallee();
    if (functionEffects.contains(calleeName)) {
      return functionEffects[calleeName];
    }

    // Unknown - assume unknown effects
    return EFFECT_UNKNOWN;
  }

  // Store inferred effects for functions
  llvm::DenseMap<StringRef, uint64_t> functionEffects;
};

//===----------------------------------------------------------------------===//
// Effect Check Pass
//===----------------------------------------------------------------------===//

struct EffectCheckPass
    : public PassWrapper<EffectCheckPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(EffectCheckPass)

  void runOnOperation() override {
    ModuleOp module = getOperation();
    EffectInference inference;
    bool hadError = false;

    // First pass: infer effects for all functions (bottom-up)
    SmallVector<FnOp, 16> functions;
    module.walk([&](FnOp func) { functions.push_back(func); });

    // Process in reverse order (callees before callers)
    // Simplified: just process all functions
    for (auto func : functions) {
      uint64_t inferredEffects = inferFunctionEffects(func, inference);

      // Store inferred effects
      inference.functionEffects[func.getSymName()] = inferredEffects;

      // Annotate function with inferred effects
      if (!func->hasAttr("effects")) {
        auto ctx = func.getContext();
        func->setAttr("effects", EffectSetAttr::get(ctx, inferredEffects));
      }
    }

    // Second pass: check effect boundaries
    for (auto func : functions) {
      hadError |= checkEffectBoundaries(func, inference);
    }

    if (hadError)
      signalPassFailure();
  }

  // Infer effects for a function by examining its body
  uint64_t inferFunctionEffects(FnOp func, EffectInference &inference) {
    uint64_t combinedEffects = EFFECT_PURE;

    func.walk([&](Operation *op) {
      // Skip the function itself
      if (op == func.getOperation())
        return;

      // Combine effects from all operations
      uint64_t opEffects = inference.inferEffects(op);
      combinedEffects |= opEffects;
    });

    return combinedEffects;
  }

  // Check that effect annotations match actual effects
  bool checkEffectBoundaries(FnOp func, EffectInference &inference) {
    bool hadError = false;

    // Get declared effects
    uint64_t declaredEffects = EFFECT_PURE;
    if (auto effectAttr = func->getAttrOfType<EffectSetAttr>("effects")) {
      declaredEffects = effectAttr.getEffectBits();
    }

    // Get actual effects
    uint64_t actualEffects = inferFunctionEffects(func, inference);

    // Check if actual effects are subset of declared effects
    if ((actualEffects & ~declaredEffects) != 0) {
      func.emitError("function has undeclared effects")
          << "\n  declared: " << effectsToString(declaredEffects)
          << "\n  actual:   " << effectsToString(actualEffects);
      hadError = true;
    }

    // Check calls respect effect boundaries
    func.walk([&](CallOp callOp) {
      hadError |= checkCallEffects(callOp, declaredEffects, inference);
    });

    return hadError;
  }

  // Check that calls respect caller's effect boundaries
  bool checkCallEffects(CallOp callOp, uint64_t callerEffects,
                        EffectInference &inference) {
    uint64_t calleeEffects = inference.inferCallEffects(callOp);

    // Callee effects must be subset of caller effects
    if ((calleeEffects & ~callerEffects) != 0) {
      callOp.emitError("call has effects not allowed in caller")
          << "\n  caller allows: " << effectsToString(callerEffects)
          << "\n  callee has:    " << effectsToString(calleeEffects);
      return true;
    }

    return false;
  }

  // Helper: Convert effects to string
  std::string effectsToString(uint64_t effects) {
    if (effects == EFFECT_PURE)
      return "pure";

    std::string result;
    if (effects & EFFECT_IO)
      result += "io|";
    if (effects & EFFECT_MEMORY)
      result += "memory|";
    if (effects & EFFECT_THROW)
      result += "throw|";
    if (effects & EFFECT_ASYNC)
      result += "async|";
    if (effects & EFFECT_UNSAFE)
      result += "unsafe|";
    if (effects & EFFECT_UNKNOWN)
      result += "unknown|";

    if (!result.empty())
      result.pop_back(); // Remove trailing '|'

    return result;
  }

  StringRef getArgument() const final { return "nova-effect-check"; }

  StringRef getDescription() const final {
    return "Infer and check side effects";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createEffectCheckPass() {
  return std::make_unique<EffectCheckPass>();
}

} // namespace nova
} // namespace mlir
