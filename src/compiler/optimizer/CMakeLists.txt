add_library(nova_optimizer STATIC
    optimizer.c
    const_fold.c
    dce.c
    inliner.c
    loop_opts.c
    simd_vectorizer.c
    alias_analysis.c
    escape_analysis.c
)

target_include_directories(nova_optimizer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova_optimizer PRIVATE
    nova_ir
)
