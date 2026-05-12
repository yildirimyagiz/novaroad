add_library(nova_security STATIC
    capabilities.c
    sandbox.c
    crypto/hash.c
    crypto/aead.c
    crypto/kdf.c
    crypto/signing.c
    audit/audit_log.c
)

target_include_directories(nova_security PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova_security PRIVATE
    nova_std
)
