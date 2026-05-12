//===- hir_to_mlir.cpp - HIR to MLIR Lowering ----------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// Lower Nova HIR (High-level IR) to MLIR with Mojo-specific optimizations.
//
//===----------------------------------------------------------------------===//

#include "hir_to_mlir.h"
#include "compiler/mlir/dialect/attrs.h"
#include "compiler/mlir/dialect/ops.h"
#include "compiler/mlir/dialect/types.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"

#include <unordered_map>
#include <vector>

namespace nova {

//===----------------------------------------------------------------------===//
// HIR Type Definitions (Forward declarations from Nova frontend)
//===----------------------------------------------------------------------===//

// Basic HIR node types
enum class HIRNodeKind {
  Module,
  Function,
  Statement,
  Expression,
  Type,
  Pattern
};

// HIR Type system
class HIRType {
public:
  virtual ~HIRType() = default;
  virtual std::string getName() const = 0;
  virtual bool isIntegerType() const { return false; }
  virtual bool isFloatType() const { return false; }
  virtual bool isSIMDType() const { return false; }
};

class HIRIntegerType : public HIRType {
  int width_;

public:
  explicit HIRIntegerType(int width) : width_(width) {}
  std::string getName() const override { return "i" + std::to_string(width_); }
  bool isIntegerType() const override { return true; }
  int getWidth() const { return width_; }
};

class HIRFloatType : public HIRType {
  int width_;

public:
  explicit HIRFloatType(int width) : width_(width) {}
  std::string getName() const override { return "f" + std::to_string(width_); }
  bool isFloatType() const override { return true; }
  int getWidth() const { return width_; }
};

// Mojo-specific SIMD type
class HIRSIMDType : public HIRType {
  HIRType *elementType_;
  int width_;

public:
  HIRSIMDType(HIRType *elemType, int width)
      : elementType_(elemType), width_(width) {}
  std::string getName() const override {
    return "simd<" + elementType_->getName() + ", " + std::to_string(width_) +
           ">";
  }
  bool isSIMDType() const override { return true; }
  HIRType *getElementType() const { return elementType_; }
  int getWidth() const { return width_; }
};

// HIR Expression types
enum class HIRExprKind {
  Literal,
  Variable,
  BinaryOp,
  UnaryOp,
  Call,
  SIMDOp,     // Mojo-specific
  CompileTime // Mojo compile-time evaluation
};

class HIRExpression {
public:
  HIRExprKind kind;
  HIRType *type;

  explicit HIRExpression(HIRExprKind k, HIRType *t) : kind(k), type(t) {}
  virtual ~HIRExpression() = default;
};

class HIRLiteral : public HIRExpression {
public:
  int64_t intValue;
  double floatValue;
  bool isFloat;

  HIRLiteral(int64_t val, HIRType *t)
      : HIRExpression(HIRExprKind::Literal, t), intValue(val), isFloat(false) {}
  HIRLiteral(double val, HIRType *t)
      : HIRExpression(HIRExprKind::Literal, t), floatValue(val), isFloat(true) {
  }
};

class HIRVariable : public HIRExpression {
public:
  std::string name;

  HIRVariable(const std::string &n, HIRType *t)
      : HIRExpression(HIRExprKind::Variable, t), name(n) {}
};

class HIRBinaryOp : public HIRExpression {
public:
  enum OpKind { Add, Sub, Mul, Div, Mod, And, Or, Xor };
  OpKind op;
  HIRExpression *left;
  HIRExpression *right;

  HIRBinaryOp(OpKind o, HIRExpression *l, HIRExpression *r, HIRType *t)
      : HIRExpression(HIRExprKind::BinaryOp, t), op(o), left(l), right(r) {}
};

// Mojo SIMD operation
class HIRSIMDOp : public HIRExpression {
public:
  enum SIMDOpKind { Load, Store, Add, Mul, FMA };
  SIMDOpKind op;
  std::vector<HIRExpression *> operands;

  HIRSIMDOp(SIMDOpKind o, std::vector<HIRExpression *> ops, HIRType *t)
      : HIRExpression(HIRExprKind::SIMDOp, t), op(o), operands(ops) {}
};

// HIR Statement types
enum class HIRStmtKind { VarDecl, Assignment, Return, If, While, For };

class HIRStatement {
public:
  HIRStmtKind kind;

  explicit HIRStatement(HIRStmtKind k) : kind(k) {}
  virtual ~HIRStatement() = default;
};

class HIRVarDecl : public HIRStatement {
public:
  std::string name;
  HIRType *type;
  HIRExpression *initializer;
  bool isMutable;

