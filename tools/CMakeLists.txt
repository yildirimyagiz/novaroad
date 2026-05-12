# Nova Compiler (znc)
add_executable(znc 
    znc/main.c
    znc/cli_opts.c
)

target_link_libraries(znc PRIVATE
    nova_compiler
    nova_runtime
    nova_std
)

# Nova REPL (znrepl)
add_executable(znrepl
    znrepl/main.c
)

target_link_libraries(znrepl PRIVATE
    nova_compiler
    nova_runtime
    nova_std
)

# Nova LSP Server (znlsp)
add_executable(znlsp
    znlsp/main.c
    znlsp/lsp_protocol.c
)

target_link_libraries(znlsp PRIVATE
    nova_compiler
    nova_std
)

# Install tools
install(TARGETS znc znrepl znlsp
    RUNTIME DESTINATION bin
)
