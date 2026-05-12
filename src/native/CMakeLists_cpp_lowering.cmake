# CMake configuration for C++ lowering integration
# Include this in main CMakeLists.txt

# Find C++ compiler
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# C++ lowering source
set(NOVA_CPP_LOWERING_SRC
    src/compiler/lowering/nova_cpp_lowering.cpp
)

# Create C++ lowering library
add_library(nova_cpp_lowering STATIC ${NOVA_CPP_LOWERING_SRC})

target_include_directories(nova_cpp_lowering PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link with main nova compiler
target_link_libraries(nova_compiler PRIVATE nova_cpp_lowering)

# Set C++ specific flags
if(MSVC)
    target_compile_options(nova_cpp_lowering PRIVATE /W4 /EHsc)
else()
    target_compile_options(nova_cpp_lowering PRIVATE 
        -Wall -Wextra -pedantic
        -fno-exceptions  # Optional: disable exceptions if not needed
        -fno-rtti        # Optional: disable RTTI if not needed
    )
endif()

# Install
install(TARGETS nova_cpp_lowering
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

message(STATUS "C++ lowering support enabled")
