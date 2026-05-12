//===- ir_helpers.cpp - MLIR IR Utility Functions ------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "ir_helpers.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinTypes.h"

namespace nova {
namespace mlir_utils {

// ============================================================================
// Attribute Creation
// ============================================================================

mlir::UnitAttr unit(mlir::OpBuilder &b) { return b.getUnitAttr(); }

mlir::IntegerAttr i32(mlir::OpBuilder &b, int32_t v) {
  return b.getI32IntegerAttr(v);
}

mlir::BoolAttr boolean(mlir::OpBuilder &b, bool v) { return b.getBoolAttr(v); }

// ============================================================================
// Attribute Setting
// ============================================================================

void setUnitAttr(mlir::Operation *op, mlir::StringRef name) {
  mlir::OpBuilder b(op->getContext());
  op->setAttr(name, b.getUnitAttr());
}

void setI32Attr(mlir::Operation *op, mlir::StringRef name, int32_t v) {
  mlir::OpBuilder b(op->getContext());
  op->setAttr(name, b.getI32IntegerAttr(v));
}

void setBoolAttr(mlir::Operation *op, mlir::StringRef name, bool v) {
  mlir::OpBuilder b(op->getContext());
  op->setAttr(name, b.getBoolAttr(v));
}

// ============================================================================
// Attribute Getting
// ============================================================================

bool hasAttr(mlir::Operation *op, mlir::StringRef name) {
  return op->hasAttr(name);
}

bool getBoolAttr(mlir::Operation *op, mlir::StringRef name, bool defaultValue) {
  if (auto a = op->getAttrOfType<mlir::BoolAttr>(name))
    return a.getValue();
  if (op->getAttr(name))
    return true; // unit attr -> true
  return defaultValue;
}

int32_t getI32Attr(mlir::Operation *op, mlir::StringRef name,
                   int32_t defaultValue) {
  if (auto a = op->getAttrOfType<mlir::IntegerAttr>(name))
    return (int32_t)a.getInt();
  return defaultValue;
}

// ============================================================================
// Operation Replacement
// ============================================================================

mlir::LogicalResult replaceAllUsesAndErase(mlir::Operation *op,
                                           mlir::Value replacement) {
  if (!op)
    return mlir::failure();
  if (op->getNumResults() != 1)
    return mlir::failure();

  op->getResult(0).replaceAllUsesWith(replacement);
  op->erase();
  return mlir::success();
}

mlir::LogicalResult replaceOpWithValuesAndErase(mlir::RewriterBase &rewriter,
                                                mlir::Operation *op,
                                                mlir::ValueRange replacements) {
  if (!op)
    return mlir::failure();
  if ((size_t)op->getNumResults() != replacements.size())
    return mlir::failure();

  for (auto [res, rep] : llvm::zip(op->getResults(), replacements)) {
    res.replaceAllUsesWith(rep);
  }
  rewriter.eraseOp(op);
  return mlir::success();
}

// ============================================================================
// Type and Value Queries
// ============================================================================

bool isConstantLike(mlir::Value v) {
  if (!v)
    return false;
  if (v.getDefiningOp<mlir::arith::ConstantOp>())
    return true;
  return false;
}

bool isVectorLike(mlir::Type t) {
  return t && (t.isa<mlir::VectorType>() || t.isa<mlir::MemRefType>() ||
               t.isa<mlir::TensorType>());
}

} // namespace mlir_utils
} // namespace nova
