//===- type_helpers.cpp - MLIR Type Utilities ----------------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "type_helpers.h"
#include "mlir/IR/BuiltinTypes.h"
#include "llvm/Support/raw_ostream.h"
#include <sstream>

namespace nova {
namespace mlir_utils {

//===----------------------------------------------------------------------===//
// Basic Type Queries (Legacy - Maintained)
//===----------------------------------------------------------------------===//

bool isSIMDType(mlir::Type t) { return t && t.isa<mlir::VectorType>(); }

int64_t simdWidth(mlir::Type t) {
  if (auto vt = t.dyn_cast_or_null<mlir::VectorType>()) {
    if (vt.getRank() == 1)
      return vt.getShape()[0];
  }
  return 0;
}

bool isFloatType(mlir::Type t) {
  t = elementTypeOf(t);
  return t && t.isa<mlir::FloatType>();
}

bool isIntType(mlir::Type t) {
  t = elementTypeOf(t);
  return t && t.isa<mlir::IntegerType>();
}

mlir::Type elementTypeOf(mlir::Type t) {
  if (!t)
    return {};
  if (auto vt = t.dyn_cast<mlir::VectorType>())
    return vt.getElementType();
  if (auto tt = t.dyn_cast<mlir::TensorType>())
    return tt.getElementType();
  if (auto mt = t.dyn_cast<mlir::MemRefType>())
    return mt.getElementType();
  return t;
}

//===----------------------------------------------------------------------===//
// Nova-Specific Type Queries (NEW)
//===----------------------------------------------------------------------===//

bool hasOwnershipSemantics(mlir::Type t) {
  if (!t)
    return false;

  // Check type name for Nova ownership markers
  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.own<") != std::string::npos ||
         typeName.find("z.ref<") != std::string::npos ||
         typeName.find("z.mut<") != std::string::npos ||
         typeName.find("z.lin<") != std::string::npos;
}

bool isOwnedType(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.own<") != std::string::npos;
}

bool isReferenceType(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.ref<") != std::string::npos;
}

bool isMutableReferenceType(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.mut<") != std::string::npos;
}

bool isLinearType(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.lin<") != std::string::npos;
}

bool hasLifetime(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  // Look for lifetime annotation (e.g., "'a", "'static")
  return typeName.find("'") != std::string::npos;
}

llvm::Optional<std::string> getLifetimeName(mlir::Type t) {
  if (!hasLifetime(t))
    return llvm::None;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  // Extract lifetime name (simplified - needs proper parsing)
  size_t pos = typeName.find("'");
  if (pos != std::string::npos) {
    size_t end = typeName.find_first_of(",>", pos);
    if (end != std::string::npos) {
      return typeName.substr(pos, end - pos);
    }
  }

  return llvm::None;
}

//===----------------------------------------------------------------------===//
// Effect System Type Queries (NEW)
//===----------------------------------------------------------------------===//

bool hasEffects(mlir::Type t) {
  // In real implementation, check type attributes
  // For now, conservative check
  return false;
}

uint64_t getEffects(mlir::Type t) {
  // Return effect bitmask from type attributes
  // For now, assume pure
  return 0; // EFFECT_PURE
}

bool isPureType(mlir::Type t) { return !hasEffects(t); }

bool hasIOEffect(mlir::Type t) {
  return (getEffects(t) & 0x01) != 0; // EFFECT_IO = 0x01
}

//===----------------------------------------------------------------------===//
// Dependent Type Queries (NEW)
//===----------------------------------------------------------------------===//

bool isDependentFunction(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.pi<") != std::string::npos;
}

bool isDependentPair(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.sigma<") != std::string::npos;
}

bool isEqualityType(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.eq<") != std::string::npos;
}

bool isUniverseType(mlir::Type t) {
  if (!t)
    return false;

  std::string typeName;
  llvm::raw_string_ostream os(typeName);
  t.print(os);
  os.flush();

  return typeName.find("z.type<") != std::string::npos;
}

llvm::Optional<unsigned> getUniverseLevel(mlir::Type t) {
  if (!isUniverseType(t))
    return llvm::None;

  // Extract level from "z.type<N>"
  // Simplified - needs proper parsing
  return 0;
}

//===----------------------------------------------------------------------===//
// Advanced SIMD Queries (Extended)
//===----------------------------------------------------------------------===//

bool isSIMDCompatible(mlir::Type t) {
  t = elementTypeOf(t);

  // Floats and integers are SIMD-compatible
  return isFloatType(t) || isIntType(t);
}

int64_t getOptimalSIMDWidth(mlir::Type t) {
  t = elementTypeOf(t);

  if (!isSIMDCompatible(t))
    return 1;

  // Get bit width
  unsigned bitWidth = 0;
  if (auto ft = t.dyn_cast<mlir::FloatType>()) {
    bitWidth = ft.getWidth();
  } else if (auto it = t.dyn_cast<mlir::IntegerType>()) {
    bitWidth = it.getWidth();
  }

  // Target-specific optimal width (assume 256-bit SIMD)
  if (bitWidth == 32)
    return 8; // 256 / 32 = 8
  if (bitWidth == 64)
    return 4; // 256 / 64 = 4
  if (bitWidth == 16)
    return 16; // 256 / 16 = 16
  if (bitWidth == 8)
    return 32; // 256 / 8 = 32

  return 1;
}

bool canVectorize(mlir::Type t) {
  return isSIMDCompatible(t) && !hasOwnershipSemantics(t);
}

mlir::Type getVectorizedType(mlir::Type scalarType, int64_t width) {
  if (width <= 1)
    return scalarType;

  return mlir::VectorType::get({width}, scalarType);
}

//===----------------------------------------------------------------------===//
// Type Conversion Utilities (NEW)
//===----------------------------------------------------------------------===//

mlir::Type novaToMLIR(mlir::Type novaType) {
  // Strip Nova-specific annotations
  return stripOwnership(novaType);
}

mlir::Type mlirToNova(mlir::Type mlirType, bool addOwnershipAnnotation) {
  if (!addOwnershipAnnotation)
    return mlirType;

  // Default to owned type
  return addOwnership(mlirType, "own");
}

mlir::Type stripOwnership(mlir::Type t) {
  // In real implementation: unwrap ownership wrapper types
  // For now, return as-is
  return t;
}

mlir::Type addOwnership(mlir::Type t, const std::string &ownershipKind) {
  // In real implementation: wrap type with ownership
  // For now, return as-is
  return t;
}

//===----------------------------------------------------------------------===//
// Type Equivalence (NEW)
//===----------------------------------------------------------------------===//

bool areStructurallyEquivalent(mlir::Type t1, mlir::Type t2) {
  if (!t1 || !t2)
    return t1 == t2;

  // Exact equality for now
  return t1 == t2;
}

bool areEquivalentIgnoringOwnership(mlir::Type t1, mlir::Type t2) {
  return areStructurallyEquivalent(stripOwnership(t1), stripOwnership(t2));
}

bool areEquivalentIgnoringLifetimes(mlir::Type t1, mlir::Type t2) {
  // For now, same as structural equivalence
  return areStructurallyEquivalent(t1, t2);
}

//===----------------------------------------------------------------------===//
// Type Size/Alignment (NEW)
//===----------------------------------------------------------------------===//

llvm::Optional<int64_t> getSizeInBytes(mlir::Type t) {
  t = elementTypeOf(t);

  if (auto ft = t.dyn_cast<mlir::FloatType>()) {
    return ft.getWidth() / 8;
  }
  if (auto it = t.dyn_cast<mlir::IntegerType>()) {
    return it.getWidth() / 8;
  }

  return llvm::None;
}

llvm::Optional<int64_t> getAlignmentInBytes(mlir::Type t) {
  auto size = getSizeInBytes(t);
  if (!size)
    return llvm::None;

  // Natural alignment: same as size (up to 16 bytes)
  return std::min(*size, int64_t(16));
}

bool isZeroSized(mlir::Type t) {
  auto size = getSizeInBytes(t);
  return size && *size == 0;
}

//===----------------------------------------------------------------------===//
// Type Construction Helpers (NEW)
//===----------------------------------------------------------------------===//

mlir::Type createOwnedType(mlir::Type innerType, mlir::MLIRContext *ctx) {
  // In real implementation: create z.own<T>
  // For now, return inner type
  return innerType;
}

mlir::Type createReferenceType(mlir::Type innerType,
                               const std::string &lifetime,
                               mlir::MLIRContext *ctx) {
  // In real implementation: create z.ref<T,'a>
  return innerType;
}

mlir::Type createMutableReferenceType(mlir::Type innerType,
                                      const std::string &lifetime,
                                      mlir::MLIRContext *ctx) {
  // In real implementation: create z.mut<T,'a>
  return innerType;
}

mlir::Type createLinearType(mlir::Type innerType, mlir::MLIRContext *ctx) {
  // In real implementation: create z.lin<T>
  return innerType;
}

//===----------------------------------------------------------------------===//
// Debug/Display Utilities (NEW)
//===----------------------------------------------------------------------===//

std::string getTypeName(mlir::Type t) {
  if (!t)
    return "<null>";

  std::string result;
  llvm::raw_string_ostream os(result);
  t.print(os);
  os.flush();
  return result;
}

std::string getDetailedTypeDescription(mlir::Type t) {
  std::stringstream ss;

  ss << "Type: " << getTypeName(t) << "\n";

  if (hasOwnershipSemantics(t)) {
    ss << "  Ownership: ";
    if (isOwnedType(t))
      ss << "Owned\n";
    else if (isReferenceType(t))
      ss << "Reference\n";
    else if (isMutableReferenceType(t))
      ss << "Mutable Reference\n";
    else if (isLinearType(t))
      ss << "Linear\n";

    if (auto lifetime = getLifetimeName(t)) {
      ss << "  Lifetime: " << *lifetime << "\n";
    }
  }

  if (isSIMDType(t)) {
    ss << "  SIMD Width: " << simdWidth(t) << "\n";
  }

  if (auto size = getSizeInBytes(t)) {
    ss << "  Size: " << *size << " bytes\n";
  }

  if (auto align = getAlignmentInBytes(t)) {
    ss << "  Alignment: " << *align << " bytes\n";
  }

  return ss.str();
}

bool isWellFormed(mlir::Type t) {
  // Basic well-formedness checks
  if (!t)
    return false;

  // Check for valid SIMD dimensions
  if (isSIMDType(t)) {
    int64_t width = simdWidth(t);
    if (width <= 0 || (width & (width - 1)) != 0) {
      // Width must be power of 2
      return false;
    }
  }

  return true;
}

} // namespace mlir_utils
} // namespace nova
