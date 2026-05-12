# Nova Codegen - MLIR Lowering

add_mlir_library(NovaCodegen
  mlir_lowering.cpp
  llvm_lowering.cpp
  
  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/compiler
  ${PROJECT_BINARY_DIR}/compiler/ir
  
  DEPENDS
  NovaDialect
  
  LINK_LIBS PUBLIC
  NovaDialect
  MLIRIR
  MLIRLLVMDialect
  MLIRFuncDialect
  MLIRMemRefDialect
  MLIRArithDialect
  MLIRSCFDialect
  MLIRTransforms
  MLIRTargetLLVMIRExport
)
