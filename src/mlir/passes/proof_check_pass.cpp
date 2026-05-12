//===- proof_check_pass.cpp - Proof Checking Pass ----------------------===//
//
// Validates proof-carrying code and proof obligations
//
//===----------------------------------------------------------------------===//

#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/raw_ostream.h"

namespace mlir {
namespace nova {

/// Proof checking pass
struct ProofCheckPass
    : public PassWrapper<ProofCheckPass, OperationPass<ModuleOp>> {
  StringRef getArgument() const final { return "nova-proof-check"; }
  StringRef getDescription() const final {
    return "Verify proof obligations in Nova IR";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    bool hasErrors = false;

    // Walk all operations looking for proof attributes
    module.walk([&](Operation *op) {
      if (auto proofAttr = op->getAttrOfType<StringAttr>("proof")) {
        if (!checkProof(op, proofAttr)) {
          op->emitError("Proof verification failed");
          hasErrors = true;
        }
      }

      // Check specific proof operations
      if (op->getName().getStringRef().startswith("z.proof.")) {
        if (!checkProofOp(op)) {
          op->emitError("Invalid proof operation");
          hasErrors = true;
        }
      }

      // Check termination annotations
      if (op->getName().getStringRef() == "z.structural_recursion") {
        if (!checkStructuralRecursion(op)) {
          op->emitError("Structural recursion check failed");
          hasErrors = true;
        }
      }
    });

    if (hasErrors) {
      signalPassFailure();
    }
  }

private:
  /// Check proof attribute
  bool checkProof(Operation *op, StringAttr proofAttr) {
    // TODO: Implement full proof checking
    // For now, just validate that proof exists
    return !proofAttr.getValue().empty();
  }

  /// Check proof operation validity
  bool checkProofOp(Operation *op) {
    StringRef opName = op->getName().getStringRef();

    if (opName == "z.proof.refl") {
      // Reflexivity: must have value operand
      return op->getNumOperands() >= 1;
    } else if (opName == "z.proof.symm") {
      // Symmetry: must have equality proof
      return op->getNumOperands() >= 1;
    } else if (opName == "z.proof.trans") {
      // Transitivity: must have two equality proofs
      return op->getNumOperands() >= 2;
    }

    return true;
  }

  /// Check structural recursion validity
  bool checkStructuralRecursion(Operation *op) {
    // Check that decreasing_param attribute exists
    if (!op->hasAttr("decreasing_param")) {
      return false;
    }

    auto decreasingParam = op->getAttrOfType<IntegerAttr>("decreasing_param");
    if (!decreasingParam) {
      return false;
    }

    // TODO: Validate that recursive calls are on smaller arguments
    return true;
  }
};

} // namespace nova
} // namespace mlir

namespace mlir {
namespace nova {

/// Register the pass
void registerProofCheckPass() { PassRegistration<ProofCheckPass>(); }

} // namespace nova
} // namespace mlir
