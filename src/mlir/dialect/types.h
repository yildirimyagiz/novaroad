//===- types.h - Nova type definitions ------------------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_IR_TYPES_H
#define NOVA_IR_TYPES_H

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Types.h"

// Forward declarations
namespace mlir {
namespace nova {
class NovaDialect;
} // namespace nova
} // namespace mlir

// Include generated type declarations
#define GET_TYPEDEF_CLASSES
#include "NovaTypes.h.inc"

namespace mlir {
namespace nova {

/// Helper to check if a type is an owned type
inline bool isOwnedType(Type type) { return type.isa<OwnedType>(); }

/// Helper to check if a type is a reference type
inline bool isRefType(Type type) { return type.isa<RefType>(); }

/// Helper to check if a type is a linear type
inline bool isLinearType(Type type) { return type.isa<LinearType>(); }

/// Helper to unwrap owned/ref/linear types to get element type
Type unwrapType(Type type);

} // namespace nova
} // namespace mlir

#endif // NOVA_IR_TYPES_H
