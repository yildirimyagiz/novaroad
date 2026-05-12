//===- pass_pipeline_manager.h - Pass Pipeline Management -------*- C++ -*-===//
//
// Part of the Nova Project
//
//===----------------------------------------------------------------------===//
//
// This file defines the Pass Pipeline Manager for Nova, providing:
// - Deterministic pass ordering
// - Pass dependency resolution
// - Legality verification gates
// - Pass interaction chaos prevention
//
// This is the critical stability layer for production-grade compilation.
//
//===----------------------------------------------------------------------===//

#ifndef NOVA_MLIR_PASSES_PASS_PIPELINE_MANAGER_H
#define NOVA_MLIR_PASSES_PASS_PIPELINE_MANAGER_H

#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include <functional>
#include <memory>

namespace mlir {
namespace nova {

//===----------------------------------------------------------------------===//
// Pass Categories and Phases
//===----------------------------------------------------------------------===//

enum class PassPhase {
  // Early verification (ownership, linearity, lifetimes)
  EarlyVerification,

  // Type system elaboration (dependent types, traits)
  TypeElaboration,

  // Effect analysis and checking
  EffectAnalysis,

  // High-level optimizations (canonicalization, CSE)
  HighLevelOptimization,

  // Mojo-level transformations (vectorization, parallelization)
  MojoLevelTransform,

  // Backend-specific (GPU, Metal, CPU)
  BackendSpecific,

  // Final verification
  FinalVerification
};

enum class PassCategory {
  TypeSafety,
  AdvancedTypes,
  EffectSystem,
  Optimization,
  Parallelization,
  BackendLowering,
  Verification,
  ProofChecking
};

//===----------------------------------------------------------------------===//
// Pass Metadata and Dependencies
//===----------------------------------------------------------------------===//

struct PassMetadata {
  std::string name;
  PassCategory category;
  PassPhase preferredPhase;

  // Dependencies: passes that must run before this one
  SmallVector<std::string, 4> dependencies;

  // Conflicts: passes that cannot run together
  SmallVector<std::string, 2> conflicts;

  // Requirements: IR properties this pass requires
  SmallVector<std::string, 2>
    requires;

  // Preserves: IR properties this pass preserves
  SmallVector<std::string, 4> preserves;

  // Invalidates: IR properties this pass invalidates
  SmallVector<std::string, 2> invalidates;

  // Cost estimate (for scheduling)
  unsigned estimatedCost = 100;

  // Whether this pass is mandatory
  bool isMandatory = false;
};

//===----------------------------------------------------------------------===//
// Pipeline Configuration
//===----------------------------------------------------------------------===//

struct PipelineConfig {
  // Optimization level (0-3, similar to -O0 to -O3)
  unsigned optLevel = 2;

  // Verification level (0-3)
  unsigned verificationLevel = 2;

  // Enable Mojo-level optimizations
  bool enableMojoOpts = true;

  // Enable proof checking
  bool enableProofChecking = true;

  // Enable parallel passes
  bool enableParallelization = true;

  // Target backend (cpu, metal, cuda)
  std::string targetBackend = "cpu";

  // Debug: dump IR after each pass
  bool dumpIRAfterEachPass = false;

  // Debug: verify IR after each pass
  bool verifyAfterEachPass = true;

  // Maximum compilation time (seconds, 0 = unlimited)
  unsigned maxCompileTime = 0;
};

//===----------------------------------------------------------------------===//
// Pass Pipeline Manager
//===----------------------------------------------------------------------===//

class PassPipelineManager {
public:
  explicit PassPipelineManager(MLIRContext *ctx);

  //===--------------------------------------------------------------------===//
  // Pass Registration
  //===--------------------------------------------------------------------===//

  /// Register a pass with metadata
  void registerPass(const std::string &name,
                    std::function<std::unique_ptr<Pass>()> creator,
                    PassMetadata metadata);

  /// Register a pass with default metadata (inferred from pass itself)
  void registerPass(const std::string &name,
                    std::function<std::unique_ptr<Pass>()> creator);

  //===--------------------------------------------------------------------===//
  // Pipeline Construction
  //===--------------------------------------------------------------------===//

