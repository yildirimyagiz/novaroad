//===- types.cpp - Nova type definitions --------------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "types.h"
#include "dialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace mlir;
using namespace mlir::nova;

// Include generated type definitions
#define GET_TYPEDEF_CLASSES
#include "NovaTypes.cpp.inc"

//===----------------------------------------------------------------------===//
// Type Utilities
//===----------------------------------------------------------------------===//

Type mlir::nova::unwrapType(Type type) {
  return llvm::TypeSwitch<Type, Type>(type)
      .Case<OwnedType>([](OwnedType t) { return t.getElementType(); })
      .Case<RefType>([](RefType t) { return t.getElementType(); })
      .Case<LinearType>([](LinearType t) { return t.getElementType(); })
      .Default([](Type t) { return t; });
}
