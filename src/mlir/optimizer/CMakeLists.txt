# Nova MLIR Optimization Passes

add_mlir_library(NovaOptimizer
  canonicalization_pass.cpp
  dce_pass.cpp
  cse_pass.cpp
  inliner_pass.cpp
  vectorization_pass.cpp
  gpu_metal_pass.cpp
  async_pass.cpp
  loop_optimization_pass.cpp
  auto_parallel_pass.cpp
  pgo_pass.cpp
  
  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/compiler
  ${PROJECT_BINARY_DIR}/compiler/ir
  
  DEPENDS
  NovaDialect
  NovaAnalysis
  
  LINK_LIBS PUBLIC
  NovaDialect
  NovaAnalysis
  MLIRIR
  MLIRPass
  MLIRSupport
  MLIRTransforms
  MLIRTransformUtils
)
