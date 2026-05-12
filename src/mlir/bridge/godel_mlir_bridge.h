//===- godel_mlir_bridge.h - Gödel ↔ MLIR Bridge ----------------*- C++ -*-===//
//
// Part of the Nova Formal Kernel
//
//===----------------------------------------------------------------------===//
//
// Bridge between Gödel symbolic engine and MLIR for verification and
// Mojo-level optimizations.
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_BRIDGE_GODEL_MLIR_BRIDGE_H
#define NOVA_MLIR_BRIDGE_GODEL_MLIR_BRIDGE_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"
#include <memory>

namespace mlir {
namespace nova {

// Forward declaration
class GodelMLIRBridge;

/// Create a new Gödel MLIR bridge
std::unique_ptr<GodelMLIRBridge> createGodelBridge(MLIRContext *ctx);

/// Gödel MLIR Bridge - connects symbolic verification to MLIR
class GodelMLIRBridge {
public:
  explicit GodelMLIRBridge(MLIRContext *ctx);

  /// Lower MLIR module with Gödel verification
  LogicalResult lowerWithVerification(ModuleOp module);

  /// Verify a single operation
  bool verify(Operation *op);

  /// Apply Mojo-specific optimizations
  void applyMojoOptimizations(ModuleOp module);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_BRIDGE_GODEL_MLIR_BRIDGE_H
