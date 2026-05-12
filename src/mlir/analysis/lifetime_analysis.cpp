//===- lifetime_analysis.cpp - Lifetime Analysis Implementation -----------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "lifetime_analysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Dominance.h"
#include "llvm/ADT/DenseSet.h"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// LifetimeAnalysis Implementation
//===----------------------------------------------------------------------===//

LifetimeAnalysis::LifetimeAnalysis(Operation *root) {
  assignProgramPoints(root);
  inferRegions(root);
  solveConstraints();
}

const LifetimeRegion *LifetimeAnalysis::getRegion(StringRef lifetime) const {
  auto it = regions.find(lifetime);
  if (it != regions.end())
    return &it->second;
  return nullptr;
}

bool LifetimeAnalysis::outlives(StringRef lifetime1,
                                StringRef lifetime2) const {
  // Special case: 'static outlives everything
  if (lifetime1 == "'static")
    return true;
  if (lifetime2 == "'static")
    return false;

  const LifetimeRegion *region1 = getRegion(lifetime1);
  const LifetimeRegion *region2 = getRegion(lifetime2);

  if (!region1 || !region2)
    return false;

  // lifetime1 outlives lifetime2 if:
  // - region1 starts before or at region2's start
  // - region1 ends after or at region2's end
  return region1->startPoint <= region2->startPoint &&
         region1->endPoint >= region2->endPoint;
}

unsigned LifetimeAnalysis::getProgramPoint(Operation *op) const {
  auto it = programPoints.find(op);
  if (it != programPoints.end())
    return it->second;
  return 0;
}

bool LifetimeAnalysis::checkConstraints() const {
  for (const auto &constraint : constraints) {
    // Check that 'sub' does not outlive 'sup'
    if (outlives(constraint.sub, constraint.sup)) {
      return false; // Constraint violated
    }
  }
  return true;
}

SmallVector<LifetimeConstraint, 8> LifetimeAnalysis::getViolations() const {
  SmallVector<LifetimeConstraint, 8> violations;

  for (const auto &constraint : constraints) {
    if (outlives(constraint.sub, constraint.sup)) {
      violations.push_back(constraint);
    }
  }

  return violations;
}

void LifetimeAnalysis::analyze(Operation *op) {
  // Recursively analyze all operations
  op->walk([&](Operation *innerOp) {
    // Check for lifetime attributes
    if (auto lifetimeAttr = innerOp->getAttrOfType<StringAttr>("lifetime")) {
      StringRef lifetime = lifetimeAttr.getValue();

      // Update region with this operation's program point
      unsigned point = getProgramPoint(innerOp);
      auto &region = regions[lifetime];
      region.name = lifetime;

      if (region.startPoint == 0 || point < region.startPoint)
        region.startPoint = point;
      if (point > region.endPoint)
        region.endPoint = point;
    }

    // Check for borrow operations
    if (innerOp->getName().getStringRef().contains("borrow")) {
      // Extract lifetime constraints from borrow
      if (auto borrowedLifetime =
              innerOp->getAttrOfType<StringAttr>("borrowed_lifetime")) {
        if (auto ownerLifetime =
                innerOp->getAttrOfType<StringAttr>("owner_lifetime")) {
          // Borrowed lifetime must not outlive owner
          LifetimeConstraint constraint;
          constraint.sub = borrowedLifetime.getValue();
          constraint.sup = ownerLifetime.getValue();
          constraint.origin = innerOp;
          constraints.push_back(constraint);
        }
      }
    }
  });
}

void LifetimeAnalysis::assignProgramPoints(Operation *op) {
  unsigned counter = 1;

  // Assign increasing program points in execution order
  op->walk([&](Operation *innerOp) { programPoints[innerOp] = counter++; });
}

void LifetimeAnalysis::inferRegions(Operation *op) {
  // Create a special 'static lifetime that covers everything
  auto &staticRegion = regions["'static"];
  staticRegion.name = "'static";
  staticRegion.startPoint = 0;
  staticRegion.endPoint = UINT_MAX;

  // Analyze operation to build regions
  analyze(op);

  // For each value, infer its lifetime region
  op->walk([&](Operation *innerOp) {
    for (Value result : innerOp->getResults()) {
      // Try to infer lifetime from type
      Type type = result.getType();

      // Check if type has a lifetime annotation
      // For now, assign to a region based on dominance
      unsigned defPoint = getProgramPoint(innerOp);
      unsigned lastUsePoint = defPoint;

      // Find last use
      for (Operation *user : result.getUsers()) {
        unsigned usePoint = getProgramPoint(user);
        if (usePoint > lastUsePoint)
          lastUsePoint = usePoint;
      }

      // Create or update a region for this value's lifetime
      std::string lifetimeName = "'r" + std::to_string(defPoint);
      auto &region = regions[lifetimeName];
      region.name = lifetimeName;
      region.startPoint = defPoint;
      region.endPoint = lastUsePoint;
      region.values.push_back(result);
    }
  });
}

void LifetimeAnalysis::solveConstraints() {
  // Fixed-point iteration to propagate lifetime constraints
  bool changed = true;
  unsigned maxIterations = 100;
  unsigned iteration = 0;

  while (changed && iteration < maxIterations) {
    changed = false;
    iteration++;

    for (const auto &constraint : constraints) {
      auto *subRegion = regions.find(constraint.sub);
      auto *supRegion = regions.find(constraint.sup);

      if (subRegion != regions.end() && supRegion != regions.end()) {
        // Ensure sub region doesn't outlive sup region
        // Shrink sub region if necessary
        if (subRegion->second.startPoint < supRegion->second.startPoint) {
          subRegion->second.startPoint = supRegion->second.startPoint;
          changed = true;
        }
        if (subRegion->second.endPoint > supRegion->second.endPoint) {
          subRegion->second.endPoint = supRegion->second.endPoint;
          changed = true;
        }
      }
    }
  }
}

} // namespace nova
} // namespace mlir
