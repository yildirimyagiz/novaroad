add_library(nova_platform STATIC
    platform.c
    atomic.c
    simd.c
    intrinsics.c
)

target_include_directories(nova_platform PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
