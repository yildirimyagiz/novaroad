# Nova vs PyTorch Real Benchmarks
cmake_minimum_required(VERSION 3.15)
project(NovaBenchmarks CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: Find PyTorch (libtorch)
# Download from: https://pytorch.org/get-started/locally/
# Set TORCH_DIR to libtorch installation path

option(NOVA_USE_PYTORCH "Build with PyTorch support" OFF)

if(NOVA_USE_PYTORCH)
    find_package(Torch QUIET)
    
    if(Torch_FOUND)
        message(STATUS "PyTorch found: ${TORCH_LIBRARIES}")
        add_definitions(-DNOVA_USE_PYTORCH)
        
        # Build PyTorch bridge
        add_library(pytorch_bridge SHARED
            ../src/ffi/pytorch_bridge.cpp
        )
        
        target_include_directories(pytorch_bridge PUBLIC
            ../src/ffi
            ${TORCH_INCLUDE_DIRS}
        )
        
        target_link_libraries(pytorch_bridge
            ${TORCH_LIBRARIES}
        )
        
        # Copy to lib directory
        install(TARGETS pytorch_bridge
            LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/../lib
        )
        
        message(STATUS "PyTorch benchmarks enabled")
    else()
        message(WARNING "PyTorch not found. Benchmarks will use estimated times.")
        message(STATUS "To enable real PyTorch benchmarks:")
        message(STATUS "  1. Download libtorch from https://pytorch.org/")
        message(STATUS "  2. Extract and set: cmake -DCMAKE_PREFIX_PATH=/path/to/libtorch")
        message(STATUS "  3. Reconfigure with: cmake -DNOVA_USE_PYTORCH=ON")
    endif()
else()
    message(STATUS "PyTorch benchmarks disabled. Use -DNOVA_USE_PYTORCH=ON to enable.")
endif()

# Stub library when PyTorch not available
if(NOT NOVA_USE_PYTORCH OR NOT Torch_FOUND)
    add_library(pytorch_bridge SHARED
        ../src/ffi/pytorch_bridge.cpp
    )
    
    target_include_directories(pytorch_bridge PUBLIC
        ../src/ffi
    )
endif()
