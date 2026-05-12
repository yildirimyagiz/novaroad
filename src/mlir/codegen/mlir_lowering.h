//===- mlir_lowering.h - Nova to Standard MLIR Lowering ------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_CODEGEN_MLIR_LOWERING_H
#define NOVA_MLIR_CODEGEN_MLIR_LOWERING_H

#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
namespace nova {

/// Create a pass to lower Nova dialect to standard MLIR dialects
std::unique_ptr<Pass> createLowerToStandardPass();

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_CODEGEN_MLIR_LOWERING_H
