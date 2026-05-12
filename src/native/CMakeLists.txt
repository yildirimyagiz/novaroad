# ═══════════════════════════════════════════════════════════════════════════
# Nova Formal Verification Layer - CMake Build Script
# ═══════════════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.15)
project(NovaFormalVerification VERSION 1.0.0 LANGUAGES C)

# ═══════════════════════════════════════════════════════════════════════════
# Build Configuration
# ═══════════════════════════════════════════════════════════════════════════

option(NOVA_FORMAL_DISABLED "Disable all formal checks (0% overhead)" OFF)
option(NOVA_FORMAL_FAST "Enable fast runtime checks (<1% overhead)" ON)
option(NOVA_FORMAL_RESEARCH "Enable detailed logging (5% overhead)" OFF)
option(NOVA_FORMAL_STRICT "Enable strict mode (build-time only)" OFF)

option(NOVA_RUN_FORMAL_VERIFICATION "Run proof tools at build time" ON)
option(NOVA_ENABLE_ISABELLE "Enable Isabelle/HOL verification" ON)
option(NOVA_ENABLE_KLEE "Enable KLEE symbolic execution" ON)
option(NOVA_ENABLE_CVC5 "Enable CVC5 SMT solver" ON)
option(NOVA_FULL_BUILD "Full build: backend dispatch + autocal GPU calibration (stub backends if no SDK)" OFF)

# ═══════════════════════════════════════════════════════════════════════════
# Compiler Flags
# ═══════════════════════════════════════════════════════════════════════════

if(NOVA_FORMAL_DISABLED)
  add_compile_definitions(NOVA_USE_FORMAL_DISABLED)
  message(STATUS "[Gödel] Formal verification: DISABLED (0% overhead)")

elseif(NOVA_FORMAL_STRICT)
  add_compile_definitions(NOVA_USE_FORMAL_STRICT)
  message(STATUS "[Gödel] Formal verification: STRICT (abort on missing proofs)")

elseif(NOVA_FORMAL_RESEARCH)
  add_compile_definitions(NOVA_USE_FORMAL_RESEARCH)
  message(STATUS "[Gödel] Formal verification: RESEARCH (detailed logging)")

elseif(NOVA_FORMAL_FAST)
  add_compile_definitions(NOVA_USE_FORMAL_FAST)
  message(STATUS "[Gödel] Formal verification: FAST (<1% overhead)")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Formal Verification Pipeline
# ═══════════════════════════════════════════════════════════════════════════

set(PROOF_MANIFEST_FILE "${CMAKE_BINARY_DIR}/nova_proof_manifest_generated.c")

if(NOVA_RUN_FORMAL_VERIFICATION)
  message(STATUS "[Gödel] Formal verification pipeline: ENABLED")
  
  # Custom target to run proof generation
  add_custom_command(
    OUTPUT ${PROOF_MANIFEST_FILE}
    COMMAND python3 ${CMAKE_SOURCE_DIR}/../scripts/generate_proof_manifest.py
            --theory-dir ${CMAKE_SOURCE_DIR}/../theories
            --test-dir ${CMAKE_SOURCE_DIR}/../tests/klee
            --smt-dir ${CMAKE_SOURCE_DIR}/../smt
            --output ${PROOF_MANIFEST_FILE}
    DEPENDS
      ${CMAKE_SOURCE_DIR}/../scripts/generate_proof_manifest.py
      # Add theory files as dependencies (if they exist)
      # ${CMAKE_SOURCE_DIR}/theories/Nova_Tensor_Safety.thy
      # ${CMAKE_SOURCE_DIR}/tests/klee/memory_safety_test.c
      # ${CMAKE_SOURCE_DIR}/smt/optimizer_stability.smt2
    COMMENT "[Gödel] Running formal verification tools..."
    VERBATIM
  )
  
  add_custom_target(nova_formal_verification
    DEPENDS ${PROOF_MANIFEST_FILE}
  )
  
