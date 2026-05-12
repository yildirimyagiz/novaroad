//===- dialect.h - Nova dialect -------------------------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file defines the Nova MLIR dialect.
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_IR_DIALECT_H
#define NOVA_IR_DIALECT_H

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"

// Include generated dialect declaration
#include "NovaDialect.h.inc"

namespace mlir {
namespace nova {

/// Initialize all Nova dialect components
void registerNovaDialect(DialectRegistry &registry);

} // namespace nova
} // namespace mlir

#endif // NOVA_IR_DIALECT_H