  /// Build the complete compilation pipeline
  LogicalResult buildPipeline(PassManager &pm, const PipelineConfig &config);

  /// Build a specific phase pipeline
  LogicalResult buildPhase(OpPassManager &pm, PassPhase phase,
                           const PipelineConfig &config);

  /// Add a custom pass to the pipeline
  void addPass(PassManager &pm, const std::string &passName);

  //===--------------------------------------------------------------------===//
  // Dependency Resolution
  //===--------------------------------------------------------------------===//

  /// Resolve pass dependencies and return ordered pass list
  SmallVector<std::string, 32>
  resolveDependencies(const SmallVector<std::string, 16> &requestedPasses);

  /// Check for circular dependencies
  bool hasCircularDependencies(const std::string &passName) const;

  /// Validate pass ordering
  LogicalResult validateOrdering(const SmallVector<std::string, 32> &passOrder);

  //===--------------------------------------------------------------------===//
  // Legality Gates
  //===--------------------------------------------------------------------===//

  /// Insert verification gates between passes
  void insertVerificationGates(PassManager &pm);

  /// Check IR legality before pass
  bool checkPreConditions(Operation *op, const std::string &passName);

  /// Check IR legality after pass
  bool checkPostConditions(Operation *op, const std::string &passName);

  //===--------------------------------------------------------------------===//
  // Conflict Detection
  //===--------------------------------------------------------------------===//

  /// Check if two passes conflict
  bool passesConflict(const std::string &pass1, const std::string &pass2);

  /// Find all conflicts in a pass list
  SmallVector<std::pair<std::string, std::string>, 4>
  findConflicts(const SmallVector<std::string, 32> &passes);

  //===--------------------------------------------------------------------===//
  // Statistics and Diagnostics
  //===--------------------------------------------------------------------===//

  /// Get pass execution statistics
  struct PassStats {
    std::string name;
    unsigned executionCount = 0;
    double totalTime = 0.0;
    double avgTime = 0.0;
    bool succeeded = true;
  };

  SmallVector<PassStats, 32> getStatistics() const;

  /// Dump pipeline to string (for debugging)
  std::string dumpPipeline() const;

  /// Verify pipeline integrity
  bool verifyPipelineIntegrity() const;

private:
  MLIRContext *context;

  // Pass creators and metadata
  llvm::StringMap<std::function<std::unique_ptr<Pass>()>> passCreators;
  llvm::StringMap<PassMetadata> passMetadata;

  // Dependency graph
  llvm::StringMap<SmallVector<std::string, 4>> dependencyGraph;

  // Statistics tracking
  mutable llvm::StringMap<PassStats> statistics;

  // Helper methods
  void buildDependencyGraph();
  SmallVector<std::string, 32>
  topologicalSort(const SmallVector<std::string, 16> &passes);
  bool detectCycle(const std::string &pass, llvm::StringMap<int> &visited,
                   llvm::StringMap<int> &recStack) const;

  PassMetadata inferMetadata(const std::string &passName) const;
  void addPhaseVerification(OpPassManager &pm, PassPhase phase);
};

//===----------------------------------------------------------------------===//
// Predefined Pipeline Builders
//===----------------------------------------------------------------------===//

/// Build the standard Nova compilation pipeline
void buildStandardPipeline(PassPipelineManager &manager, PassManager &pm,
                           const PipelineConfig &config);

/// Build verification-only pipeline
void buildVerificationPipeline(PassPipelineManager &manager, PassManager &pm);

/// Build optimization-only pipeline (no verification)
void buildOptimizationPipeline(PassPipelineManager &manager, PassManager &pm,
                               unsigned optLevel);

/// Build Mojo-class performance pipeline
void buildMojoClassPipeline(PassPipelineManager &manager, PassManager &pm);

/// Build research/experimental pipeline (all passes)
void buildExperimentalPipeline(PassPipelineManager &manager, PassManager &pm);

} // namespace nova
} // namespace mlir

#endif // NOVA_MLIR_PASSES_PASS_PIPELINE_MANAGER_H
