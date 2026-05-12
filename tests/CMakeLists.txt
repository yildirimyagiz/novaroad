# Unit tests compiled as separate executables to prevent duplicate 'main' symbols

# Helper macro to register tests easily
macro(add_nova_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name}
        nova_compiler
        nova_runtime
        nova_std
    )
    add_test(NAME ${name} COMMAND ${name})
endmacro()

add_nova_test(test_lexer unit/test_lexer.c)
add_nova_test(test_parser unit/test_parser.c)
add_nova_test(test_gc unit/test_gc.c)
add_nova_test(test_alloc unit/test_alloc.c)
add_nova_test(test_hashmap unit/test_hashmap.c)
add_nova_test(test_string unit/test_string.c)
