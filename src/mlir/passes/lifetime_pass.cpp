//===- lifetime_pass.cpp - Lifetime Inference Pass ----------------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file implements lifetime inference (Non-Lexical Lifetimes style).
//
//===----------------------------------------------------------------------===//

#include "compiler/mlir/analysis/lifetime_analysis.h"
#include "compiler/mlir/dialect/nova_attrs.h"
#include "compiler/mlir/dialect/nova_dialect.h"
#include "compiler/mlir/dialect/nova_ops.h"
#include "compiler/mlir/dialect/nova_types.h"
#include "pass_registry.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Lifetime Region
//===----------------------------------------------------------------------===//

struct LifetimeRegion {
  StringRef name;
  unsigned startPoint = 0;
  unsigned endPoint = 0;
  SmallVector<Value, 4> borrowedValues;
};

//===----------------------------------------------------------------------===//
// Lifetime Constraint
//===----------------------------------------------------------------------===//

struct LifetimeConstraint {
  StringRef sub; // Sub-lifetime (must not outlive sup)
  StringRef sup; // Super-lifetime
  Operation *origin = nullptr;
};

//===----------------------------------------------------------------------===//
// Lifetime Pass
//===----------------------------------------------------------------------===//

struct LifetimePass
    : public PassWrapper<LifetimePass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LifetimePass)

  void runOnOperation() override {
    ModuleOp module = getOperation();
    bool hadError = false;

    // Process each function
    module.walk([&](FnOp func) { hadError |= checkFunction(func); });

    if (hadError)
      signalPassFailure();
  }

  bool checkFunction(FnOp func) {
    // Track lifetime regions
    llvm::StringMap<LifetimeRegion> regions;
    SmallVector<LifetimeConstraint, 16> constraints;
    bool hadError = false;

    // Assign program points to operations
    unsigned programPoint = 0;
    llvm::DenseMap<Operation *, unsigned> programPoints;

    func.walk([&](Operation *op) { programPoints[op] = programPoint++; });

    // Collect lifetime regions from borrows
    func.walk([&](BorrowOp borrowOp) {
      auto lifetime = borrowOp.getLifetime();
      auto lifetimeName = lifetime.getName().getValue();

      // Create or get lifetime region
      auto &region = regions[lifetimeName];
      region.name = lifetimeName;

      // Track start point (first borrow)
      unsigned point = programPoints[borrowOp];
      if (region.startPoint == 0 || point < region.startPoint) {
        region.startPoint = point;
      }

      // Track borrowed value
      region.borrowedValues.push_back(borrowOp.getResult());
    });

    // Infer end points by finding last use of each borrowed value
    for (auto &[name, region] : regions) {
      for (auto value : region.borrowedValues) {
        for (auto &use : value.getUses()) {
          Operation *user = use.getOwner();
          unsigned point = programPoints[user];
          if (point > region.endPoint) {
            region.endPoint = point;
          }
        }
      }
    }

    // Add 'static lifetime (entire function)
    auto &staticLifetime = regions["'static"];
    staticLifetime.name = "'static";
    staticLifetime.startPoint = 0;
    staticLifetime.endPoint = programPoint;

    // Collect lifetime constraints
    hadError |= collectConstraints(func, regions, constraints);

    // Solve constraints (check for violations)
    hadError |= solveConstraints(regions, constraints);

    // Annotate operations with lifetime information
    annotateLifetimes(func, regions);

    return hadError;
  }

  bool collectConstraints(FnOp func, llvm::StringMap<LifetimeRegion> &regions,
                          SmallVector<LifetimeConstraint, 16> &constraints) {
    bool hadError = false;

    func.walk([&](Operation *op) {
      // For borrow operations, ensure borrowed value outlives the borrow
      if (auto borrowOp = dyn_cast<BorrowOp>(op)) {
        // Get the borrowed value's lifetime
        Value source = borrowOp.getValue();
        StringRef borrowLifetime = borrowOp.getLifetime().getName().getValue();

        // Find the source value's lifetime
        // Simplified: assume source has 'static lifetime for now
        // Real implementation would track source lifetimes

        // Add constraint: borrow lifetime ⊆ source lifetime
        constraints.push_back({borrowLifetime,
                               "'static", // Simplified
                               borrowOp});
      }
    });

    return hadError;
  }

  bool solveConstraints(llvm::StringMap<LifetimeRegion> &regions,
                        SmallVector<LifetimeConstraint, 16> &constraints) {
    bool hadError = false;

    for (auto &constraint : constraints) {
      auto *subRegion = regions.find(constraint.sub);
      auto *supRegion = regions.find(constraint.sup);

      if (subRegion == regions.end()) {
        constraint.origin->emitError("unknown lifetime: ") << constraint.sub;
        hadError = true;
        continue;
      }

      if (supRegion == regions.end()) {
        constraint.origin->emitError("unknown lifetime: ") << constraint.sup;
        hadError = true;
        continue;
      }

      // Check constraint: sub's end point must not exceed sup's end point
      if (subRegion->second.endPoint > supRegion->second.endPoint) {
        constraint.origin->emitError("lifetime constraint violation")
            << "\n  lifetime " << constraint.sub << " outlives "
            << constraint.sup;
        hadError = true;
      }
    }

    return hadError;
  }

  void annotateLifetimes(FnOp func, llvm::StringMap<LifetimeRegion> &regions) {
    // Annotate borrow operations with computed lifetime info
    func.walk([&](BorrowOp borrowOp) {
      auto lifetimeName = borrowOp.getLifetime().getName().getValue();
      if (auto *region = regions.find(lifetimeName)) {
        // Could add attributes with start/end points for debugging
        // borrowOp->setAttr("lifetime_start", ...);
        // borrowOp->setAttr("lifetime_end", ...);
      }
    });
  }

  StringRef getArgument() const final { return "nova-lifetime"; }

  StringRef getDescription() const final {
    return "Infer and check lifetimes (Non-Lexical Lifetimes)";
  }
};

//===----------------------------------------------------------------------===//
// Pass Creation
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createLifetimePass() {
  return std::make_unique<LifetimePass>();
}

} // namespace nova
} // namespace mlir
