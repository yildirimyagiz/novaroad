//===- attrs.cpp - Nova attribute definitions ---------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "attrs.h"
#include "dialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"

using namespace mlir;
using namespace mlir::nova;

// Include generated enum definitions
#include "NovaEnums.cpp.inc"

// Include generated attribute definitions
#define GET_ATTRDEF_CLASSES
#include "NovaAttrs.cpp.inc"
