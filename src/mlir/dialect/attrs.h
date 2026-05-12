//===- attrs.h - Nova attribute definitions -------------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_IR_ATTRS_H
#define NOVA_IR_ATTRS_H

#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"

// Forward declarations
namespace mlir {
namespace nova {
class NovaDialect;
} // namespace nova
} // namespace mlir

// Include generated enum declarations
#include "NovaEnums.h.inc"

// Include generated attribute declarations
#define GET_ATTRDEF_CLASSES
#include "NovaAttrs.h.inc"

namespace mlir {
namespace nova {

/// Effect bit positions
constexpr uint64_t EFFECT_PURE = 0;
constexpr uint64_t EFFECT_IO = 1 << 1;
constexpr uint64_t EFFECT_MEMORY = 1 << 2;
constexpr uint64_t EFFECT_THROW = 1 << 3;
constexpr uint64_t EFFECT_ASYNC = 1 << 4;
constexpr uint64_t EFFECT_UNSAFE = 1 << 5;

} // namespace nova
} // namespace mlir

#endif // NOVA_IR_ATTRS_H
