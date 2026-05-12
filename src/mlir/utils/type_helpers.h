//===- type_helpers.h - MLIR Type Utilities ----------------------*- C++
//-*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file provides utility functions for working with MLIR types,
// including Nova-specific types (ownership, effects, dependent types).
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_UTILS_TYPE_HELPERS_H
#define NOVA_MLIR_UTILS_TYPE_HELPERS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Types.h"
#include "llvm/ADT/Optional.h"

namespace nova {
namespace mlir_utils {

//===----------------------------------------------------------------------===//
// Basic Type Queries (Legacy - Maintained for Compatibility)
//===----------------------------------------------------------------------===//

/// Check if type is a SIMD/vector type
bool isSIMDType(mlir::Type t);

/// Get SIMD width (0 if not SIMD)
int64_t simdWidth(mlir::Type t);

/// Check if type is floating-point (or contains float elements)
bool isFloatType(mlir::Type t);

/// Check if type is integer (or contains integer elements)
bool isIntType(mlir::Type t);

/// Get element type (unwraps vectors, tensors, memrefs)
mlir::Type elementTypeOf(mlir::Type t);

//===----------------------------------------------------------------------===//
// Nova-Specific Type Queries (NEW)
//===----------------------------------------------------------------------===//

/// Check if type has ownership semantics (z.own<T>, z.ref<T,'a>, z.lin<T>)
bool hasOwnershipSemantics(mlir::Type t);

/// Check if type is owned (z.own<T>)
bool isOwnedType(mlir::Type t);

/// Check if type is borrowed/reference (z.ref<T,'a>)
bool isReferenceType(mlir::Type t);

/// Check if type is mutable reference (z.mut<T,'a>)
bool isMutableReferenceType(mlir::Type t);

/// Check if type is linear (z.lin<T>)
bool isLinearType(mlir::Type t);

/// Check if type has lifetime parameter
bool hasLifetime(mlir::Type t);

/// Get lifetime name (e.g., "'a", "'static") if present
llvm::Optional<std::string> getLifetimeName(mlir::Type t);

//===----------------------------------------------------------------------===//
// Effect System Type Queries (NEW)
//===----------------------------------------------------------------------===//

/// Check if type has effect annotations
bool hasEffects(mlir::Type t);

/// Get effect set from type (if any)
/// Returns: effect bitmask (see effect_analysis.h)
uint64_t getEffects(mlir::Type t);

/// Check if type is pure (no effects)
bool isPureType(mlir::Type t);

/// Check if type involves I/O
bool hasIOEffect(mlir::Type t);

//===----------------------------------------------------------------------===//
// Dependent Type Queries (NEW)
//===----------------------------------------------------------------------===//

/// Check if type is a dependent function (Π-type)
bool isDependentFunction(mlir::Type t);

/// Check if type is a dependent pair (Σ-type)
bool isDependentPair(mlir::Type t);

/// Check if type is equality type
bool isEqualityType(mlir::Type t);

/// Check if type is universe type
bool isUniverseType(mlir::Type t);

/// Get universe level (Type_0, Type_1, etc.)
llvm::Optional<unsigned> getUniverseLevel(mlir::Type t);

//===----------------------------------------------------------------------===//
// Advanced SIMD Queries (Extended)
//===----------------------------------------------------------------------===//

/// Check if type is SIMD-compatible
bool isSIMDCompatible(mlir::Type t);

/// Get optimal SIMD width for type on current target
int64_t getOptimalSIMDWidth(mlir::Type t);

/// Check if type can be vectorized
bool canVectorize(mlir::Type t);

/// Get vectorized version of scalar type
mlir::Type getVectorizedType(mlir::Type scalarType, int64_t width);

//===----------------------------------------------------------------------===//
// Type Conversion Utilities (NEW)
//===----------------------------------------------------------------------===//

/// Convert Nova ownership type to MLIR type
mlir::Type novaToMLIR(mlir::Type novaType);

/// Convert MLIR type to Nova type (with ownership)
mlir::Type mlirToNova(mlir::Type mlirType, bool addOwnership = true);

/// Strip ownership/lifetime annotations
mlir::Type stripOwnership(mlir::Type t);

/// Add ownership annotation
mlir::Type addOwnership(mlir::Type t, const std::string &ownershipKind);

//===----------------------------------------------------------------------===//
// Type Equivalence (NEW)
//===----------------------------------------------------------------------===//

/// Check if two types are structurally equivalent
bool areStructurallyEquivalent(mlir::Type t1, mlir::Type t2);

/// Check if types are equivalent ignoring ownership
bool areEquivalentIgnoringOwnership(mlir::Type t1, mlir::Type t2);

/// Check if types are equivalent ignoring lifetimes
bool areEquivalentIgnoringLifetimes(mlir::Type t1, mlir::Type t2);

//===----------------------------------------------------------------------===//
// Type Size/Alignment (NEW)
//===----------------------------------------------------------------------===//

/// Get size of type in bytes (if known)
llvm::Optional<int64_t> getSizeInBytes(mlir::Type t);

/// Get alignment requirement in bytes
llvm::Optional<int64_t> getAlignmentInBytes(mlir::Type t);

/// Check if type is zero-sized
bool isZeroSized(mlir::Type t);

//===----------------------------------------------------------------------===//
// Type Construction Helpers (NEW)
//===----------------------------------------------------------------------===//

/// Create owned type: z.own<T>
mlir::Type createOwnedType(mlir::Type innerType, mlir::MLIRContext *ctx);

/// Create reference type: z.ref<T,'a>
mlir::Type createReferenceType(mlir::Type innerType,
                               const std::string &lifetime,
                               mlir::MLIRContext *ctx);

/// Create mutable reference: z.mut<T,'a>
mlir::Type createMutableReferenceType(mlir::Type innerType,
                                      const std::string &lifetime,
                                      mlir::MLIRContext *ctx);

/// Create linear type: z.lin<T>
mlir::Type createLinearType(mlir::Type innerType, mlir::MLIRContext *ctx);

//===----------------------------------------------------------------------===//
// Debug/Display Utilities (NEW)
//===----------------------------------------------------------------------===//

/// Get human-readable type name
std::string getTypeName(mlir::Type t);

/// Get detailed type description (with ownership, effects, etc.)
std::string getDetailedTypeDescription(mlir::Type t);

/// Check if type is well-formed
bool isWellFormed(mlir::Type t);

} // namespace mlir_utils
} // namespace nova

#endif // NOVA_MLIR_UTILS_TYPE_HELPERS_H
