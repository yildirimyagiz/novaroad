//===- ops.cpp - Nova operation definitions -----------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "ops.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/FunctionImplementation.h"
#include "mlir/IR/OpImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace mlir;
using namespace mlir::nova;

//===----------------------------------------------------------------------===//
// NovaDialect Operation Verifiers
//===----------------------------------------------------------------------===//

// Custom verifiers can be added here for complex constraints

//===----------------------------------------------------------------------===//
// CallOp
//===----------------------------------------------------------------------===//

CallInterfaceCallable CallOp::getCallableForCallee() {
  return (*this)->getAttrOfType<SymbolRefAttr>("callee");
}

void CallOp::setCalleeFromCallable(CallInterfaceCallable callee) {
  (*this)->setAttr("callee", callee.get<SymbolRefAttr>());
}

Operation::operand_range CallOp::getArgOperands() { return getOperands(); }

//===----------------------------------------------------------------------===//
// FnOp
//===----------------------------------------------------------------------===//

// Implement FunctionOpInterface methods if needed

// Include generated operation definitions
#define GET_OP_CLASSES
#include "NovaOps.cpp.inc"
