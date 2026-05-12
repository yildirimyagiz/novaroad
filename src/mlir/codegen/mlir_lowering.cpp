//===- mlir_lowering.cpp - Nova to Standard MLIR Lowering -------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// Lower Nova dialect to standard MLIR dialects.
//
//===----------------------------------------------------------------------===//

#include "mlir_lowering.h"
#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Type Conversion
//===----------------------------------------------------------------------===//

class NovaTypeConverter : public TypeConverter {
public:
  NovaTypeConverter() {
    // Convert owned types to their base types
    addConversion([](Type type) -> Type {
      if (auto ownedType = type.dyn_cast<OwnedType>())
        return ownedType.getElementType();
      return type;
    });

    // Convert reference types to memref
    addConversion([](Type type) -> Type {
      if (auto refType = type.dyn_cast<RefType>()) {
        auto elemType = refType.getElementType();
        return MemRefType::get({}, elemType);
      }
      return type;
    });

    // Convert linear types to their base types
    addConversion([](Type type) -> Type {
      if (auto linType = type.dyn_cast<LinearType>())
        return linType.getElementType();
      return type;
    });
  }
};

//===----------------------------------------------------------------------===//
// Operation Lowering Patterns
//===----------------------------------------------------------------------===//

// Lower z.fn to func.func
struct FnOpLowering : public OpConversionPattern<FnOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(FnOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {

    // Convert function type
    TypeConverter *typeConverter = getTypeConverter();
    auto funcType = op.getFunctionType();

    SmallVector<Type> argTypes;
    for (Type argType : funcType.getInputs()) {
      argTypes.push_back(typeConverter->convertType(argType));
    }

    SmallVector<Type> resultTypes;
    for (Type resultType : funcType.getResults()) {
      resultTypes.push_back(typeConverter->convertType(resultType));
    }

    auto newFuncType =
        FunctionType::get(op.getContext(), argTypes, resultTypes);

    // Create func.func
    auto funcOp = rewriter.create<func::FuncOp>(op.getLoc(), op.getSymName(),
                                                newFuncType);

    // Move body
    if (!op.getBody().empty()) {
      rewriter.inlineRegionBefore(op.getBody(), funcOp.getBody(), funcOp.end());
    }

    rewriter.eraseOp(op);
    return success();
  }
};

// Lower z.call to func.call
struct CallOpLowering : public OpConversionPattern<CallOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(CallOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {

    rewriter.replaceOpWithNewOp<func::CallOp>(
        op, op.getCallee(), op.getResultTypes(), adaptor.getOperands());

    return success();
  }
};

// Lower z.alloc to memref.alloc
struct AllocOpLowering : public OpConversionPattern<AllocOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(AllocOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {

    // Convert type
    auto ownedType = op.getResult().getType().cast<OwnedType>();
    auto elemType = ownedType.getElementType();
    auto memrefType = MemRefType::get({}, elemType);

    rewriter.replaceOpWithNewOp<memref::AllocOp>(op, memrefType);
    return success();
  }
};

// Lower z.free to memref.dealloc
struct FreeOpLowering : public OpConversionPattern<FreeOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(FreeOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {

    rewriter.replaceOpWithNewOp<memref::DeallocOp>(op, adaptor.getValue());
    return success();
  }
};

// Lower z.borrow (becomes no-op in standard MLIR)
struct BorrowOpLowering : public OpConversionPattern<BorrowOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(BorrowOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {

    // Borrow is compile-time only - just forward the value
    rewriter.replaceOp(op, adaptor.getValue());
    return success();
  }
};

// Lower z.move (becomes no-op)
struct MoveOpLowering : public OpConversionPattern<MoveOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(MoveOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {

    // Move is compile-time only - just forward the value
    rewriter.replaceOp(op, adaptor.getSource());
    return success();
  }
};

// Lower z.drop (becomes no-op or dealloc)
struct DropOpLowering : public OpConversionPattern<DropOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(DropOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {

    // For now, just remove (destructor logic would go here)
    rewriter.eraseOp(op);
    return success();
  }
};

//===----------------------------------------------------------------------===//
// Lowering Pass
//===----------------------------------------------------------------------===//

struct LowerToStandardPass
    : public PassWrapper<LowerToStandardPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerToStandardPass)

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    ModuleOp module = getOperation();

    // Setup conversion target
    ConversionTarget target(*ctx);
    target.addLegalDialect<func::FuncDialect, memref::MemRefDialect,
                           arith::ArithDialect>();
    target.addIllegalDialect<NovaDialect>();

    // Setup type converter
    NovaTypeConverter typeConverter;

    // Setup patterns
    RewritePatternSet patterns(ctx);
    patterns.add<FnOpLowering, CallOpLowering, AllocOpLowering, FreeOpLowering,
                 BorrowOpLowering, MoveOpLowering, DropOpLowering>(
        typeConverter, ctx);

    // Apply conversion
    if (failed(applyFullConversion(module, target, std::move(patterns)))) {
      signalPassFailure();
    }
  }

  StringRef getArgument() const final { return "nova-lower-to-standard"; }

  StringRef getDescription() const final {
    return "Lower Nova dialect to standard MLIR dialects";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createLowerToStandardPass() {
  return std::make_unique<LowerToStandardPass>();
}

} // namespace nova
} // namespace mlir
