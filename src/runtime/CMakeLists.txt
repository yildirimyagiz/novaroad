add_library(nova_runtime STATIC
    runtime.c
    value.c
    gc/gc.c
    gc/mark_sweep.c
    gc/generational.c
    gc/arena.c
    async/event_loop.c
    async/coroutine.c
    async/future.c
    threads/thread_pool.c
    threads/mutex.c
    threads/channel.c
    actor/actor.c
    actor/mailbox.c
    ffi/ffi_runtime.c
    ffi/c_interop.c
)

target_include_directories(nova_runtime PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(nova_runtime PRIVATE
    nova_std
    nova_plugin
)
