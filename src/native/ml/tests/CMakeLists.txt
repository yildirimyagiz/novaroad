cmake_minimum_required(VERSION 3.16)
project(nova_ml_tests C)

enable_testing()

# Nova ML Unit Tests
set(ML_SRC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Common include dirs
set(ML_INCLUDES
    ${ML_SRC}/ai/core
    ${ML_SRC}/optimizer
    ${ML_SRC}/ai/matlab
)

# nova_metrics tests
add_executable(test_nova_metrics
    test_nova_metrics.c
    ${ML_SRC}/ai/core/nova_metrics.c
)
target_include_directories(test_nova_metrics PRIVATE ${ML_INCLUDES})
target_link_libraries(test_nova_metrics PRIVATE m)
add_test(NAME nova_metrics COMMAND test_nova_metrics)

# nova_tree tests
add_executable(test_nova_tree
    test_nova_tree.c
    ${ML_SRC}/optimizer/nova_tree.c
    ${ML_SRC}/ai/core/nova_metrics.c
)
target_include_directories(test_nova_tree PRIVATE ${ML_INCLUDES})
target_link_libraries(test_nova_tree PRIVATE m)
add_test(NAME nova_tree COMMAND test_nova_tree)

# nova_ensemble tests
add_executable(test_nova_ensemble
    test_nova_ensemble.c
    ${ML_SRC}/optimizer/nova_ensemble.c
    ${ML_SRC}/ai/core/nova_metrics.c
)
target_include_directories(test_nova_ensemble PRIVATE ${ML_INCLUDES})
target_link_libraries(test_nova_ensemble PRIVATE m)
add_test(NAME nova_ensemble COMMAND test_nova_ensemble)

# test_nova_rl
add_executable(test_nova_rl
    test_nova_rl.c
    ${ML_SRC}/ai/core/nova_rl.c
)
target_include_directories(test_nova_rl PRIVATE ${ML_INCLUDES})
target_link_libraries(test_nova_rl PRIVATE m pthread)
add_test(NAME nova_rl COMMAND test_nova_rl)

# test_nova_genetic
add_executable(test_nova_genetic
    test_nova_genetic.c
    ${ML_SRC}/optimizer/nova_genetic.c
)
target_include_directories(test_nova_genetic PRIVATE ${ML_INCLUDES})
target_link_libraries(test_nova_genetic PRIVATE m)
add_test(NAME nova_genetic COMMAND test_nova_genetic)

# test_nova_physics_opt
add_executable(test_nova_physics_opt
    test_nova_physics_opt.c
    ${ML_SRC}/optimizer/nova_physics_opt.c
)
target_include_directories(test_nova_physics_opt PRIVATE ${ML_INCLUDES})
target_link_libraries(test_nova_physics_opt PRIVATE m)
add_test(NAME nova_physics_opt COMMAND test_nova_physics_opt)

# test_nova_compute (self-contained)
add_executable(test_nova_compute test_nova_compute.c)
target_link_libraries(test_nova_compute PRIVATE m)
add_test(NAME nova_compute COMMAND test_nova_compute)

# test_nova_backends (self-contained)
add_executable(test_nova_backends test_nova_backends.c)
target_link_libraries(test_nova_backends PRIVATE m)
add_test(NAME nova_backends COMMAND test_nova_backends)

# test_nova_quantization
add_executable(test_nova_quantization
    test_nova_quantization.c
    ${ML_SRC}/quantization/nova_quantization.c
)
target_include_directories(test_nova_quantization PRIVATE
    ${ML_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
)
target_compile_options(test_nova_quantization PRIVATE
    -std=c11
    -Wno-unused-parameter
    -Wno-unused-variable
    -include ${ML_SRC}/nova_c_compat.h
)
target_link_libraries(test_nova_quantization PRIVATE m)
add_test(NAME nova_quantization COMMAND test_nova_quantization)

# test_nova_symbolic
add_executable(test_nova_symbolic
    test_nova_symbolic.c
    ${ML_SRC}/formal/nova_symbolic_ir.c
)
target_include_directories(test_nova_symbolic PRIVATE
    ${ML_INCLUDES}
    ${ML_SRC}/formal
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
)
target_compile_options(test_nova_symbolic PRIVATE
    -std=c11
    -Wno-unused-parameter
    -Wno-unused-variable
    -include ${ML_SRC}/nova_c_compat.h
)
target_link_libraries(test_nova_symbolic PRIVATE m)
add_test(NAME nova_symbolic COMMAND test_nova_symbolic)

# =============================================================================
# Extended tests (NOVA_ML_EXTENDED=ON)
# tensor + nn + adaptive_optimizer + learning_runtime + autotune_v2
# =============================================================================
if(NOVA_ML_EXTENDED)
    set(EXTENDED_INCLUDES
        ${ML_SRC}/ai/core
        ${ML_SRC}/optimizer
        ${ML_SRC}/optimizer/fusion
        ${ML_SRC}/memory
        ${ML_SRC}/formal
        ${ML_SRC}/ai/kernels
        ${ML_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/native/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/include
    )

    set(EXTENDED_SOURCES
        test_nova_tensor_extended.c
        nova_extended_stubs.c
        ${ML_SRC}/memory/nova_allocator.c
        ${ML_SRC}/formal/nova_formal.c
        ${ML_SRC}/ai/kernels/nova_kernels.c
        ${ML_SRC}/ai/core/nova_tensor.c
        ${ML_SRC}/ai/core/nova_tensor_ops.c
        ${ML_SRC}/ai/core/nova_tensor_sanity.c
        ${ML_SRC}/ai/core/nova_nn.c
        ${ML_SRC}/optimizer/nova_adaptive_optimizer.c
        ${ML_SRC}/optimizer/nova_learning_runtime.c
        ${ML_SRC}/optimizer/nova_autotune_v2.c
    )

    add_executable(test_nova_tensor_extended ${EXTENDED_SOURCES})
    target_include_directories(test_nova_tensor_extended PRIVATE ${EXTENDED_INCLUDES})
    target_compile_options(test_nova_tensor_extended PRIVATE
        -std=c11
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-value
        -include ${ML_SRC}/nova_c_compat.h
    )
    target_link_libraries(test_nova_tensor_extended PRIVATE m pthread)
    add_test(NAME nova_tensor_extended COMMAND test_nova_tensor_extended)
    message(STATUS "Nova ML: Extended tests enabled (tensor + nn + optimizer)")
endif()

# =============================================================================
# Metal/Accelerate backend tests (macOS only)
# =============================================================================
# =============================================================================
# End-to-End Training Pipeline Tests
# =============================================================================
set(E2E_SOURCES
    test_nova_training_e2e.c
    ${ML_SRC}/ai/core/nova_tensor.c
    ${ML_SRC}/ai/core/nova_tensor_ops.c
    ${ML_SRC}/ai/core/nova_tensor_sanity.c
    ${ML_SRC}/ai/core/nova_nn.c
    ${ML_SRC}/ai/core/nova_metrics.c
    ${ML_SRC}/ai/kernels/nova_kernels.c
    nova_extended_stubs.c
)
set(E2E_INCLUDES
    ${ML_SRC}/ai/core
    ${ML_SRC}/optimizer
    ${ML_SRC}/ai/kernels
    ${ML_SRC}/backends/cpu
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
)
add_executable(test_nova_training_e2e ${E2E_SOURCES})
target_include_directories(test_nova_training_e2e PRIVATE ${E2E_INCLUDES})
target_compile_options(test_nova_training_e2e PRIVATE
    -std=c11
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-value
    -include ${ML_SRC}/nova_c_compat.h
)
target_link_libraries(test_nova_training_e2e PRIVATE m pthread)
add_test(NAME nova_training_e2e COMMAND test_nova_training_e2e)
message(STATUS "Nova ML: End-to-end training pipeline tests enabled")

# =============================================================================
# AdamW Optimizer E2E Tests
# =============================================================================
set(ADAMW_SOURCES
    test_nova_adamw_e2e.c
    ${ML_SRC}/ai/core/nova_tensor.c
    ${ML_SRC}/ai/core/nova_tensor_ops.c
    ${ML_SRC}/ai/core/nova_tensor_sanity.c
    ${ML_SRC}/ai/core/nova_nn.c
    ${ML_SRC}/ai/core/nova_metrics.c
    ${ML_SRC}/ai/kernels/nova_kernels.c
    ${ML_SRC}/ai/optim/adamw.c
    ${ML_SRC}/ai/optim/sgd.c
    nova_extended_stubs.c
)
set(ADAMW_INCLUDES
    ${ML_SRC}/ai/core
    ${ML_SRC}/ai/optim
    ${ML_SRC}/ai/kernels
    ${ML_SRC}/backends/cpu
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
)
add_executable(test_nova_adamw_e2e ${ADAMW_SOURCES})
target_include_directories(test_nova_adamw_e2e PRIVATE ${ADAMW_INCLUDES})
target_compile_options(test_nova_adamw_e2e PRIVATE
    -std=c11
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-value
    -include ${ML_SRC}/nova_c_compat.h
)
target_link_libraries(test_nova_adamw_e2e PRIVATE m pthread)
add_test(NAME nova_adamw_e2e COMMAND test_nova_adamw_e2e)
message(STATUS "Nova ML: AdamW optimizer E2E tests enabled")

# =============================================================================
# Transformer / Attention E2E Tests
# =============================================================================
set(TRANSFORMER_SOURCES
    test_nova_transformer_e2e.c
    ${ML_SRC}/ai/core/nova_tensor.c
    ${ML_SRC}/ai/core/nova_tensor_ops.c
    ${ML_SRC}/ai/core/nova_tensor_sanity.c
    ${ML_SRC}/ai/core/nova_nn.c
    ${ML_SRC}/ai/core/nova_metrics.c
    ${ML_SRC}/ai/kernels/nova_kernels.c
    nova_extended_stubs.c
)
add_executable(test_nova_transformer_e2e ${TRANSFORMER_SOURCES})
target_include_directories(test_nova_transformer_e2e PRIVATE ${E2E_INCLUDES})
target_compile_options(test_nova_transformer_e2e PRIVATE
    -std=c11 -Wno-unused-parameter -Wno-unused-variable
    -include ${ML_SRC}/nova_c_compat.h
)
target_link_libraries(test_nova_transformer_e2e PRIVATE m pthread)
add_test(NAME nova_transformer_e2e COMMAND test_nova_transformer_e2e)
message(STATUS "Nova ML: Transformer/Attention E2E tests enabled")

# =============================================================================
# QAT (Quantization-Aware Training) E2E Tests
# =============================================================================
set(QAT_SOURCES
    test_nova_qat_e2e.c
    ${ML_SRC}/ai/core/nova_tensor.c
    ${ML_SRC}/ai/core/nova_tensor_ops.c
    ${ML_SRC}/ai/core/nova_tensor_sanity.c
    ${ML_SRC}/ai/core/nova_nn.c
    ${ML_SRC}/ai/core/nova_metrics.c
    ${ML_SRC}/ai/kernels/nova_kernels.c
    ${ML_SRC}/quantization/nova_quantization.c
    ${ML_SRC}/quantization/nova_qat.c
    nova_extended_stubs.c
)
set(QAT_INCLUDES
    ${ML_SRC}/ai/core
    ${ML_SRC}/ai/optim
    ${ML_SRC}/ai/kernels
    ${ML_SRC}/quantization
    ${ML_SRC}/backends/cpu
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
)
add_executable(test_nova_qat_e2e ${QAT_SOURCES})
target_include_directories(test_nova_qat_e2e PRIVATE ${QAT_INCLUDES})
target_compile_options(test_nova_qat_e2e PRIVATE
    -std=c11
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-value
    -include ${ML_SRC}/nova_c_compat.h
)
target_link_libraries(test_nova_qat_e2e PRIVATE m pthread)
add_test(NAME nova_qat_e2e COMMAND test_nova_qat_e2e)
message(STATUS "Nova ML: QAT E2E tests enabled")

if(APPLE)
    add_executable(test_nova_metal_accelerate
        test_nova_metal_accelerate.c
        ${ML_SRC}/backends/metal/nova_metal_accelerate.c
    )
    target_include_directories(test_nova_metal_accelerate PRIVATE
        ${ML_SRC}/backends/metal
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include
    )
    target_link_libraries(test_nova_metal_accelerate PRIVATE
        "-framework Accelerate"
        m
    )
    add_test(NAME nova_metal_accelerate COMMAND test_nova_metal_accelerate)
    message(STATUS "Nova ML: Metal/Accelerate backend tests enabled (macOS)")
endif()
