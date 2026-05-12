//===- llvm_lowering.h - MLIR to LLVM IR Lowering --------------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_CODEGEN_LLVM_LOWERING_H
#define NOVA_MLIR_CODEGEN_LLVM_LOWERING_H

#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
namespace nova {

/// Create a pass to lower standard MLIR to LLVM dialect
std::unique_ptr<Pass> createLowerToLLVMPass();

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_CODEGEN_LLVM_LOWERING_H
