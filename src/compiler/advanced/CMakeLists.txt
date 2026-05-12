add_library(nova_advanced_opt STATIC
    neural_optimizer.c
    self_proving.c
    resonance_optimizer.c
)

target_include_directories(nova_advanced_opt PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova_advanced_opt PRIVATE
    nova_compiler
    nova_ai
    nova_std
)

# Link math library
target_link_libraries(nova_advanced_opt PRIVATE m)
