# Nova MLIR Dialect Implementation

add_mlir_dialect_library(NovaDialect
  nova_dialect.cpp
  nova_ops.cpp
  nova_types.cpp
  nova_attrs.cpp
  
  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/compiler
  ${PROJECT_BINARY_DIR}/compiler/ir
  
  DEPENDS
  NovaDialectIncGen
  NovaTypesIncGen
  NovaAttrsIncGen
  NovaOpsIncGen
  
  LINK_LIBS PUBLIC
  MLIRIR
  MLIRSupport
  MLIRCallInterfaces
  MLIRControlFlowInterfaces
  MLIRInferTypeOpInterface
  MLIRSideEffectInterfaces
  MLIRFuncDialect
)

# Export for other subdirectories
set(NOVA_DIALECT_LIBS NovaDialect PARENT_SCOPE)
