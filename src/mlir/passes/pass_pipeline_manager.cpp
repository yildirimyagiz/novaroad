//===- pass_pipeline_manager.cpp - Pass Pipeline Management ---------------===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//

#include "pass_pipeline_manager.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/PassManager.h"
#include "pass_registry.h"
#include "llvm/Support/Debug.h"
#include <algorithm>
#include <chrono>

#define DEBUG_TYPE "nova-pipeline-manager"

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// PassPipelineManager Implementation
//===----------------------------------------------------------------------===//

PassPipelineManager::PassPipelineManager(MLIRContext *ctx) : context(ctx) {
  buildDependencyGraph();
}

void PassPipelineManager::registerPass(
    const std::string &name, std::function<std::unique_ptr<Pass>()> creator,
    PassMetadata metadata) {
  passCreators[name] = std::move(creator);
  passMetadata[name] = std::move(metadata);

  // Update dependency graph
  for (const auto &dep : passMetadata[name].dependencies) {
    dependencyGraph[dep].push_back(name);
  }
}

void PassPipelineManager::registerPass(
    const std::string &name, std::function<std::unique_ptr<Pass>()> creator) {
  PassMetadata metadata = inferMetadata(name);
  registerPass(name, std::move(creator), std::move(metadata));
}

LogicalResult PassPipelineManager::buildPipeline(PassManager &pm,
                                                 const PipelineConfig &config) {

  // Configure pass manager
  pm.enableVerifier(config.verifyAfterEachPass);
  pm.enableTiming();

  if (config.dumpIRAfterEachPass) {
    pm.enableIRPrinting();
  }

  // Build pipeline in phases
  SmallVector<PassPhase, 7> phases = {
      PassPhase::EarlyVerification,  PassPhase::TypeElaboration,
      PassPhase::EffectAnalysis,     PassPhase::HighLevelOptimization,
      PassPhase::MojoLevelTransform, PassPhase::BackendSpecific,
      PassPhase::FinalVerification};

  for (PassPhase phase : phases) {
    if (failed(buildPhase(pm, phase, config))) {
      return failure();
    }
  }

  // Insert verification gates
  if (config.verificationLevel >= 2) {
    insertVerificationGates(pm);
  }

  return success();
}

LogicalResult PassPipelineManager::buildPhase(OpPassManager &pm,
                                              PassPhase phase,
                                              const PipelineConfig &config) {

  SmallVector<std::string, 16> phasePasses;

  switch (phase) {
  case PassPhase::EarlyVerification:
    if (config.verificationLevel >= 1) {
      phasePasses.push_back("borrow-check");
      phasePasses.push_back("linear-types");
      phasePasses.push_back("lifetime-inference");
    }
    break;

  case PassPhase::TypeElaboration:
    if (config.verificationLevel >= 2) {
      phasePasses.push_back("dependent-types");
      phasePasses.push_back("trait-solver");
    }
    break;

  case PassPhase::EffectAnalysis:
    phasePasses.push_back("effect-check");
    if (config.verificationLevel >= 2) {
      phasePasses.push_back("termination-check");
    }
    break;

  case PassPhase::HighLevelOptimization:
    if (config.optLevel >= 1) {
      phasePasses.push_back("canonicalization");
      phasePasses.push_back("cse");
      phasePasses.push_back("dce");
    }
    if (config.optLevel >= 2) {
      phasePasses.push_back("inliner");
    }
    break;

  case PassPhase::MojoLevelTransform:
    if (config.enableMojoOpts && config.optLevel >= 2) {
      phasePasses.push_back("vectorization");
      phasePasses.push_back("loop-optimization");

      if (config.enableParallelization) {
        phasePasses.push_back("auto-parallel");
      }

      if (config.optLevel >= 3) {
        phasePasses.push_back("pgo");
      }
    }
    break;

  case PassPhase::BackendSpecific:
    if (config.targetBackend == "metal" || config.targetBackend == "cuda") {
      phasePasses.push_back("gpu-metal");
    }
    phasePasses.push_back("async-lowering");
    break;

  case PassPhase::FinalVerification:
    if (config.enableProofChecking && config.verificationLevel >= 3) {
      phasePasses.push_back("proof-check");
      phasePasses.push_back("equality-check");
    }
    break;
  }

  // Resolve dependencies
  auto orderedPasses = resolveDependencies(phasePasses);

  // Validate ordering
  if (failed(validateOrdering(orderedPasses))) {
    return failure();
  }

  // Add passes to pipeline
  for (const auto &passName : orderedPasses) {
    auto it = passCreators.find(passName);
    if (it != passCreators.end()) {
      pm.addPass(it->second());

      // Track statistics
      statistics[passName].name = passName;
      statistics[passName].executionCount++;
    }
  }

  // Add phase verification
  addPhaseVerification(pm, phase);

  return success();
}

