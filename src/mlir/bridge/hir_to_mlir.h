//===- hir_to_mlir.h - HIR to MLIR Lowering --------------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// Lower Nova HIR to MLIR (Nova dialect) with Mojo-level optimizations.
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_BRIDGE_HIR_TO_MLIR_H
#define NOVA_MLIR_BRIDGE_HIR_TO_MLIR_H

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Value.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nova {

// Forward declarations (HIR types from Nova frontend)
class HIRModule;
class HIRFunction;
class HIRStatement;
class HIRExpression;
class HIRType;
class HIRSIMDOp;

/// Lower Nova HIR to MLIR with Mojo-specific optimizations
class HIRToMLIRLowering {
public:
  explicit HIRToMLIRLowering(mlir::MLIRContext *context);

  /// Lower complete HIR module to MLIR
  mlir::OwningOpRef<mlir::ModuleOp> lower(const HIRModule &hirModule);

private:
  mlir::MLIRContext *context;
  mlir::OpBuilder builder;
  std::unordered_map<std::string, mlir::Value> valueMap_;

  // Core lowering methods
  void lowerFunction(const HIRFunction &func, mlir::ModuleOp module);
  void lowerStatement(const HIRStatement &stmt, mlir::Block *block);
  mlir::Value lowerExpression(const HIRExpression &expr, mlir::Block *block);

  // Type conversion
  mlir::Type convertType(HIRType *hirType);

  // Mojo-specific SIMD lowering
  mlir::Value lowerSIMDOperation(const HIRSIMDOp &simdOp,
                                 const std::vector<mlir::Value> &operands,
                                 mlir::Location location);
};

} // namespace nova

#endif // NOVA_MLIR_BRIDGE_HIR_TO_MLIR_H
