# Nova MLIR Passes - Type System & Semantics

add_mlir_library(NovaPasses
  borrow_check_pass.cpp
  linear_types_pass.cpp
  effect_check_pass.cpp
  lifetime_pass.cpp
  advanced_type_passes.cpp
  proof_check_pass.cpp
  pass_registry.cpp
  pass_pipeline_manager.cpp
  
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
)
