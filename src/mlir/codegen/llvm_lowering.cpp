//===- llvm_lowering.cpp - MLIR to LLVM IR Lowering ---------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// Lower standard MLIR to LLVM IR.
//
//===----------------------------------------------------------------------===//

#include "llvm_lowering.h"

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Target/LLVMIR/Export.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Lower to LLVM Pass
//===----------------------------------------------------------------------===//

struct LowerToLLVMPass
    : public PassWrapper<LowerToLLVMPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerToLLVMPass)

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<LLVM::LLVMDialect>();
  }

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    ModuleOp module = getOperation();

    // Setup conversion target
    LLVMConversionTarget target(*ctx);
    target.addLegalOp<ModuleOp>();

    // Setup type converter
    LLVMTypeConverter typeConverter(ctx);

    // Setup patterns
    RewritePatternSet patterns(ctx);

    // Add conversion patterns
    populateFuncToLLVMConversionPatterns(typeConverter, patterns);
    populateMemRefToLLVMConversionPatterns(typeConverter, patterns);
    populateArithToLLVMConversionPatterns(typeConverter, patterns);
    cf::populateControlFlowToLLVMConversionPatterns(typeConverter, patterns);

    // Apply conversion
    if (failed(applyFullConversion(module, target, std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-lower-to-llvm"; }

  StringRef getDescription() const final {
    return "Lower standard MLIR to LLVM dialect";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createLowerToLLVMPass() {
  return std::make_unique<LowerToLLVMPass>();
}

} // namespace nova
} // namespace mlir
