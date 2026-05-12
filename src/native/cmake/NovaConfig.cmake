# Nova CMake Configuration

get_filename_component(Nova_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# Set Nova include directories
set(Nova_INCLUDE_DIRS "${Nova_CMAKE_DIR}/../../include")

# Set Nova libraries
set(Nova_LIBRARIES nova_compiler nova_runtime nova_stdlib)

# Nova compiler executable
set(Nova_COMPILER "${Nova_CMAKE_DIR}/../../bin/novac")

# Check components
set(Nova_FOUND TRUE)

foreach(comp ${Nova_FIND_COMPONENTS})
    if(comp STREQUAL "compiler")
        # Component available
    elseif(comp STREQUAL "runtime")
        # Component available
    elseif(comp STREQUAL "stdlib")
        # Component available
    else()
        set(Nova_FOUND FALSE)
        message(WARNING "Unknown Nova component: ${comp}")
    endif()
endforeach()
