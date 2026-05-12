//===- ir_helpers.h - MLIR IR Utility Functions ------------------*- C++
//-*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains utility functions for working with MLIR IR constructs.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"

namespace nova {
namespace mlir_utils {

// ============================================================================
// Attribute Creation Helpers
// ============================================================================

/// Create a UnitAttr
mlir::UnitAttr unit(mlir::OpBuilder &b);

/// Create an I32 IntegerAttr
mlir::IntegerAttr i32(mlir::OpBuilder &b, int32_t v);

/// Create a BoolAttr
mlir::BoolAttr boolean(mlir::OpBuilder &b, bool v);

// ============================================================================
// Attribute Setting Helpers
// ============================================================================

/// Set a unit attribute on an operation
void setUnitAttr(mlir::Operation *op, mlir::StringRef name);

/// Set an i32 attribute on an operation
void setI32Attr(mlir::Operation *op, mlir::StringRef name, int32_t v);

/// Set a bool attribute on an operation
void setBoolAttr(mlir::Operation *op, mlir::StringRef name, bool v);

// ============================================================================
// Attribute Getting Helpers
// ============================================================================

/// Check if operation has an attribute
bool hasAttr(mlir::Operation *op, mlir::StringRef name);

/// Get bool attribute value with default
bool getBoolAttr(mlir::Operation *op, mlir::StringRef name,
                 bool defaultValue = false);

/// Get i32 attribute value with default
int32_t getI32Attr(mlir::Operation *op, mlir::StringRef name,
                   int32_t defaultValue = 0);

// ============================================================================
// Operation Replacement Helpers
// ============================================================================

/// Replace all uses of an operation's result and erase the operation
/// \param op The operation to replace and erase
/// \param replacement The value to replace with
/// \return Success if operation had exactly one result
mlir::LogicalResult replaceAllUsesAndErase(mlir::Operation *op,
                                           mlir::Value replacement);

/// Replace operation with values and erase (using rewriter)
/// \param rewriter The pattern rewriter
/// \param op The operation to replace
/// \param replacements The replacement values
/// \return Success if number of results matches replacements
mlir::LogicalResult replaceOpWithValuesAndErase(mlir::RewriterBase &rewriter,
                                                mlir::Operation *op,
                                                mlir::ValueRange replacements);

// ============================================================================
// Type and Value Queries
// ============================================================================

/// Check if a value is constant-like
bool isConstantLike(mlir::Value v);

/// Check if a type is vector-like (vector, memref, or tensor)
bool isVectorLike(mlir::Type t);

} // namespace mlir_utils
} // namespace nova