else()
  message(STATUS "[Gödel] Formal verification pipeline: SKIPPED")
  
  # Generate stub manifest
  add_custom_command(
    OUTPUT ${PROOF_MANIFEST_FILE}
    COMMAND python3 ${CMAKE_SOURCE_DIR}/../scripts/generate_proof_manifest.py
            --skip-verification
            --output ${PROOF_MANIFEST_FILE}
    DEPENDS ${CMAKE_SOURCE_DIR}/../scripts/generate_proof_manifest.py
    COMMENT "[Gödel] Generating stub proof manifest (no verification)..."
    VERBATIM
  )
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Platform-specific sources (Metal on Apple, stub elsewhere)
# ═══════════════════════════════════════════════════════════════════════════
if(APPLE)
  set(NOVA_METAL_SRC src/backends/metal/nova_metal_gpu.c)
  set_source_files_properties(src/backends/metal/nova_metal_gpu.c PROPERTIES COMPILE_OPTIONS "-x;objective-c")
else()
  set(NOVA_METAL_SRC src/backends/metal/nova_metal_stub.c)
endif()

# Backend dispatch + CPU fallback: always in build (stubs when SDK missing)
set(NOVA_DISPATCH_SRCS
  src/backends/nova_backend_dispatch.c
  src/backends/cpu/nova_cpu_backend.c
  src/backends/cuda/nova_cuda_stub.c
  src/backends/rocm/nova_rocm_stub.c
  src/backends/vulkan/nova_vulkan_stub.c
  src/backends/opencl/nova_opencl_stub.c
)
# Full build: enable autocal GPU calibration (nova_autocal_run calibrates all backends)
if(NOVA_FULL_BUILD)
  add_compile_definitions(NOVA_AUTOCAL_USE_DISPATCH)
  message(STATUS "[Nova] FULL_BUILD: autocal GPU calibration enabled")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Library Target
# ═══════════════════════════════════════════════════════════════════════════

add_library(nova SHARED
  src/ai/core/nova_tensor.c
  src/ai/core/nova_tensor_ops.c
  src/compute/graph/nova_graph.c
  src/nova_context.c
  src/memory/nova_allocator.c
  src/quantization/nova_quantization.c
  src/sparse/nova_sparse.c
  src/formal/nova_proof_cache.c
  src/formal/nova_invariant_validator.c
  src/formal/nova_kernel_verifier.c
  src/formal/nova_optimizer_verifier.c
  src/formal/nova_kernel_contracts.c
  src/formal/nova_obligation.c
  src/formal/nova_obligation_v2.c
  src/compute/graph/nova_graph_obligations.c
  src/compute/graph/nova_graph_runtime.c
  src/compiler/lowering/nova_lower_graph.c
  src/backends/cpu/nova_kernels_cpu.c
  ${NOVA_METAL_SRC}
  ${NOVA_DISPATCH_SRCS}
  src/optimizer/nova_autotune.c
  src/autocal/nova_autocal.c
  src/optimizer/nova_advanced_optimizations.c
  src/optimizer/nova_advanced_optimizations.c
  src/optimizer/nova_profile_enforcer.c
  src/optimizer/nova_kernel_fusion.c
  src/compute/nova_mirror.c
  src/compute/nova_mirror_ml.c
  src/compute/nova_mirror_v2.c
  src/compute/nova_bridge.c
  src/compute/nova_crypto.c
  src/monitoring/nova_monitor.c
  src/monitoring/nova_health.c
  src/cluster/nova_cluster.c
  src/cluster/nova_economy.c
  src/compute/nova_shield.c
  src/compute/nova_shard.c
  src/compute/nova_jit.c
  ${PROOF_MANIFEST_FILE}
)

target_include_directories(nova
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends
)