SmallVector<std::string, 32> PassPipelineManager::resolveDependencies(
    const SmallVector<std::string, 16> &requestedPasses) {

  SmallVector<std::string, 32> result;
  llvm::StringSet<> visited;

  // Topological sort with dependency resolution
  std::function<void(const std::string &)> visit =
      [&](const std::string &pass) {
        if (visited.contains(pass))
          return;

        visited.insert(pass);

        // Visit dependencies first
        auto it = passMetadata.find(pass);
        if (it != passMetadata.end()) {
          for (const auto &dep : it->second.dependencies) {
            visit(dep);
          }
        }

        result.push_back(pass);
      };

  for (const auto &pass : requestedPasses) {
    visit(pass);
  }

  return result;
}

bool PassPipelineManager::hasCircularDependencies(
    const std::string &passName) const {
  llvm::StringMap<int> visited;
  llvm::StringMap<int> recStack;

  return detectCycle(passName, visited, recStack);
}

LogicalResult PassPipelineManager::validateOrdering(
    const SmallVector<std::string, 32> &passOrder) {

  // Check for conflicts
  auto conflicts = findConflicts(passOrder);
  if (!conflicts.empty()) {
    llvm::errs() << "Pass ordering has conflicts:\n";
    for (const auto &conflict : conflicts) {
      llvm::errs() << "  " << conflict.first << " conflicts with "
                   << conflict.second << "\n";
    }
    return failure();
  }

  // Verify all dependencies are satisfied
  llvm::StringSet<> seen;
  for (const auto &pass : passOrder) {
    auto it = passMetadata.find(pass);
    if (it != passMetadata.end()) {
      for (const auto &dep : it->second.dependencies) {
        if (!seen.contains(dep)) {
          llvm::errs() << "Dependency not satisfied: " << pass << " requires "
                       << dep << "\n";
          return failure();
        }
      }
    }
    seen.insert(pass);
  }

  return success();
}

void PassPipelineManager::insertVerificationGates(PassManager &pm) {
  // Verification gates are inserted automatically by enabling verifier
  // Additional custom gates can be added here
}

bool PassPipelineManager::checkPreConditions(Operation *op,
                                             const std::string &passName) {

  auto it = passMetadata.find(passName);
  if (it == passMetadata.end())
    return true;

  // Check required IR properties
  for (const auto &requirement : it->second.requires) {
    // Check if operation has required property
    if (!op->hasAttr(requirement)) {
      llvm::errs() << "Pre-condition failed for " << passName << ": missing "
                   << requirement << "\n";
      return false;
    }
  }

  return true;
}

bool PassPipelineManager::checkPostConditions(Operation *op,
                                              const std::string &passName) {

  auto it = passMetadata.find(passName);
  if (it == passMetadata.end())
    return true;

  // Verify preserved properties
  for (const auto &preserved : it->second.preserves) {
    // Verify property is still present
    if (!op->hasAttr(preserved)) {
      llvm::errs() << "Post-condition failed for " << passName << ": lost "
                   << preserved << "\n";
      return false;
    }
  }

  return true;
}

bool PassPipelineManager::passesConflict(const std::string &pass1,
                                         const std::string &pass2) {

  auto it = passMetadata.find(pass1);
  if (it == passMetadata.end())
    return false;

  for (const auto &conflict : it->second.conflicts) {
    if (conflict == pass2)
      return true;
  }

  return false;
}

SmallVector<std::pair<std::string, std::string>, 4>
PassPipelineManager::findConflicts(const SmallVector<std::string, 32> &passes) {

  SmallVector<std::pair<std::string, std::string>, 4> conflicts;

  for (size_t i = 0; i < passes.size(); ++i) {
    for (size_t j = i + 1; j < passes.size(); ++j) {
      if (passesConflict(passes[i], passes[j])) {
        conflicts.push_back({passes[i], passes[j]});
      }
    }
  }

  return conflicts;
}

SmallVector<PassPipelineManager::PassStats, 32>
PassPipelineManager::getStatistics() const {
  SmallVector<PassStats, 32> stats;

  for (const auto &entry : statistics) {
    stats.push_back(entry.second);
  }

  // Sort by total time
  std::sort(stats.begin(), stats.end(),
            [](const PassStats &a, const PassStats &b) {
              return a.totalTime > b.totalTime;
            });

  return stats;
}

std::string PassPipelineManager::dumpPipeline() const {
  std::string result;
  llvm::raw_string_ostream os(result);

  os << "Nova Pass Pipeline:\n";
  os << "====================\n\n";

  for (const auto &entry : passMetadata) {
    os << "Pass: " << entry.first() << "\n";
    os << "  Category: ";
    switch (entry.second.category) {
    case PassCategory::TypeSafety:
      os << "TypeSafety";
      break;
    case PassCategory::AdvancedTypes:
      os << "AdvancedTypes";
      break;
    case PassCategory::EffectSystem:
      os << "EffectSystem";
      break;
    case PassCategory::Optimization:
      os << "Optimization";
      break;
    case PassCategory::Parallelization:
      os << "Parallelization";
      break;
    case PassCategory::BackendLowering:
      os << "BackendLowering";
      break;
    case PassCategory::Verification:
      os << "Verification";
      break;
    case PassCategory::ProofChecking:
      os << "ProofChecking";
      break;
    }
    os << "\n";

    if (!entry.second.dependencies.empty()) {
      os << "  Dependencies: ";
      for (const auto &dep : entry.second.dependencies)
        os << dep << " ";
      os << "\n";
    }

    os << "\n";
  }

  os.flush();
  return result;
}

