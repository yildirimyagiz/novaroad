//===- ops.h - Nova operation definitions --------------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_IR_OPS_H
#define NOVA_IR_OPS_H

#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "dialect.h"
#include "types.h"
#include "attrs.h"

// Include generated operation declarations
#define GET_OP_CLASSES
#include "NovaOps.h.inc"

#endif // NOVA_IR_OPS_H
