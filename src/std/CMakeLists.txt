add_library(nova_std STATIC
    alloc/alloc.c
    alloc/pool.c
    alloc/mimalloc_bridge.c
    string/nova_string.c
    string/rope.c
    collections/vec.c
    collections/hashmap.c
    collections/btree.c
    io/io.c
    io/buffered_io.c
    file/file_io.c
    net/socket.c
    net/tcp.c
    net/udp.c
)

target_include_directories(nova_std PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific networking
if(UNIX)
    target_compile_definitions(nova_std PRIVATE NOVA_POSIX)
elseif(WIN32)
    target_link_libraries(nova_std PRIVATE ws2_32)
    target_compile_definitions(nova_std PRIVATE NOVA_WINDOWS)
endif()
