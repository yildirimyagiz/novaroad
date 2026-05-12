# Nova Analysis Infrastructure

add_mlir_library(NovaAnalysis
  borrow_analysis.cpp
  lifetime_analysis.cpp
  effect_analysis.cpp
  linearity_analysis.cpp
  constraint_solver.cpp
  unification_solver.cpp
  
  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/compiler
  ${PROJECT_BINARY_DIR}/compiler/ir
  
  DEPENDS
  NovaDialect
  
  LINK_LIBS PUBLIC
  NovaDialect
  MLIRIR
  MLIRAnalysis
  MLIRSupport
)

set(NOVA_ANALYSIS_LIBS NovaAnalysis PARENT_SCOPE)
