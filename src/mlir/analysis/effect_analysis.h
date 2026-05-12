//===- effect_analysis.h - Effect Analysis Infrastructure ------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_ANALYSIS_EFFECT_ANALYSIS_H
#define NOVA_MLIR_ANALYSIS_EFFECT_ANALYSIS_H

#include "mlir/IR/Operation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include <cstdint>

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Effect Constants
//===----------------------------------------------------------------------===//

constexpr uint64_t EFFECT_PURE = 0;
constexpr uint64_t EFFECT_IO = 1 << 1;
constexpr uint64_t EFFECT_MEMORY = 1 << 2;
constexpr uint64_t EFFECT_THROW = 1 << 3;
constexpr uint64_t EFFECT_ASYNC = 1 << 4;
constexpr uint64_t EFFECT_UNSAFE = 1 << 5;
constexpr uint64_t EFFECT_UNKNOWN = 1 << 6;

//===----------------------------------------------------------------------===//
// Effect Analysis
//===----------------------------------------------------------------------===//

class EffectAnalysis {
public:
  explicit EffectAnalysis(Operation *root);

  // Query effects
  uint64_t getEffects(Operation *op) const;
  uint64_t getFunctionEffects(StringRef funcName) const;

  // Effect checks
  bool isPure(Operation *op) const;
  bool hasIO(Operation *op) const;
  bool hasMemory(Operation *op) const;
  bool hasAsync(Operation *op) const;

  // Effect combination
  uint64_t combineEffects(uint64_t e1, uint64_t e2) const;
  bool isSubsetOf(uint64_t sub, uint64_t sup) const;

  // Utilities
  std::string effectsToString(uint64_t effects) const;

private:
  llvm::DenseMap<Operation *, uint64_t> opEffects;
  llvm::StringMap<uint64_t> functionEffects;

  void analyze(Operation *op);
  uint64_t inferEffects(Operation *op);
};

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_ANALYSIS_EFFECT_ANALYSIS_H
