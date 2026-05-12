cmake_minimum_required(VERSION 3.16)

# =============================================================================
# Nova ML Native Library
# src/native/ml/ — C/C++ native ML implementations
# =============================================================================

project(nova_ml_native C)

# --- Source files (tested and verified: 11 files, 0 errors) ---
set(NOVA_ML_SOURCES
    # AI Core — tested
    ai/core/nova_metrics.c
    ai/core/nova_rl.c

    # Optimizer / Tree / Ensemble — tested
    optimizer/nova_ensemble.c
    optimizer/nova_tree.c
    optimizer/nova_genetic.c
    optimizer/nova_physics_opt.c

    # MATLAB Engine Bridge (stub mode — no MATLAB required)
    ai/matlab/nova_matlab_engine.c
    ai/matlab/nova_matlab_value.c
    ai/matlab/nova_engine_session.c
    ai/matlab/nova_engine_helper.c
    ai/matlab/nova_feval_future.c
)

# --- Extended sources (require additional headers — opt-in) ---
# To enable: cmake .. -DNOVA_ML_EXTENDED=ON
if(NOVA_ML_EXTENDED)
    list(APPEND NOVA_ML_SOURCES
        ai/core/nova_tensor.c
        ai/core/nova_tensor_ops.c
        ai/core/nova_tensor_layout.c
        ai/core/nova_tensor_sanity.c
        ai/core/nova_nn.c
        ai/kernels/nova_kernels.c
        optimizer/nova_adaptive_optimizer.c
        optimizer/nova_learning_runtime.c
        optimizer/nova_autotune_v2.c
        optimizer/nova_kernel_fusion.c
        tests/nova_extended_stubs.c
    )
endif()

# --- Include directories ---
set(NOVA_ML_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/ai/core
    ${CMAKE_CURRENT_SOURCE_DIR}/ai/matlab
    ${CMAKE_CURRENT_SOURCE_DIR}/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/optimizer/fusion
    ${CMAKE_CURRENT_SOURCE_DIR}/memory
    ${CMAKE_CURRENT_SOURCE_DIR}/compute
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
    # Root include/ for nova_tensor.h, nova_allocator.h, nova_context.h etc.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/include
)

# --- Static library: nova_ml ---
add_library(nova_ml STATIC ${NOVA_ML_SOURCES})
target_include_directories(nova_ml PUBLIC ${NOVA_ML_INCLUDES})
target_compile_options(nova_ml PRIVATE -Wall -Wextra -std=c11)
target_link_libraries(nova_ml PUBLIC m pthread)

# Optional: MATLAB native mode (requires MATLAB installation)
if(DEFINED ENV{MATLAB_ROOT})
    target_compile_definitions(nova_ml PRIVATE NOVA_MATLAB_NATIVE)
    target_include_directories(nova_ml PRIVATE
        $ENV{MATLAB_ROOT}/extern/include)
    target_link_directories(nova_ml PRIVATE
        $ENV{MATLAB_ROOT}/bin/maci64
        $ENV{MATLAB_ROOT}/bin/glnxa64)
    target_link_libraries(nova_ml PRIVATE eng mx)
    message(STATUS "Nova ML: MATLAB native mode enabled ($ENV{MATLAB_ROOT})")
else()
    message(STATUS "Nova ML: MATLAB stub mode (set MATLAB_ROOT for native)")
endif()

# --- Shared library: nova_ml_shared ---
add_library(nova_ml_shared SHARED ${NOVA_ML_SOURCES})
target_include_directories(nova_ml_shared PUBLIC ${NOVA_ML_INCLUDES})
target_compile_options(nova_ml_shared PRIVATE -Wall -Wextra -std=c11)
target_link_libraries(nova_ml_shared PUBLIC m pthread)
set_target_properties(nova_ml_shared PROPERTIES OUTPUT_NAME "nova_ml")

# Optional: MATLAB native mode for shared library
if(DEFINED ENV{MATLAB_ROOT})
    target_compile_definitions(nova_ml_shared PRIVATE NOVA_MATLAB_NATIVE)
    target_include_directories(nova_ml_shared PRIVATE
        $ENV{MATLAB_ROOT}/extern/include)
    target_link_directories(nova_ml_shared PRIVATE
        $ENV{MATLAB_ROOT}/bin/maci64
        $ENV{MATLAB_ROOT}/bin/glnxa64)
    target_link_libraries(nova_ml_shared PRIVATE eng mx)
endif()

# --- Tests (optional) ---
if(NOVA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Install ---
install(TARGETS nova_ml nova_ml_shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(FILES
    ai/core/nova_metrics.h
    ai/core/nova_rl.h
    optimizer/nova_ensemble.h
    optimizer/nova_tree.h
    optimizer/nova_genetic.h
    optimizer/nova_physics_opt.h
    ai/matlab/nova_matlab_engine.h
    DESTINATION include/ml)

message(STATUS "Nova ML Native: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Sources: ${NOVA_ML_SOURCES}")
