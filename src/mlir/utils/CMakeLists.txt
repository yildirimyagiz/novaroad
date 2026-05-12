# Nova MLIR Utilities

add_library(NovaMLIRUtils
  pass_helpers.cpp
  ir_helpers.cpp
  type_helpers.cpp
)

target_include_directories(NovaMLIRUtils PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${PROJECT_SOURCE_DIR}/compiler
)

target_link_libraries(NovaMLIRUtils PUBLIC
  NovaPasses
  MLIRIR
  MLIRPass
  MLIRSupport
  MLIRTransforms
)
