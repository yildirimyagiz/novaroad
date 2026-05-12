//===- dialect.cpp - Nova dialect implementation
//-------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "dialect.h"
#include "attrs.h"
#include "ops.h"
#include "types.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace mlir;
using namespace mlir::nova;

// Include generated dialect definition
#include "NovaDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Nova Dialect
//===----------------------------------------------------------------------===//

void NovaDialect::initialize() {
  // Register types
  addTypes<
#define GET_TYPEDEF_LIST
#include "NovaTypes.cpp.inc"
      >();

  // Register attributes
  addAttributes<
#define GET_ATTRDEF_LIST
#include "NovaAttrs.cpp.inc"
      >();

  // Register operations
  addOperations<
#define GET_OP_LIST
#include "NovaOps.cpp.inc"
      >();
}

//===----------------------------------------------------------------------===//
// Registration
//===----------------------------------------------------------------------===//

void mlir::nova::registerNovaDialect(DialectRegistry &registry) {
  registry.insert<NovaDialect>();
}
