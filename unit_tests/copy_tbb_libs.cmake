# CMake script to find and copy TBB libraries
# Usage: cmake -DTBB_BUILD_DIR=<build_dir> -DTARGET_DIR=<target_dir> -P copy_tbb_libs.cmake

if(NOT TBB_BUILD_DIR)
    message(FATAL_ERROR "TBB_BUILD_DIR must be specified")
endif()

if(NOT TARGET_DIR)
    message(FATAL_ERROR "TARGET_DIR must be specified")
endif()

# Determine the platform and library extensions
if(WIN32)
    set(TBB_LIB_PATTERN "${TBB_BUILD_DIR}/**/tbb*.dll")
elseif(APPLE)
    set(TBB_LIB_PATTERN "${TBB_BUILD_DIR}/**/libtbb*.dylib")
else()
    set(TBB_LIB_PATTERN "${TBB_BUILD_DIR}/**/libtbb*.so*")
endif()

# Find TBB libraries
file(GLOB_RECURSE TBB_LIBS ${TBB_LIB_PATTERN})

if(TBB_LIBS)
    message(STATUS "Found TBB libraries: ${TBB_LIBS}")
    foreach(TBB_LIB ${TBB_LIBS})
        get_filename_component(LIB_NAME ${TBB_LIB} NAME)
        message(STATUS "Copying ${LIB_NAME} to ${TARGET_DIR}")
        file(COPY ${TBB_LIB} DESTINATION ${TARGET_DIR})
    endforeach()
else()
    message(WARNING "No TBB libraries found in ${TBB_BUILD_DIR}")
endif()
