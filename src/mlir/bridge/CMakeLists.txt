# MLIR Bridge - Gödel and HIR to MLIR lowering

add_library(nova_mlir_bridge
  godel_mlir_bridge.cpp
  hir_to_mlir.cpp
)

target_include_directories(nova_mlir_bridge PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../
  ${MLIR_INCLUDE_DIRS}
  ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(nova_mlir_bridge
  MLIRIR
  MLIRFuncDialect
  MLIRArithDialect
  MLIRSCFDialect
  MLIRMemRefDialect
  MLIRVectorDialect
  MLIRSupport
)

# Test executable
add_executable(test_mlir_bridge
  test_bridge.cpp
)

target_link_libraries(test_mlir_bridge
  nova_mlir_bridge
  MLIRIR
  MLIRFuncDialect
  MLIRArithDialect
  MLIRParser
  MLIRSupport
)

# Install targets
install(TARGETS nova_mlir_bridge
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(FILES
  godel_mlir_bridge.h
  hir_to_mlir.h
  DESTINATION include/mlir/bridge
)
