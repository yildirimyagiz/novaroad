cmake_minimum_required(VERSION 3.20)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# =============================================================================
# mimalloc
# =============================================================================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mimalloc/CMakeLists.txt)
    set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    add_subdirectory(mimalloc)
    message(STATUS "Nova: mimalloc — local")
elseif(NOVA_FETCH_DEPS)
    FetchContent_Declare(mimalloc
        GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
        GIT_TAG        v2.1.2)
    set(MI_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
    set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(mimalloc)
    message(STATUS "Nova: mimalloc — fetched")
else()
    # Stub: malloc/free yeterli
    add_library(mimalloc INTERFACE)
    target_compile_definitions(mimalloc INTERFACE
        mi_malloc=malloc mi_free=free mi_calloc=calloc mi_realloc=realloc)
    message(WARNING "Nova: mimalloc bulunamadı — stdlib malloc kullanılıyor")
endif()

# =============================================================================
# BLAKE3
# =============================================================================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blake3/c/blake3.h)
    add_library(blake3 STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/blake3/c/blake3.c
        ${CMAKE_CURRENT_SOURCE_DIR}/blake3/c/blake3_dispatch.c
        ${CMAKE_CURRENT_SOURCE_DIR}/blake3/c/blake3_portable.c)
    target_include_directories(blake3 PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/blake3/c)
    message(STATUS "Nova: blake3 — local")
elseif(NOVA_FETCH_DEPS)
    FetchContent_Declare(blake3
        GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
        GIT_TAG        1.5.1)
    FetchContent_MakeAvailable(blake3)
    message(STATUS "Nova: blake3 — fetched")
else()
    # Stub
    add_library(blake3 INTERFACE)
    file(WRITE ${CMAKE_BINARY_DIR}/blake3_stub.h
"#pragma once\n#include <stdint.h>\n#include <stddef.h>\n#include <string.h>\ntypedef struct { uint8_t _[1912]; } blake3_hasher;\nstatic inline void blake3_hasher_init(blake3_hasher *h) { memset(h,0,sizeof(*h)); }\nstatic inline void blake3_hasher_update(blake3_hasher *h, const void *in, size_t n) { (void)h;(void)in;(void)n; }\nstatic inline void blake3_hasher_finalize(const blake3_hasher *h, uint8_t *out, size_t n) { (void)h; memset(out,0,n); }\n")
    target_include_directories(blake3 INTERFACE ${CMAKE_BINARY_DIR})
    message(WARNING "Nova: blake3 bulunamadı — stub kullanılıyor")
endif()

# =============================================================================
# Unity (test framework)
# =============================================================================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/unity.c)
    add_library(unity STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/unity/src/unity.c)
    target_include_directories(unity PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/unity/src)
    message(STATUS "Nova: unity — local")
elseif(NOVA_FETCH_DEPS)
    FetchContent_Declare(unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.5.2)
    FetchContent_MakeAvailable(unity)
    message(STATUS "Nova: unity — fetched")
else()
    # Stub
    add_library(unity INTERFACE)
    file(WRITE ${CMAKE_BINARY_DIR}/unity.h
"#pragma once\n#include <stdio.h>\n#define TEST_ASSERT(c) do { if(!(c)) fprintf(stderr, \"FAIL %s:%d: %s\\n\",__FILE__,__LINE__,#c); } while(0)\n#define TEST_ASSERT_EQUAL(a,b) TEST_ASSERT((a)==(b))\n#define RUN_TEST(f) do { printf(\"  RUN %s\\n\",#f); f(); } while(0)\n#define UNITY_BEGIN() printf(\"=== Tests ===\\n\")\n#define UNITY_END()   printf(\"=== Done ===\\n\")\n")
    target_include_directories(unity INTERFACE ${CMAKE_BINARY_DIR})
    message(WARNING "Nova: unity bulunamadı — stub kullanılıyor")
endif()

# =============================================================================
# LLVM (opsiyonel — büyük bağımlılık)
# =============================================================================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/llvm/CMakeLists.txt)
    message(STATUS "Nova: llvm — local (büyük, uzun sürebilir)")
    add_subdirectory(llvm)
else()
    # Sistem LLVM'ini dene
    find_package(LLVM QUIET)
    if(LLVM_FOUND)
        message(STATUS "Nova: LLVM ${LLVM_VERSION} — sistem")
        include_directories(${LLVM_INCLUDE_DIRS})
        add_definitions(${LLVM_DEFINITIONS})
    else()
        message(STATUS "Nova: LLVM bulunamadı — LLVM backend devre dışı")
        add_library(nova_llvm_stub INTERFACE)
    endif()
endif()
# =============================================================================
# Nova ML Native Library
# =============================================================================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/native/ml/CMakeLists.txt)
    option(NOVA_BUILD_ML "Build Nova ML native library" ON)
    if(NOVA_BUILD_ML)
        add_subdirectory(src/native/ml)
        message(STATUS "Nova: ML native library enabled")
    endif()
endif()

# Core Nova Components
add_subdirectory(src/std)

# Plugin System library
add_library(nova_plugin STATIC src/plugin.c)
target_include_directories(nova_plugin PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
    target_link_libraries(nova_plugin PRIVATE dl pthread)
endif()

add_subdirectory(src/runtime)
add_subdirectory(src/compiler)
add_subdirectory(tools)
add_subdirectory(tests)

