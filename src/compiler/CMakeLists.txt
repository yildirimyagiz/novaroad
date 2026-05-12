add_library(nova_compiler STATIC
    frontend/lexer.c
    frontend/parser.c
    frontend/ast.c
    backend/codegen.c
    typesystem/type_checker.c
    typesystem/borrow_checker.c
    typesystem/types.c
    typesystem/traits.c
    typesystem/lifetimes.c
    typesystem/type_infer.c
    tokens.c
    sourcemgr.c
    diagnostics.c
    dimensions.c
    contracts.c
    module_registry.c
    type_unification.c
    pattern_semantic.c
    pattern_matching.c
    exhaustiveness.c
    monomorphization.c
    semantic.c
    compiler.c
    parser_error.c
    physics_constants.c
    nova_integration.c
    backend/vm/vm.c
    backend/vm/bytecode.c
    backend/vm/chunk.c
    backend/vm/opcode.c
    backend/metal_backend.c
    backend/spirv_backend.c
    backend/jit/jit_x86_64.c
    compute/nova_execution_fabric.c
    compute/nova_dispatcher.c
    compute/nova_scheduler.c
    ir/ir.c
    backend/llvm/nova_llvm_backend.c
)


add_executable(novac main.c)

target_include_directories(nova_compiler PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_include_directories(novac PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(nova_compiler PRIVATE
    nova_std
    nova_runtime
)

target_link_libraries(novac PRIVATE
    nova_compiler
    nova_std
    nova_runtime
)

# Optional LLVM support
find_package(LLVM CONFIG)
if(LLVM_FOUND)
    message(STATUS "Nova: Found LLVM ${LLVM_VERSION}")
    target_sources(nova_compiler PRIVATE
        backend/llvm/nova_llvm_codegen.c
        backend/llvm/nova_llvm_emit.c
    )
    target_include_directories(nova_compiler PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(nova_compiler PRIVATE HAVE_LLVM)
    target_link_libraries(nova_compiler PRIVATE LLVM)
    target_link_libraries(novac PRIVATE LLVM)
    target_compile_definitions(novac PRIVATE HAVE_LLVM)
endif()