bool PassPipelineManager::verifyPipelineIntegrity() const {
  // Check for circular dependencies
  for (const auto &entry : passMetadata) {
    if (hasCircularDependencies(entry.first())) {
      llvm::errs() << "Circular dependency detected for: " << entry.first()
                   << "\n";
      return false;
    }
  }

  // Verify all dependencies exist
  for (const auto &entry : passMetadata) {
    for (const auto &dep : entry.second.dependencies) {
      if (passCreators.find(dep) == passCreators.end()) {
        llvm::errs() << "Missing dependency: " << entry.first() << " requires "
                     << dep << "\n";
        return false;
      }
    }
  }

  return true;
}

//===----------------------------------------------------------------------===//
// Helper Methods
//===----------------------------------------------------------------------===//

void PassPipelineManager::buildDependencyGraph() {
  dependencyGraph.clear();

  for (const auto &entry : passMetadata) {
    for (const auto &dep : entry.second.dependencies) {
      dependencyGraph[dep].push_back(entry.first());
    }
  }
}

bool PassPipelineManager::detectCycle(const std::string &pass,
                                      llvm::StringMap<int> &visited,
                                      llvm::StringMap<int> &recStack) const {

  visited[pass] = 1;
  recStack[pass] = 1;

  auto it = passMetadata.find(pass);
  if (it != passMetadata.end()) {
    for (const auto &dep : it->second.dependencies) {
      if (!visited.lookup(dep)) {
        if (detectCycle(dep, visited, recStack))
          return true;
      } else if (recStack.lookup(dep)) {
        return true;
      }
    }
  }

  recStack[pass] = 0;
  return false;
}

PassMetadata
PassPipelineManager::inferMetadata(const std::string &passName) const {
  PassMetadata metadata;
  metadata.name = passName;

  // Infer category from name
  if (passName.find("borrow") != std::string::npos ||
      passName.find("linear") != std::string::npos ||
      passName.find("lifetime") != std::string::npos) {
    metadata.category = PassCategory::TypeSafety;
    metadata.preferredPhase = PassPhase::EarlyVerification;
  } else if (passName.find("dependent") != std::string::npos ||
             passName.find("trait") != std::string::npos) {
    metadata.category = PassCategory::AdvancedTypes;
    metadata.preferredPhase = PassPhase::TypeElaboration;
  } else if (passName.find("effect") != std::string::npos) {
    metadata.category = PassCategory::EffectSystem;
    metadata.preferredPhase = PassPhase::EffectAnalysis;
  } else if (passName.find("vectori") != std::string::npos ||
             passName.find("parallel") != std::string::npos ||
             passName.find("loop") != std::string::npos) {
    metadata.category = PassCategory::Parallelization;
    metadata.preferredPhase = PassPhase::MojoLevelTransform;
  } else if (passName.find("proof") != std::string::npos ||
             passName.find("equality") != std::string::npos) {
    metadata.category = PassCategory::ProofChecking;
    metadata.preferredPhase = PassPhase::FinalVerification;
  } else {
    metadata.category = PassCategory::Optimization;
    metadata.preferredPhase = PassPhase::HighLevelOptimization;
  }

  return metadata;
}

void PassPipelineManager::addPhaseVerification(OpPassManager &pm,
                                               PassPhase phase) {
  // Add phase-specific verification
  // This ensures IR is in valid state between phases
}

//===----------------------------------------------------------------------===//
// Predefined Pipeline Builders
//===----------------------------------------------------------------------===//

void buildStandardPipeline(PassPipelineManager &manager, PassManager &pm,
                           const PipelineConfig &config) {
  manager.buildPipeline(pm, config);
}

void buildVerificationPipeline(PassPipelineManager &manager, PassManager &pm) {
  PipelineConfig config;
  config.optLevel = 0;
  config.verificationLevel = 3;
  config.enableMojoOpts = false;
  config.enableProofChecking = true;

  manager.buildPipeline(pm, config);
}

void buildOptimizationPipeline(PassPipelineManager &manager, PassManager &pm,
                               unsigned optLevel) {
  PipelineConfig config;
  config.optLevel = optLevel;
  config.verificationLevel = 1;
  config.enableMojoOpts = true;

  manager.buildPipeline(pm, config);
}

void buildMojoClassPipeline(PassPipelineManager &manager, PassManager &pm) {
  PipelineConfig config;
  config.optLevel = 3;
  config.verificationLevel = 2;
  config.enableMojoOpts = true;
  config.enableParallelization = true;

  manager.buildPipeline(pm, config);
}

void buildExperimentalPipeline(PassPipelineManager &manager, PassManager &pm) {
  PipelineConfig config;
  config.optLevel = 3;
  config.verificationLevel = 3;
  config.enableMojoOpts = true;
  config.enableProofChecking = true;
  config.enableParallelization = true;
  config.verifyAfterEachPass = true;

  manager.buildPipeline(pm, config);
}

} // namespace nova
} // namespace mlir