  HIRVarDecl(const std::string &n, HIRType *t, HIRExpression *init, bool mut)
      : HIRStatement(HIRStmtKind::VarDecl), name(n), type(t), initializer(init),
        isMutable(mut) {}
};

class HIRReturn : public HIRStatement {
public:
  HIRExpression *value;

  explicit HIRReturn(HIRExpression *val)
      : HIRStatement(HIRStmtKind::Return), value(val) {}
};

// HIR Function
class HIRFunction {
public:
  std::string name;
  std::vector<std::pair<std::string, HIRType *>> parameters;
  HIRType *returnType;
  std::vector<HIRStatement *> body;
  bool isInline;      // Mojo-style inlining hint
  bool isCompileTime; // Mojo compile-time function

  HIRFunction(const std::string &n)
      : name(n), returnType(nullptr), isInline(false), isCompileTime(false) {}
};

// HIR Module
class HIRModule {
public:
  std::string name;
  std::vector<HIRFunction *> functions;
  std::unordered_map<std::string, HIRType *> types;

  explicit HIRModule(const std::string &n) : name(n) {}
};

//===----------------------------------------------------------------------===//
// HIR to MLIR Lowering Implementation
//===----------------------------------------------------------------------===//

HIRToMLIRLowering::HIRToMLIRLowering(mlir::MLIRContext *context)
    : context(context), builder(context) {}

mlir::OwningOpRef<mlir::ModuleOp>
HIRToMLIRLowering::lower(const HIRModule &hirModule) {
  // Create MLIR module
  auto location = builder.getUnknownLoc();
  auto module = mlir::ModuleOp::create(location);

  builder.setInsertionPointToEnd(module.getBody());

  // Lower all functions
  for (const auto *func : hirModule.functions) {
    lowerFunction(*func, module);
  }

  return mlir::OwningOpRef<mlir::ModuleOp>(module);
}

void HIRToMLIRLowering::lowerFunction(const HIRFunction &func,
                                      mlir::ModuleOp module) {
  auto location = builder.getUnknownLoc();

  // Convert parameter types
  llvm::SmallVector<mlir::Type, 4> paramTypes;
  for (const auto &[name, type] : func.parameters) {
    paramTypes.push_back(convertType(type));
  }

  // Convert return type
  mlir::Type returnType =
      func.returnType ? convertType(func.returnType) : builder.getNoneType();

  // Create function type
  auto funcType = builder.getFunctionType(paramTypes, returnType);

  // Create function operation
  auto funcOp =
      builder.create<mlir::func::FuncOp>(location, func.name, funcType);

  // Add Mojo-specific attributes
  if (func.isInline) {
    funcOp->setAttr("nova.inline", builder.getUnitAttr());
  }
  if (func.isCompileTime) {
    funcOp->setAttr("nova.const_eval", builder.getUnitAttr());
  }

  // Create entry block
  auto *entryBlock = funcOp.addEntryBlock();
  builder.setInsertionPointToStart(entryBlock);

  // Map parameters to block arguments
  valueMap_.clear();
  for (size_t i = 0; i < func.parameters.size(); ++i) {
    valueMap_[func.parameters[i].first] = entryBlock->getArgument(i);
  }

  // Lower function body
  for (const auto *stmt : func.body) {
    lowerStatement(*stmt, entryBlock);
  }

  // Add to module
  module.push_back(funcOp);
}

void HIRToMLIRLowering::lowerStatement(const HIRStatement &stmt,
                                       mlir::Block *block) {
  auto location = builder.getUnknownLoc();
  builder.setInsertionPointToEnd(block);

  switch (stmt.kind) {
  case HIRStmtKind::VarDecl: {
    const auto &varDecl = static_cast<const HIRVarDecl &>(stmt);

    // Lower initializer
    mlir::Value initValue = lowerExpression(*varDecl.initializer, block);

    // Store in value map
    valueMap_[varDecl.name] = initValue;
    break;
  }

  case HIRStmtKind::Return: {
    const auto &retStmt = static_cast<const HIRReturn &>(stmt);

    if (retStmt.value) {
      mlir::Value retValue = lowerExpression(*retStmt.value, block);
      builder.create<mlir::func::ReturnOp>(location, retValue);
    } else {
      builder.create<mlir::func::ReturnOp>(location);
    }
    break;
  }

  default:
    break;
  }
}

mlir::Value HIRToMLIRLowering::lowerExpression(const HIRExpression &expr,
                                               mlir::Block *block) {
  auto location = builder.getUnknownLoc();
  builder.setInsertionPointToEnd(block);

  switch (expr.kind) {
  case HIRExprKind::Literal: {
    const auto &lit = static_cast<const HIRLiteral &>(expr);
    mlir::Type type = convertType(lit.type);

    if (lit.isFloat) {
      return builder.create<mlir::arith::ConstantOp>(
          location, type, builder.getF64FloatAttr(lit.floatValue));
    } else {
      return builder.create<mlir::arith::ConstantOp>(
          location, type, builder.getI64IntegerAttr(lit.intValue));
    }
  }

  case HIRExprKind::Variable: {
    const auto &var = static_cast<const HIRVariable &>(expr);
    return valueMap_[var.name];
  }

  case HIRExprKind::BinaryOp: {
    const auto &binOp = static_cast<const HIRBinaryOp &>(expr);

    mlir::Value left = lowerExpression(*binOp.left, block);
    mlir::Value right = lowerExpression(*binOp.right, block);

    // Determine if integer or float operation
    bool isFloat = binOp.type->isFloatType();

    switch (binOp.op) {
    case HIRBinaryOp::Add:
      return isFloat
                 ? builder.create<mlir::arith::AddFOp>(location, left, right)
                       .getResult()
                 : builder.create<mlir::arith::AddIOp>(location, left, right)
                       .getResult();
    case HIRBinaryOp::Mul:
      return isFloat
                 ? builder.create<mlir::arith::MulFOp>(location, left, right)
                       .getResult()
                 : builder.create<mlir::arith::MulIOp>(location, left, right)
                       .getResult();
    default:
      return mlir::Value();
    }
  }

  case HIRExprKind::SIMDOp: {
    // Mojo-specific SIMD lowering
    const auto &simdOp = static_cast<const HIRSIMDOp &>(expr);

    // Convert to vector operations
    std::vector<mlir::Value> operandValues;
    for (auto *operand : simdOp.operands) {
      operandValues.push_back(lowerExpression(*operand, block));
    }

    // Mark for vectorization
    mlir::Value result = lowerSIMDOperation(simdOp, operandValues, location);
    return result;
  }

  default:
    return mlir::Value();
  }
}

mlir::Type HIRToMLIRLowering::convertType(HIRType *hirType) {
  if (!hirType)
    return builder.getNoneType();

  if (auto *intType = dynamic_cast<HIRIntegerType *>(hirType)) {
    return builder.getIntegerType(intType->getWidth());
  } else if (auto *floatType = dynamic_cast<HIRFloatType *>(hirType)) {
    if (floatType->getWidth() == 32)
      return builder.getF32Type();
    if (floatType->getWidth() == 64)
      return builder.getF64Type();
  } else if (auto *simdType = dynamic_cast<HIRSIMDType *>(hirType)) {
    // Convert to MLIR vector type (Mojo-style)
    mlir::Type elemType = convertType(simdType->getElementType());
    return mlir::VectorType::get({simdType->getWidth()}, elemType);
  }

  return builder.getNoneType();
}

mlir::Value
HIRToMLIRLowering::lowerSIMDOperation(const HIRSIMDOp &simdOp,
                                      const std::vector<mlir::Value> &operands,
                                      mlir::Location location) {

  // Create SIMD operations with vectorization hints
  mlir::Value result;

  switch (simdOp.op) {
  case HIRSIMDOp::Add:
    if (operands.size() == 2) {
      auto addOp = builder.create<mlir::arith::AddFOp>(location, operands[0],
                                                       operands[1]);
      addOp->setAttr("nova.vectorize", builder.getUnitAttr());
      addOp->setAttr("nova.simd_width",
                     builder.getI32IntegerAttr(8)); // Mojo-style SIMD width
      result = addOp.getResult();
    }
    break;

  case HIRSIMDOp::Mul:
    if (operands.size() == 2) {
      auto mulOp = builder.create<mlir::arith::MulFOp>(location, operands[0],
                                                       operands[1]);
      mulOp->setAttr("nova.vectorize", builder.getUnitAttr());
      mulOp->setAttr("nova.simd_width", builder.getI32IntegerAttr(8));
      result = mulOp.getResult();
    }
    break;

  case HIRSIMDOp::FMA:
    // Fused multiply-add (Mojo optimization)
    if (operands.size() == 3) {
      // a * b + c
      auto mulOp = builder.create<mlir::arith::MulFOp>(location, operands[0],
                                                       operands[1]);
      auto addOp = builder.create<mlir::arith::AddFOp>(
          location, mulOp.getResult(), operands[2]);
      addOp->setAttr("nova.fma", builder.getUnitAttr()); // Mark for FMA fusion
      result = addOp.getResult();
    }
    break;

  default:
    break;
  }

  return result;
}

} // namespace nova