target_compile_options(nova PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  $<$<CONFIG:Debug>:-O0 -g3>
  $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# Link with math library
target_link_libraries(nova PRIVATE m)
if(APPLE)
  target_link_libraries(nova PRIVATE "-framework Foundation" "-framework Metal")
endif()

# ═══════════════════════════════════════════════════════════════════════════
# Example Test Program
# ═══════════════════════════════════════════════════════════════════════════

add_executable(nova_formal_test
  src/tools/test_formal_verification.c
)

target_link_libraries(nova_formal_test PRIVATE
  nova
)

add_executable(nova_formal_cli
  src/tools/nova_formal_cli.c
  src/formal/isabelle/nova_isabelle_theory_export.c
  src/formal/smt/nova_cvc5_bridge.c
  src/formal/smt/nova_smtlib2_writer.c
)

target_link_libraries(nova_formal_cli PRIVATE
  nova
)

# ═══════════════════════════════════════════════════════════════════════════
# Fusion Benchmark
# ═══════════════════════════════════════════════════════════════════════════

add_executable(nova_bench_fusion
  src/tools/bench_fusion.c
)

target_link_libraries(nova_bench_fusion PRIVATE
  nova
)

target_compile_options(nova_bench_fusion PRIVATE -O3)

# ═══════════════════════════════════════════════════════════════════════════
# Delta Benchmark
# ═══════════════════════════════════════════════════════════════════════════

add_executable(nova_bench_delta
  src/tools/bench_delta.c
)

target_link_libraries(nova_bench_delta PRIVATE
  nova
)

target_compile_options(nova_bench_delta PRIVATE -O3)

# ═══════════════════════════════════════════════════════════════════════════
# Mirror Benchmark
# ═══════════════════════════════════════════════════════════════════════════

add_executable(nova_bench_mirror
  src/tools/bench_mirror.c
)

target_link_libraries(nova_bench_mirror PRIVATE
  nova
)

target_compile_options(nova_bench_mirror PRIVATE -O3)

# ═══════════════════════════════════════════════════════════════════════════
# Mirror ML Pipeline Benchmark
# ═══════════════════════════════════════════════════════════════════════════

add_executable(nova_bench_mirror_ml
  src/tools/bench_mirror_ml.c
)

target_link_libraries(nova_bench_mirror_ml PRIVATE
  nova
)

target_compile_options(nova_bench_mirror_ml PRIVATE -O3)

# ═══════════════════════════════════════════════════════════════════════════
# Installation
# ═══════════════════════════════════════════════════════════════════════════

install(TARGETS nova
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(FILES
  nova_proof_manifest.h
  nova_proof_cache.h
  DESTINATION include/formal
)

# ═══════════════════════════════════════════════════════════════════════════
# Summary
# ═══════════════════════════════════════════════════════════════════════════

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "Nova Formal Verification Configuration")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "Mode:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Verification:   ${NOVA_RUN_FORMAL_VERIFICATION}")
message(STATUS "Isabelle:       ${NOVA_ENABLE_ISABELLE}")
message(STATUS "KLEE:           ${NOVA_ENABLE_KLEE}")
message(STATUS "CVC5:           ${NOVA_ENABLE_CVC5}")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "")
add_executable(nova_bench_mirror_v2 src/tools/bench_mirror_v2.c)
target_link_libraries(nova_bench_mirror_v2 nova)
add_executable(nova_bench_crypto src/tools/bench_crypto.c)
target_link_libraries(nova_bench_crypto nova)
add_executable(nova_bench_bridge src/tools/bench_bridge.c)
target_link_libraries(nova_bench_bridge nova)
add_executable(nova_bench_monitor src/tools/bench_monitor.c)
target_link_libraries(nova_bench_monitor nova)
add_executable(nova_bench_cluster src/tools/bench_cluster.c)
target_link_libraries(nova_bench_cluster nova)
add_executable(nova_sync_demo src/tools/nova_sync_demo.c)
target_link_libraries(nova_sync_demo nova)
add_executable(nova_dms_demo src/tools/nova_dms_demo.c)
target_link_libraries(nova_dms_demo nova)
add_executable(nova_integrity_test src/tools/nova_integrity_test.c)
target_link_libraries(nova_integrity_test nova)
add_executable(nova_balancer_demo src/tools/nova_balancer_demo.c)
target_link_libraries(nova_balancer_demo nova)
add_executable(nova_cdn_demo src/tools/nova_cdn_demo.c)
target_link_libraries(nova_cdn_demo nova)
add_executable(nova_gossip_demo src/tools/nova_gossip_demo.c)
target_link_libraries(nova_gossip_demo nova)
add_executable(nova_jit_demo src/tools/nova_jit_demo.c)
target_link_libraries(nova_jit_demo nova)
add_executable(nova_economy_demo src/tools/nova_economy_demo.c)
target_link_libraries(nova_economy_demo nova)
